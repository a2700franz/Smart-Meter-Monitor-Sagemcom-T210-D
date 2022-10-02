/*
  M-Bus Data to WLAN transmitter for SmartMeter T210-D
  by Franz Stoiber

  Hints -------------------------------------------------------
  -------------------------------------------------------------

  History -----------------------------------------------------
  2022-09-12 begin programming
  2022-09-21 V1.0
  2022-09-24 web settings for udp and logging included
  2022-09-25 V1.1
  -------------------------------------------------------------

  Bugs --------------------------------------------------------
  -------------------------------------------------------------

  Marks --------------------------------------------------------
  -------------------------------------------------------------

  Hardware ----------------------------------------------------
  controller: ESP8266 80 MHz
  -------------------------------------------------------------

  Pin usage ---------------------------------------------------
  DEV-BOARD                ESP-01
  Pin  5.0V
  Pin  3V3                 VCC 3V3
  Pin  GND                 GND
  Pin  EN                  EN
  Pin  RESET               RST
  Pin  INT
  Pin  MOSI
  Pin  MISO
  Pin  SCLK
  Pin  A0       ADC
  Pin  D0       GPIO16
  Pin  D1/SDA   GPIO5
  Pin  D2/SCL   GPIO4
  Pin  D3       GPIO0      FLASH
  Pin  D4       GPIO2      TX1 (LOG)
  Pin  D5       GPIO14
  Pin  D6       GPIO12
  Pin  D7       GPIO13
  Pin  D8       GPIO15
  Pin  D9       GPIO3      RX0 (DATA)
  Pin  D10      GPIO1      TX0
  -------------------------------------------------------------
*/

//includes
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>

//declarations for prg
const String PrgName = "MeterDataTransmitter";
const String PrgVersion = "V1.1";

//declarations for EEPROM
#define EEPROM_Size 256
int32_t EEAdrSsid = 0; //max len 32
int32_t EEAdrPassword = 33;     //max len 63
int32_t EEAdrUDPDataHost = 97;  //max len 15
int32_t EEAdrUDPDataPort = 113; //2 byte
int32_t EEAdrSerialLog = 115;   //1 byte
int32_t EEAdrUDPLog = 116;      //1 byte
int32_t EEAdrUDPLogHost = 117;  //max len 15
int32_t EEAdrUDPLogPort = 133;  //2 byte

//declarations for udp
String UDPDataHost;
IPAddress UDPDataHostIP;
uint16_t UDPDataPort;
String UDPLogHost;
IPAddress UDPLogHostIP;
uint16_t UDPLogPort;

//declarations for logging
uint8_t SerialLog;
uint8_t UDPLog;
char Item[256];

//declarations for WiFi
String Ssid;
String Password;
String APSsid = "MeterDataTransmitter";
String APPassword = "MeterDataTransmitter";
const char* Host = "meter-data-transmitter";
int16_t Rssi;
bool isAP = false;

//declaration for http
String HttpText;

//declarations for frame
const uint16_t FrameLength = 282;

//instance web server
ESP8266WebServer SettingsServer(80);

//instance update server
ESP8266WebServer UpdateServer(81);

//instance web updater
ESP8266HTTPUpdateServer WebUpdater;

//instance WiFiUdp
WiFiUDP udp;

//websites
const String WebHeader = "<html>\
  <head>\
    <title>MeterDataTransmitter</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>";

const String SettingsForm = "<body>\
      <h1>" + PrgName + " " + PrgVersion + "<br>Configuration</h1>\
      #response#\
      <form method=\"post\" enctype=\"application/x-www-form-urlencoded\">\
      SSID:<br>\
      <input type=\"text\" name=\"p_ssid\" value=\"#ssid#\" style=\"width: 300px\"><br><br>\
      Password:<br>\
      <input type=\"text\" name=\"p_pw\" value=\"#password#\" style=\"width: 300px\"><br><br>\
      <br>\
      UDP Data Host IP:<br>\
      <input type=\"text\" name=\"p_dh\" value=\"#udp_data_host#\" style=\"width: 200px\"><br><br>\
      UDP Data Port Nr:<br>\
      <input type=\"text\" name=\"p_dp\" value=\"#udp_data_port#\" style=\"width: 100px\"><br><br>\
      <br>\
      Serial Logging (0/1):<br>\
      <input type=\"text\" name=\"p_ls\" value=\"#serial_log#\" style=\"width: 50\"><br><br>\
      <br>\
      UDP Logging (0/1):<br>\
      <input type=\"text\" name=\"p_lu\" value=\"#udp_log#\" style=\"width: 50\"><br><br>\
      UDP Log Host IP:<br>\
      <input type=\"text\" name=\"p_lh\" value=\"#udp_log_host#\" style=\"width: 200px\"><br><br>\
      UDP Log Port Nr:<br>\
      <input type=\"text\" name=\"p_lp\" value=\"#udp_log_port#\" style=\"width: 100px\"><br><br>\
      <br><br><br>\
      <input type=\"submit\" name=\"p_config\" value=\"store Configuration\" style=\"width: 200px; height:40px;font-weight:bold;font-size:16px\"><br><br>\
    </form>\
    </body>\
  </html>";

