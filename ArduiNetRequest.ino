// Connect to ethernet with DHCP, send and track packets with response.
// 2016-08-10 <info@gemins.com.ar> http://opensource.org/licenses/mit-license.php

#include <EtherCard.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int idPos = 0;
byte idDevice;
// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

#define BUFFERSIZE 500
byte Ethernet::buffer[BUFFERSIZE];

static uint32_t timer;

uint32_t nextSeq;
char fullResponse[BUFFERSIZE - 200];

const char website[] PROGMEM = "atm.gemins.com.ar";

// called when the client request is complete
static void getDevices (byte status, word off, word len, word test) {
  Serial.println(">>>");
  //Serial.println(status);
  //Ethernet::buffer[len] = off;
  char* fullResponse;
  fullResponse = (char*) Ethernet::buffer + 294;
  String str = String(fullResponse);
  //Serial.println( (char*) Ethernet::buffer + 294);
  char json[str.length()];
  str.toCharArray(json, str.length() + 1);
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fullResponse);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  //root.printTo(Serial);
  const char* name = root["name"];
  const char* time = root["create_at"];
  const char* id = root["id"];
  const char* user_id = root["user_id"];

  EEPROM.write(idPos, id);
}

void getJson(){
  //Serial.println( (char*) Ethernet::buffer + 294);
  //char json[response.length()];
  //response.toCharArray(json, response.length() + 1);
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fullResponse);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
    return;
  }
  //root.printTo(Serial);
  const char* name = root["ixd"]["name"];
  const char* time = root["ixd"]["create_at"];
  const char* id = root["ixd"]["id"];
  const char* user_id = root["ixd"]["user_id"];

  Serial.print("ID: ");
  Serial.println(id);
  memset(fullResponse, 0, sizeof(fullResponse));

  EEPROM.write(idPos, id);
}

// called when the client request is complete
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
    Serial.println();
    getJson();
  }

  /*
  String str = String(fullResponse);
  char json[str.length()];
  str.toCharArray(json, str.length() + 1);
 
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fullResponse);
  if (!root.success())
  {
    Serial.println(fullResponse);
    Serial.println("parseObject() failed");
    return;
  }
  root.printTo(Serial);
  */
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

  //idDevice = EEPROM.read(idPos);
  //if(!idDevice)
  //  Serial.println("Not have id yet");
  //else
  //  Serial.println(idDevice);
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  if (millis() > timer) {
    timer = millis() + 10000;
    Serial.println("GET:");
    ether.browseUrl(PSTR("/devices/"), "9", website, getResponse);
  }
}
