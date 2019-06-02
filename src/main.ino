#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <AsyncJson.h>
#include <NTPClient.h>
// #include <OneButton.h>
#include <RCSwitch.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "staticpages.h"
// #include "output.h"

#define SECSINDAY 86400UL
#define SECSINHOUR 3600UL
#define SECSINMIN 60UL
#define VERSION 5200
#define LED_BLUE 2
#define TX_PIN 12
#define RX_PIN 27

const uint32_t codesON[3][4] = {
    {35885076, 35883028, 35887124, 34832404},
    {35819540, 35817492, 35821588, 34766868},
    {35754004, 35751956, 35756052, 34701332}};

const uint32_t codesOFF[3][4] = {
    {19107860, 19105812, 19109908, 18055188},
    {19042324, 19040276, 19044372, 17989652},
    {18976788, 18974740, 18978836, 17924116}};

uint8_t wSTATUS[3][4];
String wNAMES[3][4] = {
    {"Air Purifier", "Unassigned", "Unassigned", "This Row"},
    {"Unassigned", "Unassigned", "Unassigned", "This Row"},
    {"Unassigned", "Front Patio", "Dining Room", "This Row"}};

const char *fileSettings PROGMEM = "/settings.json"; //Settings File
const char *fileDefaults PROGMEM = "/defaults.json"; //Default Settings File
const char *fileStatus PROGMEM = "/status.json";     //Settings File
const char *fileNames PROGMEM = "/names.json";       //Names File
const char *fileLog PROGMEM = "/log.txt";            //Log File
const char *boxBasename PROGMEM = "SWITCH_";
const char *msgTruncated PROGMEM = "Truncated";
const char *msgFirmware PROGMEM = "Firmware";
const char *Space PROGMEM = " ";
const char *Tab PROGMEM = "\t";
const uint8_t LENGTH = 26;

const uint8_t setBSSID[6] = {0xE4, 0xF0, 0x42, 0xE0, 0x09, 0x2F}; //Master - Game Room
// const uint8_t setBSSID[6] = {0xE4, 0xF0, 0x42, 0xE7, 0x6F, 0x29}; // Mesh - DnK Bedroom
char boxID[40];
char setWIFI[30] = "rbntx";
char setPASS[30] = "bekahbekah";
char setNAME[30] = "Testing";
char startTIME[21];

uint32_t currEPOCH = 0;
uint16_t currDAY = 0;
uint8_t currHOUR = 0;
uint8_t currMINUTE = 0;
uint8_t currSECOND = 0;
uint8_t timeTRIALS = 0;
int8_t setTIMEZONE = 0;

String lastTimeLong;

WiFiUDP ntpUDP;
AsyncWebServer server(80);
NTPClient timeClient(ntpUDP, "192.168.1.16");

RCSwitch mySwitch_tx = RCSwitch();
//RCSwitch mySwitch_rx = RCSwitch();

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  SPIFFS.begin(true);

  loadStatus();

  pinMode(LED_BLUE, OUTPUT);
  // mySwitch_tx.enableTransmit(TX_PIN);
  // mySwitch_tx.setRepeatTransmit(3);
  // mySwitch_rx.enableReceive(RX_PIN);

  //loadSettings();

  WiFi.begin(setWIFI, setPASS, 0, setBSSID);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  timeClient.setTimeOffset(setTZ(-5));
  timeClient.begin();

  while ((timeClient.forceUpdate() != true) && (timeTRIALS < 5))
  {
    timeTRIALS++;
    delay(500);
  }

  strcpy(boxID, boxBasename);
  strncat(boxID, setNAME, 33);

  MDNS.begin(boxID);
  MDNS.addService("http", "tcp", 80);

  ArduinoOTAStuff();

  serverStuff();

  timeUpdate();
  strncpy(startTIME, lastTimeLong.c_str(), lastTimeLong.length());
  //saveSettings();
  //saveStatus();
  //saveNames();
}

void loop()
{
  //ledBlueFlash();
  timeUpdate();
  // button.tick();
  // receiveCode();
  ArduinoOTA.handle();
}

void ArduinoOTAStuff(void)
{
  ArduinoOTA.setHostname(boxID);
  //ArduinoOTA.setPasswordHash("1571dc6e37d06f9445d05fc89d8baa8d");

  ArduinoOTA.onStart([]() {
    // Clean SPIFFS
    SPIFFS.end();

    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
  });

  ArduinoOTA.begin();
}

//Files
void appendOutput(const char *sFile, const String &sStuff)
{
  File f = SPIFFS.open(sFile, "a");
  // if (f.size() > 512)
  // {
  //   f.close();
  //   f = SPIFFS.open(sFile, "w");
  //   f.println(msgTruncated);
  // }
  f.println(sStuff);
  f.close();
}

