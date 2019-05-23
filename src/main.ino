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
#include "output.h"

#define SECSINDAY 86400UL
#define SECSINHOUR 3600UL
#define SECSINMIN 60UL
#define VERSION 5200
#define LED_BLUE 2
#define TX_PIN 12
#define RX_PIN 27

// const uint32_t codesALL[3][8] = {
//     {35885076, 19107860, 35883028, 19105812, 35887124, 19109908, 34832404, 18055188},
//     {35754004, 18976788, 35751956, 18974740, 35756052, 18978836, 34701332, 17924116},
//     {35819540, 19042324, 35817492, 19040276, 35821588, 19044372, 34766868, 17989652}};

const uint32_t codesON[3][4] = {
    {35885076, 35883028, 35887124, 34832404},
    {35754004, 35751956, 35756052, 34701332},
    {35819540, 35817492, 35821588, 34766868}};

const uint32_t codesOFF[3][4] = {
    {19107860, 19105812, 19109908, 18055188},
    {18976788, 18974740, 18978836, 17924116},
    {19042324, 19040276, 19044372, 17989652}};

uint8_t wSTATUS[3][4];
char wDESCRIPTION[3][4][15];

const char *fileSettings PROGMEM = "/settings.json"; //Settings File
const char *fileDefaults PROGMEM = "/defaults.json"; //Default Settings File
const char *fileStatus PROGMEM = "/status.json";     //Settings File
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
char setTIMEZONE[10] = "CENTRAL";

uint32_t currEPOCH = 0;
uint16_t currDAY = 0;
uint8_t currHOUR = 0;
uint8_t currMINUTE = 0;
uint8_t currSECOND = 0;

String lastTimeLong;
String switchRESPONSE;

WiFiUDP ntpUDP;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
// OneButton button(GPIO_BUT, true);
NTPClient timeClient(ntpUDP, "time.google.com");

RCSwitch mySwitch_tx = RCSwitch();
// RCSwitch mySwitch_rx = RCSwitch();

void setup()
{
  switchRESPONSE.reserve(30);
  Serial.begin(115200);
  while (!Serial)
    continue;

  SPIFFS.begin(true);

  pinMode(LED_BLUE, OUTPUT);
  mySwitch_tx.enableTransmit(TX_PIN);
  // mySwitch_tx.setRepeatTransmit(3);
  // mySwitch_rx.enableReceive(RX_PIN);

  //loadSettings();

  WiFi.begin(setWIFI, setPASS, 0, setBSSID);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  timeClient.setTimeOffset(setTZOffset(setTIMEZONE));
  timeClient.begin();
  while (timeClient.forceUpdate() != true)
    delay(500);

  strcpy(boxID, boxBasename);
  strncat(boxID, setNAME, 33);

  MDNS.begin(boxID);
  MDNS.addService("http", "tcp", 80);

  ArduinoOTAStuff();

  serverStuff();

  timeUpdate();

  strncpy(startTIME, lastTimeLong.c_str(), lastTimeLong.length());

  saveSettings();
  Serial.println(String(boxID) + Space + String(startTIME));
  //ledBlueOff();
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

    // Disable client connections
    ws.enable(false);

    // Advertise connected clients what's going on
    ws.textAll("OTA Update Started");

    // Close them
    ws.closeAll();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
  });

  ArduinoOTA.begin();
}

void onRequest(AsyncWebServerRequest *request)
{
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
  //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  //Handle upload
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  //Handle WebSocket event
}