const String InfoForm = "<body>\
    <h1>" + PrgName + "</h1>\
    <h4>\
    SSID: #ssid#<br>\
    Password: #password#<br>\
    <br>\
    UDP Data Host: #udp_data_host#<br>\
    UDP Data Port: #udp_data_port#<br>\
    <br>\
    Serial Logging: #serial_log#<br>\
    <br>\
    UDP Logging: #udp_log#<br>\
    UDP Log Host: #udp_log_host#<br>\
    UDP Log Port: #udp_log_port#<br>\
    <br>\
    <br>\
    Configuration stored<br>\
    MeterDataTransmitter restart with new configuration in 10 seconds ...\
    </h4>\
    </body>\
  </html>";

void readConfig() {
  Ssid = EEReadString(EEAdrSsid);
  if (Ssid == "") {
    Ssid = "ssid";
    Password = "password";
  }
  else Password = EEReadString(EEAdrPassword);
  UDPDataHost = EEReadString(EEAdrUDPDataHost);
  if (UDPDataHost == "") UDPDataHost = "192.168.0.000";
  UDPDataHostIP.fromString(UDPDataHost);
  UDPDataPort = EEReadUint16(EEAdrUDPDataPort);
  if (UDPDataPort == 0xffff) UDPDataPort = 5000;
  SerialLog = EEReadUint8(EEAdrSerialLog);
  if (SerialLog != 0) SerialLog = 1;
  UDPLog = EEReadUint8(EEAdrUDPLog);
  if (UDPLog != 0) UDPLog = 1;
  UDPLogHost = EEReadString(EEAdrUDPLogHost);
  if (UDPLogHost == "") UDPLogHost = "192.168.0.000";
  UDPLogHostIP.fromString(UDPLogHost);
  UDPLogPort = EEReadUint16(EEAdrUDPLogPort);
  if (UDPLogPort == 0xffff) UDPLogPort = 5001;
}

void writeConfig() {
  if (Ssid.length() > 32) Ssid = Ssid.substring(0, 32);
  EEWriteString(EEAdrSsid, Ssid);
  EEWriteString(EEAdrPassword, Password);
  EEWriteString(EEAdrUDPDataHost, UDPDataHost);
  EEWriteUint16(EEAdrUDPDataPort, UDPDataPort);
  EEWriteUint8(EEAdrSerialLog, SerialLog);
  EEWriteUint8(EEAdrUDPLog, UDPLog);
  EEWriteString(EEAdrUDPLogHost, UDPLogHost);
  EEWriteUint16(EEAdrUDPLogPort, UDPLogPort);
  EEPROM.commit();
}

String EEReadString(uint16_t EEAdr) {
  uint16_t i;
  uint8_t Len;
  String Text = "";
  Len = EEPROM.read(EEAdr);
  if (Len == 0 || Len == 255) return "";
  EEAdr++;
  for (i = 0; i < Len; i++) Text += char(EEPROM.read(EEAdr + i));
  return Text;
}

void EEWriteString(uint16_t EEAdr, String Text) {
  uint16_t i;
  uint8_t Len;
  Len = Text.length();
  EEPROM.write(EEAdr, Len);
  EEAdr++;
  for (i = 0; i < Len; i++) EEPROM.write(EEAdr + i, Text[i]);
}

uint16_t EEReadUint16(uint16_t EEAdr) {
  uint8_t HiByte;
  uint8_t LoByte;
  HiByte = EEPROM.read(EEAdr);
  LoByte = EEPROM.read(EEAdr + 1);
  return HiByte * 256 + LoByte;
}

void EEWriteUint16(uint16_t EEAdr, uint16_t Value) {
  EEPROM.write(EEAdr, highByte(Value));
  EEPROM.write(EEAdr + 1, lowByte(Value));
}

uint16_t EEReadUint8(uint16_t EEAdr) {
  return EEPROM.read(EEAdr);
}

void EEWriteUint8(uint16_t EEAdr, uint8_t Value) {
  EEPROM.write(EEAdr, Value);
}

void sendUdpData(byte Frame[], uint16_t Len) {
  uint32_t FrameCnt = 0;
  if (connectWiFi(Ssid, Password)) {
    udp.beginPacket(UDPDataHostIP, UDPDataPort);
    udp.write(Frame, Len);
    udp.endPacket();
    FrameCnt = Frame[22] << 24 | Frame[23] << 16 | Frame[24] << 8 | Frame[25];
    Log("data frame " + String(FrameCnt) + " sent\n");
  }
}

void Log(String Message) {
  if (SerialLog != 0) Serial1.print(Message);
  if (UDPLog != 0) {
    if (connectWiFi(Ssid, Password)) {
      udp.beginPacket(UDPLogHostIP, UDPLogPort);
      udp.print(Message);
      udp.endPacket();
    }
  }
}

