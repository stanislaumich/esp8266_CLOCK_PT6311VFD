/*To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include "FS.h"
#include <WiFiUdp.h>
//#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <vars.h>
//#include <util/delay.h>
#include <PT6311.h>
#define GMT 3
////////////////////////////////////////////////////////////////////////////////
#define NODEMCU_D0 16
#define NODEMCU_D1 5
#define NODEMCU_D2 4
#define NODEMCU_D3 0
#define NODEMCU_D4 2
#define NODEMCU_D5 14
#define NODEMCU_D6 12
#define NODEMCU_D7 13
#define NODEMCU_D8 15
#define NODEMCU_D9 3
#define NODEMCU_D10 1
#define NODEMCU_D12 10


#define VFD_CS_PIN D1   //blue 
#define VFD_CLK_PIN D2  //gray 
#define VFD_DATA_PIN D3 //brown 


#define VFD_BYTES_PER_DIGIT 3      //3
PT6311 pt6311_driver;
// numbers
int f[10]={140,128,12,132,128,132,140,128,140,132};
int s[10]={196,64,195,195,71,135,135,192,199,199};
// symbols
int sf[10]={140,128,12,132,128,132,140,128,140,132};
int ss[10]={196,64,195,195,71,135,135,192,199,199};


////////////////////////////////////////////////////////////////////////////////
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
HTTPClient client;
WiFiUDP udp;
const char* serverUpload = "Upload File<br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>";
const char* serverIndex = "Update Firmvare<br><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
IPAddress myIP;
File fsUploadFile;
// -----------------------------------------------------------------------------
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
void write_raw(uint8_t value, uint8_t number_of_bytes) 
{
    pt6311_driver.displayMemWriteCmd(true, false);
    for (uint8_t i = 0; i < number_of_bytes; i++)
    {
        pt6311_driver.data(value, false, ((i + 1) == number_of_bytes));
    }
}

void fill_mem(uint8_t value, uint8_t number_of_bytes, uint8_t start_addr)
{
    pt6311_driver.addrSetCmd(start_addr); 
    write_raw(value, number_of_bytes);
}

void showfirst(int s1,int s2, int s3)
{
  //27
  pt6311_driver.addrSetCmd(27);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(sf[s1]+0, false, false);
  pt6311_driver.data(ss[s1], false, false);
  pt6311_driver.data(0, false, true);
  //24
  pt6311_driver.addrSetCmd(24);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(sf[s2]+2, false, false);
  pt6311_driver.data(ss[s2], false, false);
  pt6311_driver.data(0, false, true);
  //21
  pt6311_driver.addrSetCmd(21);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(sf[s3]+0, false, false);
  pt6311_driver.data(ss[s3], false, false);
  pt6311_driver.data(0, false, true);  
}

void showtime(int h, int m,int ss)
{
  //18
  pt6311_driver.addrSetCmd(18);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[(h-h%10)/10]+0, false, false);
  pt6311_driver.data(s[(h-h%10)/10], false, false);
  pt6311_driver.data(0, false, true);
  //15
  pt6311_driver.addrSetCmd(15);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[h%10]+2, false, false);
  pt6311_driver.data(s[h%10], false, false);
  pt6311_driver.data(0, false, true);
  //12
  pt6311_driver.addrSetCmd(12);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[(m-m%10)/10]+0, false, false);
  pt6311_driver.data(s[(m-m%10)/10], false, false);
  pt6311_driver.data(0, false, true);
  //9
  pt6311_driver.addrSetCmd(9);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[m%10]+2, false, false);
  pt6311_driver.data(s[m%10], false, false);
  pt6311_driver.data(0, false, true);
  //6
  pt6311_driver.addrSetCmd(6);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[(ss-ss%10)/10]+0, false, false);
  pt6311_driver.data(s[(ss-ss%10)/10], false, false);
  pt6311_driver.data(0, false, true);
  //3
  pt6311_driver.addrSetCmd(3);
  pt6311_driver.displayMemWriteCmd(true, false);
  pt6311_driver.data(f[ss%10], false, false);
  pt6311_driver.data(s[ss%10], false, false);
  pt6311_driver.data(0, false, true);
}

void initWiFi(){////////////////////////////////////////////////////////////////
 beeplen=15;
 WiFi.hostname(myHostname);
 WiFi.softAP("OTANN", "2932802951");
 Serial.println("CONNECTING...");
 WiFi.config(ip, gateway, subnet, dns1, dns2);
 WiFi.mode(WIFI_AP_STA);
 myIP = WiFi.softAPIP();
 File f = SPIFFS.open("/wifi.txt", "r");
 String ssidt=f.readStringUntil('\n');
  for (int i=0; i<5; i++)
  {
  String ssidt=f.readStringUntil('\n');
  ssid=ssidt.substring(0, ssidt.length()-1);
  String passwordt=f.readStringUntil('\n');
  password=passwordt.substring(0, passwordt.length()-1);
  WiFiMulti.addAP(ssid.c_str(), password.c_str());
 }
 f.close();
 previousMillis = millis();
 while ((WiFiMulti.run() != WL_CONNECTED)&&beeplen==15) {
 delay(500);
 unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > conntimeout){
      beeplen=150;
  }
 }
 if((WiFiMulti.run() == WL_CONNECTED))
 {
  fstr =  WiFi.SSID().c_str();
  Serial.println(fstr);
  Serial.println(WiFi.localIP());
  beeplen=15;
 }
 else{
  Serial.println("NO NETWORK");
  beeplen=150;
 }
 }
//-----------------------  ---------------------------
void msg(int i){Serial.print("Query");Serial.println(i);}
// SERVER //////////////////////////////////////////////////////////////////////////////
void handleRoot() {
  msg(1);
  myIP = WiFi.softAPIP();
  message=">>>/u update /reboot IP=";
  message.concat("192.168.1.129");
  server.send(200, "text/plain", message);
  msg(0);
}
void handleNotFound(){
  msg(1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  msg(0);
}

String XML;
void handleXML(){
  buildXML();
  server.send(200,"text/xml",XML);
}
// создаем xml данные
void buildXML(){
  XML="<?xml version='1.0'?>";
  XML+="<Donnees>"; 
    XML+="<response>";
    XML+="$value";
    XML+="</response>";
    XML+="<alert_time>";
    XML+="$value";
    XML+="</alert_time>";
    XML+="<time>";
    XML+="$value";
    XML+="</time>";
  XML+="</Donnees>"; 
}

void handleFileUpload() {
 if (server.uri() != "/upload") return;
 HTTPUpload& upload = server.upload();
 if (upload.status == UPLOAD_FILE_START) {
 String filename = upload.filename;
 if (!filename.startsWith("/")) filename = "/" + filename;

 fsUploadFile = SPIFFS.open(filename, "w");
 filename = String();
 } else if (upload.status == UPLOAD_FILE_WRITE) {
 //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
 if (fsUploadFile)
 fsUploadFile.write(upload.buf, upload.currentSize);
 } else if (upload.status == UPLOAD_FILE_END) {
 if (fsUploadFile)
 fsUploadFile.close();
 }
}
String getContentType(String filename) {
 if (server.hasArg("download")) return "application/octet-stream";
 else if (filename.endsWith(".htm")) return "text/html";
 else if (filename.endsWith(".html")) return "text/html";
 else if (filename.endsWith(".css")) return "text/css";
 else if (filename.endsWith(".js")) return "application/javascript";
 else if (filename.endsWith(".png")) return "image/png";
 else if (filename.endsWith(".gif")) return "image/gif";
 else if (filename.endsWith(".jpg")) return "image/jpeg";
 else if (filename.endsWith(".ico")) return "image/x-icon";
 else if (filename.endsWith(".xml")) return "text/xml";
 else if (filename.endsWith(".pdf")) return "application/x-pdf";
 else if (filename.endsWith(".zip")) return "application/x-zip";
 else if (filename.endsWith(".gz")) return "application/x-gzip";
 return "text/plain";
}
bool FileRead(String path) {
 String contentType = getContentType(path);
 if (SPIFFS.exists(path)) {
 File file = SPIFFS.open(path, "r");
 size_t sent = server.streamFile(file, contentType);
 file.close();
 return true;
 }
 return false;
}
void handleFileDownload() {
  bool e=FileRead(server.arg(0));
}
//-----------------------  ---------------------------
void wrtest(){
    File f = SPIFFS.open("/wifi.txt", "w");
    if (!f) {
        Serial.println("file open failed");
    }
    Serial.println("====== Writing to SPIFFS file =========");
      f.println("7");
      f.println("ZTE54");       f.println("121211119");
      f.println("ZTE541");       f.println("1212111110");
    f.close();
}
/////////////////////////////////////////////////////////////////////////////////
void synctime()
{
  WiFi.hostByName(ntpServerName, timeServerIP);
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);
    Serial.print("Unix time = ");
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    Serial.println(epoch);
    epoch = epoch + GMT * 3600;
    hour = (epoch  % 86400L) / 3600;
    minute = (epoch  % 3600) / 60;
    second = epoch % 60;
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
  }
}

//----------------------- SETUP ---------------------------
void myinitpins(){}
void myinitf(){ 
  myinitpins();
  
  }
void myinitl(){}

void setup(void){
 //----------------Display INIT
  pt6311_driver.init(VFD_CS_PIN, VFD_CLK_PIN, VFD_DATA_PIN);
  pt6311_driver.reset(VFD_DISP_MODE_10D18S); // good VFD_DISP_MODE_10D18S
   for (uint8_t i = 0; i < 12; i++) 
  {
      fill_mem(0x00, VFD_BYTES_PER_DIGIT, i * VFD_BYTES_PER_DIGIT);
      delay(10); 
  }
  showtime(0,0,0);
  //----------------Display

 Serial.begin(9600);
 Serial.println("Start Setup...");
 Serial.println("myinitf()");
 

 myinitf();
 SPIFFS.begin();
 //SPIFFS.format(); wrtest();
 initWiFi();
 Serial.println("server.begin()");
 server.on("/",  handleRoot);
 server.onNotFound(handleNotFound);
 server.on("/down",  handleFileDownload);
 //server.on("/u", HTTP_GET, [](){server.sendHeader("Connection", "close");server.send(200, "text/html", serverIndex);});
 server.on("/up", HTTP_GET, [](){server.sendHeader("Connection", "close");server.send(200, "text/html", serverUpload);});
 server.on("/u", HTTP_POST, handleFileUpload);
 server.on("/xml",handleXML); // формирование xml страницы для передачи данных в web интерфейс
 
 server.on("/update", HTTP_POST, [](){
   server.sendHeader("Connection", "close");
   server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
   ESP.restart();},[](){
   HTTPUpload& upload = server.upload();
   if(upload.status == UPLOAD_FILE_START){
     Serial.setDebugOutput(true);
     WiFiUDP::stopAll();
     Serial.printf("Update: %s\n", upload.filename.c_str());
     uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
     if(!Update.begin(maxSketchSpace)){//start with max available size
       Update.printError(Serial);
     }
    } else if(upload.status == UPLOAD_FILE_WRITE){
     if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
       Update.printError(Serial);
     }
    } else if(upload.status == UPLOAD_FILE_END){
     if(Update.end(true)){ //true to set the size to the current progress
       Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
     } else {
       Update.printError(Serial);
     }
     Serial.setDebugOutput(false);
   }
   yield();
  });
 server.on("/reboot", [](){server.send(200, "text/plain", "Restarting ESP");ESP.restart();});
 server.begin();
 Serial.println("HTTP server started");
 Serial.println("myinitl()");
 myinitl();
 Serial.println("Stop Setup");
 udp.begin(localPort);
 synctime();// синхронизируем время при включении
 }
//----------------------- LOOP ---------------------------
void loop(void){
  server.handleClient();
  if ((second == 0)||(second == 1)){            // если насчитали 60 сек
    synctime();                  // синхронизируем время
  }
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval)
  {
   second=second+1;
   second==60?second=0:second=second;
   previousMillis = currentMillis;
   //Serial.print("Work! -> ");
   //Serial.println(millis());
   Serial.print(hour);
   Serial.print(":");
   Serial.print(minute);
   Serial.print(":");
   Serial.print(second);
   Serial.println(".");
   showtime(hour,minute,second);
   }
}
