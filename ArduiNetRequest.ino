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

byte Ethernet::buffer[700];

static uint32_t timer;
String fullResponse;
String newResponse;

#define HTTP_HEADER_OFFSET 0

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

// called when the client request is complete
static void createDevices (byte status, word off, word len, word delaycnt) {
  newResponse = Ethernet::buffer + off;
  if(status == 0){
    fullResponse = Ethernet::buffer + off + newResponse.indexOf("application/json") + 20;
    String str(fullResponse);
    Serial.println(fullResponse);
  }else{
    String str(newResponse);
    fullResponse = fullResponse + newResponse;
    Serial.println(fullResponse);
  }
 char* lastChar;
 lastChar = (char *) Ethernet::buffer + len + off - 1;
 //Serial.println(fullResponse);
 //Serial.println(newResponse.indexOf("application/json"));
 

 
  if(lastChar[0] == ']' ){
    Serial.println("Yeah!");
    Serial.println(newResponse);
    Serial.println(fullResponse);
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
  Serial.begin(9600);
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

  //ether.persistTcpConnection(true);

  idDevice = EEPROM.read(idPos);
  if(!idDevice)
    Serial.println("Not have id yet");
  else
    Serial.println(idDevice);
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  if (millis() > timer) {
    timer = millis() + 30000;
    Serial.println();
    ether.browseUrl(PSTR("/devices"), "/9", website, createDevices);
    ether.persistTcpConnection(true);
  }
}