bool connectWiFi(String Ssid, String Password) {
  uint8_t Attampt = 0;
  if (WiFi.status() != WL_CONNECTED) {
    Serial1.printf("connecting % s ", Ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(Ssid, Password);
    while (true) {
      if (WiFi.status() == WL_CONNECTED) break;
      else {
        Attampt += 1;
        Serial1.print(".");
        if (Attampt >= 15) break;
        delay(500);
      }
    }
    Serial1.println();
    if (WiFi.status() == WL_CONNECTED) {
      Log("connected, IP ");
      Log(WiFi.localIP().toString());
      Rssi = WiFi.RSSI();
      sprintf(Item, ", RSSI % d dB\n", Rssi);
      Log(String(Item));
      return true;
    }
    else {
      Serial1.printf("connecting % s failed", Ssid);
      Serial1.print(", status ");
      Serial1.println(WiFi.status() + "");
      return false;
    }
  }
  else return true;
}

void handleWebRequest() {
  String Response;
  if (SettingsServer.args() == 10) {
    //store configuration pressed
    Ssid = SettingsServer.arg(0);
    Password = SettingsServer.arg(1);
    UDPDataHost = SettingsServer.arg(2);
    UDPDataPort = SettingsServer.arg(3).toInt();
    SerialLog = SettingsServer.arg(4).toInt();
    UDPLog = SettingsServer.arg(5).toInt();
    UDPLogHost = SettingsServer.arg(6);
    UDPLogPort = SettingsServer.arg(7).toInt();
    Response = "";
    if (Ssid.length() == 0 || Ssid.length() > 32) Response = "SSID invalid";
    else if (Password.length() < 8 || Password.length() > 63) Response = "Password invalid";
    else if (!UDPDataHostIP.fromString(UDPDataHost)) Response = "UDPDataHost IP invalid";
    else if (!UDPLogHostIP.fromString(UDPLogHost)) Response = "UDPLogHost IP invalid";
    else {
      //send configutation info
      HttpText = WebHeader + InfoForm;
      replaceHttpText();
      SettingsServer.send(200, "text / html", HttpText);
      yield();
      writeConfig();
      Log("ESP8266 restart in 10 sec ...\n");
      delay(10000);
      ESP.restart();
    }
  }
  //send  configuration form
  HttpText = WebHeader + SettingsForm;
  if (Response == "") HttpText.replace("#response#", "");
  else HttpText.replace("#response#", "<font color=\"red\"><b>" + Response + "</b></font><br><br>");
  replaceHttpText();
  SettingsServer.send(200, "text/html", HttpText);
}

void replaceHttpText() {
  HttpText.replace("#ssid#", Ssid);
  HttpText.replace("#password#", Password);
  HttpText.replace("#udp_data_host#", UDPDataHost);
  HttpText.replace("#udp_data_port#", String(UDPDataPort));
  HttpText.replace("#serial_log#", String(SerialLog));
  HttpText.replace("#udp_log#", String(UDPLog));
  HttpText.replace("#udp_log_host#", UDPLogHost);
  HttpText.replace("#udp_log_port#", String(UDPLogPort));
}

void setup(void) {
  //init serial connections
  Serial.begin(2400, SERIAL_8E1);
  Serial1.begin(115200);
  delay(300);

  //print prg infos
  Serial1.println();
  Serial1.print(PrgName);
  Serial1.print("  ");
  Serial1.println(PrgVersion);

  //read configuration
  EEPROM.begin(EEPROM_Size);
  readConfig();

  //init WiFi
  if (connectWiFi(Ssid, Password)) isAP = false;
  else {
    //start access point
    Serial1.println("starting access point ...");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APSsid, APPassword);
    Serial1.print("access point '");
    Serial1.print(APSsid);
    Serial1.println("' started (" + WiFi.softAPIP().toString() + ")");
    isAP = true;
  }
  //start settings server
  Serial1.println("starting web settings server ...");
  SettingsServer.begin();
  SettingsServer.on("/", handleWebRequest);
  sprintf(Item, "web settings server running (http://%s.local/)\n", Host);
  Log(String(Item));

  //no more actions in ap mode
  if (isAP) return;

  //start update server
  Log("starting web update server ...\n");
  MDNS.begin(Host);
  WebUpdater.setup(&UpdateServer);
  UpdateServer.begin();
  MDNS.addService("http", "tcp", 80);
  sprintf(Item, "web update server running (http://%s.local:81/update)\n", Host);
  Log(String(Item));
}

void loop(void) {
  uint8_t Frame[FrameLength];
  uint16_t i;
  const uint32_t TimeoutMillis = 100;
  uint32_t LastReceiveMillis;

  //handle web request for settings
  SettingsServer.handleClient();

  //no more actions in ap mode
  if (isAP) return;

  //handle web update
  UpdateServer.handleClient();
  MDNS.update();

  //if 282 bytes from m-bus comesin, send it over udp
  if (Serial.available()) {
    LastReceiveMillis = millis();
    i = 0;
    while (true) {
      if (Serial.available()) {
        Frame[i] = Serial.read();
        i += 1;
        LastReceiveMillis = millis();
      }
      if (millis() - LastReceiveMillis >= TimeoutMillis) {
        //reached end of frame
        if (i == FrameLength) sendUdpData(Frame, FrameLength);
        break;
      }
    }
  }

  //reduce power
  delay(1);
}
