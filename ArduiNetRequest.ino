// Connect to ethernet with DHCP, send and track packets with response.
// 2016-08-10 <info@gemins.com.ar> http://opensource.org/licenses/mit-license.php

#include <EtherCard.h>
#include <ArduinoJson.h>
#include <DHT11.h>
#include <EEPROM.h>

/** EEPROM Vars **/
  int idPos = 0;
  byte idDevice;
/** END EEPROM Vars**/

/*Ethernet Configuration and Vars*/
  // ethernet interface mac address, must be unique on the LAN
  static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
  //Size of buffer
  #define BUFFERSIZE 500
  byte Ethernet::buffer[BUFFERSIZE];
  uint32_t nextSeq;
  char fullResponse[BUFFERSIZE - 200];
  const char website[] PROGMEM = "atm.gemins.com.ar";
/*END Ethernet Configuration and Vars*/

/** PINS Config and Vars **/
  int pinTemp = 2;
  DHT11 dht11(pinTemp);

/** END PINS Config and Vars **/

/** Function to parse a Json data from Server **/
void getJson(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fullResponse);
 
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    memset(fullResponse, 0, sizeof(fullResponse));
  }else{
    setDataDevices(root);
  }
}

/** Function to get and set new data from server **/
static void setDataDevices(JsonObject& objJson){
  const uint16_t id = objJson["ixd"]["id"];

  Serial.print(id);

  if(id != "" and (!idDevice or EEPROM.read(idPos) != id)){
    Serial.print("change id");
    EEPROM.put(idPos, id);
  }else{
    Serial.print("NOT change id");
  }
  
  memset(fullResponse, 0, sizeof(fullResponse));
}

/** Function to get Response data from server when make a GET **/
static void getResponse (byte status, word off, word len) {
  char* tempBuffer;
    
  if (strncmp_P((char*) Ethernet::buffer+off,PSTR("HTTP"),4) == 0) {
    nextSeq = ether.getSequenceNumber();
    memset(fullResponse, 0, sizeof(fullResponse));
  }
 
  if (nextSeq != ether.getSequenceNumber()) { Serial.print(F("<IGNORE DUPLICATE(?) PACKET>")); return; }
    
  uint16_t payloadlength = ether.getTcpPayloadLength();
  int16_t chunk_size   = BUFFERSIZE-off-1;
  int16_t char_written = 0;
  int16_t char_in_buf  = chunk_size < payloadlength ? chunk_size : payloadlength;
  
  while (char_written < payloadlength) {
    String newResponse = Ethernet::buffer + off;
    int findIxd = newResponse.indexOf("ixd");
    
    if(strlen(fullResponse) == 0 && findIxd > 0){
      tempBuffer = Ethernet::buffer + off + findIxd - 2; 
    }
    
    if(strlen(fullResponse) > 0){
      tempBuffer = Ethernet::buffer + off;
    }
    
    sprintf(fullResponse, "%s%s", fullResponse, tempBuffer);
    
    char_written += char_in_buf;
    char_in_buf = ether.readPacketSlice((char*) Ethernet::buffer+off,chunk_size,off+char_written);
  }

  nextSeq += ether.getTcpPayloadLength();

  if(payloadlength < 512){
    tempBuffer[0] = 0;tempBuffer[off+len] = 0;
    Serial.println(fullResponse);
    getJson();
  }
}

void setup () {
  Serial.begin(57600);
  Serial.println(F("[webClient]"));
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);

  ether.persistTcpConnection(true);
  memset(fullResponse, 0, sizeof(fullResponse));

  //if(!idDevice)
  //  EEPROM.write(idPos,6);

}

void loop () {
  ether.packetLoop(ether.packetReceive());

  static uint32_t timer =  0;
  if (millis() > timer && strlen(fullResponse) == 0) {
    char varSend[50];
    idDevice = EEPROM.read(idPos);
    Serial.print("ID actual: "); Serial.println(idDevice);
    
    timer = millis() + 10000;

    int err;
    float temp, hum;
    
    if((err = dht11.read(hum, temp)) == 0)    // Si devuelve 0 es que ha leido bien
    {
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.print(" Humedad: ");
      Serial.print(hum);
      Serial.println();
      //If id devices is not set, create a new devices on web and get data.

      char* wanIp = sprintf("%c.%c.%s.%s", ether.hisip[0],ether.hisip[1],ether.hisip[2],ether.hisip[3]);
      //Serial.println(wanIp);

      int T(temp);
      int H(hum);
      
      if(!idDevice)
        sprintf(varSend, "create?ip=%s", wanIp);
      else //else if devices have id, connect to server to send and recive data.
        sprintf(varSend, "%d/update_sensor?temperature=%d&humidity=%d&ip=%s", idDevice, T, H, wanIp);
      
      Serial.println(varSend);
      
      ether.browseUrl(PSTR("/devices/"), varSend, website, getResponse);
    }else{
       Serial.println();
       Serial.print("Error Num :");
       Serial.print(err);
       Serial.println();
    }    
  }
}
