#ifndef vars_h
#define vars_h

int tvar=0;
int beeplen=150;

int greenpin=2;
int redpin=4;
int bluepin=15;
int soundpin=5;
int yellowpin=16;
bool snd=false;

int maxl = 600;
int minl = 20;
int ln = 15;
int step;

IPAddress ip(192, 168, 1, 129);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(8, 8, 8, 8);
IPAddress dns2(194, 158, 206, 206);
String ssid;
String password;
String myHostname = "LZ42N";
const char* host = "esp8266N";
long conntimeout = 30000; // сколько пытаться подключаться к сети
String message;
String fstr;
String srvaddr;
unsigned long delaytime=250;
long interval = 1000;// интервал повторения цикличных действий в теле loop
unsigned long previousMillis = 0;
byte hour, minute, second;
byte weekday;// день недели
boolean point;
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets




int j=0;
bool tb;
#endif