//Files
void appendOutput(const char *sFile, const String &sStuff)
{
  File f = SPIFFS.open(sFile, "a");
  if (f.size() > 512)
  {
    f.close();
    f = SPIFFS.open(sFile, "w");
    f.println(msgTruncated);
  }
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
  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  },onUpload);

  // attach filesystem root at URL /fs
  server.serveStatic("/fs", SPIFFS, "/");

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
    request->send(response);
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html, sizeof(index_html));
    // response->addHeader("Content-Encoding", "gzip");
    // request->send(response);
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  // server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico, sizeof(favicon_ico));
  //   response->addHeader("Content-Encoding", "gzip");
  //   request->send(response);
  // });

  // server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
  //   // request->send(response);
  //   AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html_gz, sizeof(settings_html_gz));
  //   response->addHeader("Content-Encoding", "gzip");
  //   request->send(response);
  // });

  // server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->redirect("/settings");
  // });

  server.on("/defaults.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/defaults.json", "application/json");
  });

  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/settings.json", "application/json");
  });

  server.on("/RESTART", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    restartSwitch();
  });

  server.on("/switchu", HTTP_GET, [](AsyncWebServerRequest *request) {
    int params = request->params();
    if (params == 3)
    {
      AsyncWebParameter *p = request->getParam(0);
      // Serial.printf("HEADER[%s]: %s\n", p1->name().c_str(), p1->value().c_str());
      uint8_t rCHANNEL = p->value().toInt();
      p = request->getParam(1);
      // Serial.printf("HEADER[%s]: %s\n", p2->name().c_str(), p2->value().c_str());
      uint8_t rINDEX = p->value().toInt();
      p = request->getParam(2);
      // Serial.printf("HEADER[%s]: %s\n", p3->name().c_str(), p3->value().c_str());
      bool rSTATUS = p->value().toInt();
      processSwitchUni(rCHANNEL, rINDEX, rSTATUS);
    }
    request->redirect("/");
  });

  server.on("/JSONSTART", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("Server", "ESP Async Web Server");
    JsonObject &root = response->getRoot();
    String sVERSION = String(VERSION) + Space + startTIME;
    root["title"] = setNAME;
    root["version"] = sVERSION;
    root["lastTimeLong"] = lastTimeLong;
    root["status"] = wSTATUS;
    root["description"] = wDESCRIPTION;
    response->setLength();
    request->send(response);
  });

  server.on("/JSONUPDATE", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    response->addHeader("Server", "ESP Async Web Server");
    JsonObject &root = response->getRoot();
    root["lastTimeLong"] = lastTimeLong;
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
    root["status"] = wSTATUS;
    root["description"] = wDESCRIPTION;
    response->setLength();
    request->send(response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      request->send(404);
    }
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

int16_t setTZOffset(const char *tTZ)
{
  uint16_t rTZ = 14400;
  if (strcmp(tTZ, "CENTRAL") == 0)
    rTZ += SECSINHOUR;
  else if (strcmp(tTZ, "MOUNTAIN") == 0)
    rTZ += (2 * SECSINHOUR);
  else if (strcmp(tTZ, "PACIFIC") == 0)
    rTZ += (3 * SECSINHOUR);
  return -rTZ;
}

void loadSettings()
{
  StaticJsonBuffer<500> jsonBuffer;
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
    strncpy(setTIMEZONE, root["zone"], sizeof(setTIMEZONE));
  }
  jsonBuffer.clear();
}

void saveSettings(void)
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["wifi"] = setWIFI;
  root["pass"] = setPASS;
  root["name"] = setNAME;
  root["zone"] = setTIMEZONE;
  File f = SPIFFS.open(fileSettings, "w");
  root.printTo(f);
  f.close();
  jsonBuffer.clear();
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
    //Serial.println(String(boxID) + Space + lastTimeLong);
  }
}

void restartSwitch(void)
{
  ESP.restart();
}

void turnOn(uint8_t idCHANNEL, uint8_t idINDEX)
{
  mySwitch_tx.send(codesON[idCHANNEL][idINDEX], LENGTH);
  wSTATUS[idCHANNEL][idINDEX] = !wSTATUS[idCHANNEL][idINDEX];
  delay(100);
}

void turnOff(uint8_t idCHANNEL, uint8_t idINDEX)
{
  mySwitch_tx.send(codesOFF[idCHANNEL][idINDEX], LENGTH);
  wSTATUS[idCHANNEL][idINDEX] = !wSTATUS[idCHANNEL][idINDEX];
  delay(100);
}

void processSwitchUni(uint8_t channel, uint8_t index, bool state)
{
  if (state)
    turnOn(channel, index);
  else
    turnOff(channel, index);
}