void replaceOutput(const String &sFile, const String &sStuff)
{
  File f = SPIFFS.open(sFile, "w");
  f.println(sStuff);
  f.close();
}

void serverStuff(void)
{
  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    // request->send(response);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, sizeof(index_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico_gz, sizeof(favicon_ico_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.serveStatic("/defaults.json", SPIFFS, "/defaults.json");
  server.serveStatic("/settings.json", SPIFFS, "/settings.json");
  server.serveStatic("/status.json", SPIFFS, "/status.json");
  server.serveStatic("/names.json", SPIFFS, "/names.json");
  server.serveStatic("/SWITCHSTATUS", SPIFFS, "/status.json");
  server.serveStatic("/SWITCHNAMES", SPIFFS, "/names.json");
  server.serveStatic("/log.txt", SPIFFS, "/log.txt");

  server.on("/RESTART", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200);
    restartSwitch();
  });

  server.on("/switchu", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam(0);
    String sPARAM = p->value();
    processSwitchUni(sPARAM);
    saveStatus();
    request->send(200);
    request->redirect("/");
  });

  server.on("/JSONSTART", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("Server", "ESP Async Web Server");
    JsonObject &root = response->getRoot();
    String sVERSION = String(VERSION) + Space + startTIME;
    root["title"] = setNAME;
    root["version"] = sVERSION;
    response->setLength();
    request->send(response);
  });

  server.on("/JSONSETTINGS", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("Server", "ESP Async Web Server");
    JsonObject &root = response->getRoot();
    root["wifi"] = setWIFI;
    root["pass"] = setPASS;
    root["name"] = setNAME;
    response->setLength();
    request->send(response);
  });

  // server.on("/SAVESETTINGS", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send(200);
  //   saveStatus();
  // });

   server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html_gz, sizeof(settings_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html);
    // request->send(response);
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/SAVESETTINGS", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &root = json.as<JsonObject>();
    root.prettyPrintTo(Serial);
    File f = SPIFFS.open(fileNames, "w");
    root.prettyPrintTo(f);
    f.close();
    request->send(200, "text/plain", "Saved & Restarting");
    delay(1000);
    restartSwitch();
  });
  server.addHandler(handler);

    server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", redirect_html_gz, sizeof(redirect_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html);
    // request->send(response);
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();
}

void makeTimeLong(void)
{
  // 2019/01/01_12:00:00A
  String tmpLong;
  tmpLong.reserve(21);
  String cAMPM = "A";
  unsigned char month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint16_t ntp_date, ntp_month, leap_days;
  uint16_t temp_days = 0;
  uint16_t ntp_year, day_of_year;

  leap_days = 0;

  ntp_year = 1970 + (currDAY / 365);

  for (uint16_t i = 1972; i < ntp_year; i += 4)
    if (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))
      leap_days++;

  ntp_year = 1970 + ((currDAY - leap_days) / 365);
  day_of_year = ((currDAY - leap_days) % 365) + 1;

  if (((ntp_year % 4 == 0) && (ntp_year % 100 != 0)) || (ntp_year % 400 == 0))
    month_days[1] = 29;
  else
    month_days[1] = 28;

  for (ntp_month = 0; ntp_month <= 11; ntp_month++)
  {
    if (day_of_year <= temp_days)
      break;
    temp_days = temp_days + month_days[ntp_month];
  }

  temp_days = temp_days - month_days[ntp_month - 1];
  ntp_date = day_of_year - temp_days;

  if (currHOUR == 0)
    currHOUR = 12;
  else if (currHOUR >= 12)
    cAMPM = "P";
  if (currHOUR >= 13)
    currHOUR = currHOUR % 12;

  String strMonth = ntp_month < 10 ? "0" + String(ntp_month) : String(ntp_month);
  String strDay = ntp_date < 10 ? "0" + String(ntp_date) : String(ntp_date);
  String strHour = currHOUR < 10 ? "0" + String(currHOUR) : String(currHOUR);
  String strMinute = currMINUTE < 10 ? "0" + String(currMINUTE) : String(currMINUTE);
  String strSecond = currSECOND < 10 ? "0" + String(currSECOND) : String(currSECOND);

  lastTimeLong = String(ntp_year) + "/" + strMonth + "/" + strDay + "_" + strHour + ":" + strMinute + ":" + strSecond + cAMPM;
}

int16_t setTZ(int8_t offSET)
{
  uint16_t rTZ = 3600 * offSET;
  return rTZ;
}

void loadSettings()
{
  StaticJsonBuffer<300> jsonBuffer;
  File f = SPIFFS.open(fileSettings, "r");
  if (!f)
  {
    f = SPIFFS.open(fileDefaults, "r");
  }
  else
  {
    size_t size = f.size();
    std::unique_ptr<char[]> buf(new char[size]);
    f.readBytes(buf.get(), size);
    f.close();
    JsonObject &root = jsonBuffer.parseObject(buf.get());
    strncpy(setWIFI, root["wifi"], sizeof(setWIFI));
    strncpy(setPASS, root["pass"], sizeof(setPASS));
    strncpy(setNAME, root["name"], sizeof(setNAME));
    // setTIMEZONE, root["zone"];
    setTIMEZONE = -5;
  }
  jsonBuffer.clear();
}

void saveSettings(void)
{
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["wifi"] = setWIFI;
  root["pass"] = setPASS;
  root["name"] = setNAME;
  root["zone"] = setTIMEZONE;
  File f = SPIFFS.open(fileSettings, "w");
  root.prettyPrintTo(f);
  f.close();
  jsonBuffer.clear();
}

void saveStatus(void)
{
  uint8_t idCHANNEL = 65;
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  for (int i = 0; i < 3; i++)
  {
    for (int p = 0; p < 4; p++)
      root[char(idCHANNEL) + String(p)] = wSTATUS[i][p];
    idCHANNEL++;
  }
  File f = SPIFFS.open(fileStatus, "w");
  root.prettyPrintTo(f);
  f.close();
  jsonBuffer.clear();
}

void saveNames(void)
{
  uint8_t idCHANNEL = 65;
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  for (int i = 0; i < 3; i++)
  {
    for (int p = 0; p < 4; p++)
      root[char(idCHANNEL) + String(p)] = wNAMES[i][p];
    idCHANNEL++;
  }
  File f = SPIFFS.open(fileNames, "w");
  root.prettyPrintTo(f);
  f.close();
  jsonBuffer.clear();
}

void loadStatus(void)
{
  StaticJsonBuffer<300> jsonBuffer;
  char idCHANNEL = 'A';
  File f = SPIFFS.open(fileStatus, "r");
  if (f)
  {
    size_t size = f.size();
    std::unique_ptr<char[]> buf(new char[size]);
    f.readBytes(buf.get(), size);
    f.close();
    JsonObject &root = jsonBuffer.parseObject(buf.get());
    for (int i = 0; i < 3; i++)
      for (int p = 0; p < 4; p++)
      {
        if (i == 1)
          idCHANNEL = 'B';
        else if (i == 2)
          idCHANNEL = 'C';
        wSTATUS[i][p] = root[String(idCHANNEL) + String(p)];
      }
    f.close();
    jsonBuffer.clear();
  }
}

void timeUpdate(void)
{
  static uint8_t lastSEC = 0;
  currSECOND = timeClient.getSeconds();
  if (currSECOND != lastSEC)
  {
    currEPOCH = timeClient.getEpochTime();
    currDAY = currEPOCH / SECSINDAY;
    currHOUR = currEPOCH % SECSINDAY / SECSINHOUR;
    currMINUTE = currEPOCH % SECSINDAY % SECSINHOUR / SECSINMIN;
    lastSEC = currSECOND;
    makeTimeLong();
  }
}

void restartSwitch(void)
{
  ESP.restart();
}

void processSwitchUni(String sPARAM)
{
  uint8_t idCHANNEL, idINDEX, idSTATUS;
  mySwitch_tx.enableTransmit(TX_PIN);
  idCHANNEL = sPARAM.charAt(0) - 'A';
  idINDEX = sPARAM.charAt(1) - '0';
  idSTATUS = sPARAM.charAt(2) - '0';
  if (idSTATUS == 1)
  {
    if (idINDEX == 3)
      for (int tINDEX = 0; tINDEX < 3; tINDEX++)
        wSTATUS[idCHANNEL][tINDEX] = idSTATUS;
    mySwitch_tx.send(codesON[idCHANNEL][idINDEX], LENGTH);
    wSTATUS[idCHANNEL][idINDEX] = idSTATUS;
  }
  else
  {
    if (idINDEX == 3)
      for (int tINDEX = 0; tINDEX < 3; tINDEX++)
        wSTATUS[idCHANNEL][tINDEX] = idSTATUS;
    mySwitch_tx.send(codesOFF[idCHANNEL][idINDEX], LENGTH);
    wSTATUS[idCHANNEL][idINDEX] = idSTATUS;
  }
  mySwitch_tx.disableTransmit();
}