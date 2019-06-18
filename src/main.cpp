#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <SPIFFSEditor.h>
#include <PubSubClient.h>
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

#define TOKEN "qLyBaqukV6MBJoue3Pfc"

#define SECSINDAY 86400UL
#define SECSINHOUR 3600UL
#define SECSINMIN 60UL
#define VERSION 6000
#define LED_BLUE 2
#define TX_PIN 12
#define RX_PIN 27

const char *thingsboardServer PROGMEM = "192.168.1.16";
const char *fileDefaults PROGMEM = "/defaults.json"; //Default Settings File
const char *fileSettings PROGMEM = "/settings.json"; //Active Settings File
const char *fileBackup PROGMEM = "/settings.bak";    //Backup Settings File
const char *boxBasename PROGMEM = "SWITCH_";
const char *msgTruncated PROGMEM = "Truncated";
const char *msgFirmware PROGMEM = "Firmware";
const char *Space PROGMEM = " ";
const char *Tab PROGMEM = "\t";
const uint8_t LENGTH = 26;

const uint8_t setBSSID[6] = {0xE4, 0xF0, 0x42, 0xE0, 0x09, 0x2F}; //Master - Game Room
// const uint8_t setBSSID[6] = {0xE4, 0xF0, 0x42, 0xE7, 0x6F, 0x29}; // Mesh - DnK Bedroom
char boxID[40];
char setWIFI[30];
char setPASS[30];
char setNAME[30];
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
WiFiClient wifiClient;
PubSubClient client(wifiClient);
AsyncWebServer server(80);
NTPClient timeClient(ntpUDP, "192.168.1.16");

RCSwitch mySwitch_tx = RCSwitch();

void processSwitchUni(uint32_t &txCODE)
{
  digitalWrite(LED_BLUE, HIGH);
  mySwitch_tx.enableTransmit(TX_PIN);
  mySwitch_tx.send(txCODE, LENGTH);
  mySwitch_tx.disableTransmit();
  digitalWrite(LED_BLUE, LOW);
}

void restartSwitch(void)
{
  ESP.restart();
}

void updateSettings(String &txID)
{
  StaticJsonBuffer<3000> jsonBuffer;
  File f = SPIFFS.open(fileSettings, "r");
  JsonObject &root = jsonBuffer.parseObject(f);
  f.close();
  if (root.success())
  {
    JsonArray &switches = root["switches"];
    int i = 0;
    if (txID.charAt(0) == 'B')
      i += 4;
    else if (txID.charAt(0) == 'C')
      i += 8;
    i += (txID.charAt(1) - '0');
    i -= 1;
    if ((i > 0) && ((i + 1) % 4 == 0))
    {
      uint8_t status = 0;
      if (switches[i]["status"] == 0)
        status++;
      switches[i]["status"] = status;
      for (int p = 1; p < 4; p++)
      {
        switches[i - p]["status"] = status;
      }
    }
    else
    {
      if (switches[i]["status"] == 0)
        switches[i]["status"] = 1;
      else
        switches[i]["status"] = 0;
    }
  }
  f = SPIFFS.open(fileSettings, "w");
  root.prettyPrintTo(f);
  f.close();
  jsonBuffer.clear();
}

void reconnectThingsboard()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Connecting to ThingsBoard node ...");
    // Attempt to connect (clientId, username, password)
    if (client.connect("Home Switches", TOKEN, NULL))
    {
      Serial.println("[DONE]");
    }
    else
    {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(client.state());
      Serial.println(" : retrying in 5 seconds]");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void ArduinoOTAStuff(void)
{
  String hostNAME = boxID;
  String macADDRESS = WiFi.macAddress();
  while (macADDRESS.indexOf(':') > 0)
    macADDRESS.remove(macADDRESS.indexOf(':'), 1);
  macADDRESS.remove(0, 6);
  hostNAME += macADDRESS;
  ArduinoOTA.setHostname(hostNAME.c_str());
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

int16_t setTZ(int8_t offSET)
{
  uint16_t rTZ = 3600 * offSET;
  return rTZ;
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

  server.on("/RESTART", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200);
    restartSwitch();
  });

  server.on("/FLIPSWITCH", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebParameter *p = request->getParam(0);
    String txID = p->value();
    p = request->getParam(1);
    uint32_t txCODE = p->value().toInt();
    if (txCODE > 0)
    {
      // Serial.println(txID + Space + txCODE);
      // processSwitchUni(sPARAM);
      processSwitchUni(txCODE);
      updateSettings(txID);
      request->send(200, "text/plain", "Switched " + txID);
    }
    else
      request->send(500, "text/plain", "Error while switching " + txID);
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html_gz, sizeof(settings_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html);
    // request->send(response);
  });

  server.on("/SETTINGS", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/settings");
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/SAVESETTINGS", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &incoming = json.as<JsonObject>();
    if (incoming.success())
    {
      StaticJsonBuffer<3000> jsonBuffer;
      File f = SPIFFS.open(fileSettings, "r");
      JsonObject &root = jsonBuffer.parseObject(f);
      JsonObject &settings = root["settings"];
      f.close();
      if (root.success())
      {
        String tmpID;
        settings["wifi"] = incoming["wifi"];
        settings["pass"] = incoming["pass"];
        settings["name"] = incoming["name"];
        settings["zone"] = incoming["zone"];
        timeClient.setTimeOffset(setTZ(settings["zone"]));
        for (int i = 1; i <= 12; i++)
        {
          JsonArray &switches = root["switches"];
          JsonObject &oneSWITCH = switches[i - 1];
          if (i <= 4)
          {
            tmpID = "A";
            tmpID += String(i);
          }
          else if (i <= 8)
          {
            tmpID = "B";
            tmpID += String(i - 4);
          }
          else if (i <= 12)
          {
            tmpID = "C";
            tmpID += String(i - 8);
          }
          oneSWITCH["name"] = incoming[tmpID];
        }
        File f = SPIFFS.open(fileSettings, "w");
        root.prettyPrintTo(f);
        f.close();
        jsonBuffer.clear();
      }
      request->send(200, "text/plain", "Settings Saved");
    }
  });
  server.addHandler(handler);

  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    // AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", redirect_html_gz, sizeof(redirect_html_gz));
    // response->addHeader("Content-Encoding", "gzip");
    // request->send(response);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", settings_html);
    request->send(response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS)
      request->send(200);
    else
      request->send(404);
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,PUT,POST,DELETE,PATCH,OPTIONS");

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

  uint8_t tmpHOUR = currHOUR;

  if (tmpHOUR == 0)
    tmpHOUR = 12;
  else if (tmpHOUR >= 12)
    cAMPM = "P";
  if (tmpHOUR >= 13)
    tmpHOUR = tmpHOUR % 12;

  String strMonth = ntp_month < 10 ? "0" + String(ntp_month) : String(ntp_month);
  String strDay = ntp_date < 10 ? "0" + String(ntp_date) : String(ntp_date);
  String strHour = tmpHOUR < 10 ? "0" + String(tmpHOUR) : String(tmpHOUR);
  String strMinute = currMINUTE < 10 ? "0" + String(currMINUTE) : String(currMINUTE);
  String strSecond = currSECOND < 10 ? "0" + String(currSECOND) : String(currSECOND);

  lastTimeLong = String(ntp_year) + "/" + strMonth + "/" + strDay + "_" + strHour + ":" + strMinute + ":" + strSecond + cAMPM;
}

void loadSettings()
{
  StaticJsonBuffer<3000> jsonBuffer;
  File f = SPIFFS.open(fileSettings, "r");
  if (!f)
  {
    f = SPIFFS.open(fileDefaults, "r");
  }
  else
  {
    JsonObject &root = jsonBuffer.parseObject(f);
    f.close();
    if (root.success())
    {
      strncpy(setWIFI, root["settings"]["wifi"], sizeof(setWIFI));
      strncpy(setPASS, root["settings"]["pass"], sizeof(setPASS));
      strncpy(setNAME, root["settings"]["name"], sizeof(setNAME));
      setTIMEZONE = root["settings"]["zone"];
    }
  }
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
  }
}

void updateThingsboard()
{
  static uint8_t lastSECOND = 0;
  if (currSECOND != lastSECOND)
  {
    StaticJsonBuffer<3000> jsonBuffer;
    File f = SPIFFS.open(fileSettings, "r");
    JsonObject &root = jsonBuffer.parseObject(f);
    f.close();
    JsonArray &switches = root["switches"];
    String tmpMESSAGE;
    tmpMESSAGE.reserve(300);
    switches[0].printTo(tmpMESSAGE);
    client.publish("v1/devices/me/telemetry", tmpMESSAGE.c_str());
    jsonBuffer.clear();
    lastSECOND = currSECOND;
  }
}

void timerCheck()
{
  static uint8_t lastMINUTE = 0;
  static uint8_t lastSECOND = 0;
  if ((currMINUTE != lastMINUTE) && (currSECOND != lastSECOND))
  {
    uint32_t tmpCODE = 0;
    uint8_t tmpSTATUS = 0;
    String strHour = currHOUR < 10 ? "0" + String(currHOUR) : String(currHOUR);
    String strMinute = currMINUTE < 10 ? "0" + String(currMINUTE) : String(currMINUTE);
    String currTIME = String(strHour) + ":" + String(strMinute);
    String timeON = "";
    String timeOFF = "";
    StaticJsonBuffer<3000> jsonBuffer;
    File f = SPIFFS.open(fileSettings, "r");
    JsonObject &root = jsonBuffer.parseObject(f);
    f.close();
    JsonArray &switches = root["switches"];
    for (int i = 0; i < 12; i++)
    {
      JsonObject &oneSWITCH = switches[i];
      JsonObject &switchTIMER = oneSWITCH["timers"];
      if (switchTIMER.success())
      {
        timeON = switchTIMER["timeon"].as<String>();
        timeOFF = switchTIMER["timeoff"].as<String>();
        if (timeON == currTIME)
        {
          tmpCODE = uint32_t(oneSWITCH["on"]);
          tmpSTATUS = 1;
        }
        else if (timeOFF == currTIME)
        {
          tmpCODE = uint32_t(oneSWITCH["off"]);
          tmpSTATUS = 0;
        }
        if (tmpCODE != 0)
        {
          processSwitchUni(tmpCODE);
          oneSWITCH["status"] = String(tmpSTATUS);
          File f = SPIFFS.open(fileSettings, "w");
          root.prettyPrintTo(f);
          f.close();
        }
      }
      lastMINUTE = currMINUTE;
      lastSECOND = currSECOND;
    }
    jsonBuffer.clear();
  }
}

void wifiCheck()
{
  static uint8_t reconnectionATTEMPTS = 0;
  static uint8_t lastMINUTE = 0;
  if (currMINUTE != lastMINUTE)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.reconnect();
      reconnectionATTEMPTS++;
      delay(500);
    }
    else if (reconnectionATTEMPTS >= 5)
    {
      restartSwitch();
    }
    lastMINUTE = currMINUTE;
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    continue;

  SPIFFS.begin(true);

  pinMode(LED_BLUE, OUTPUT);
  // mySwitch_tx.enableTransmit(TX_PIN);
  // mySwitch_tx.setRepeatTransmit(3);
  // mySwitch_rx.enableReceive(RX_PIN);

  loadSettings();

  WiFi.begin(setWIFI, setPASS, 0, setBSSID);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  timeClient.setTimeOffset(setTZ(setTIMEZONE));
  timeClient.begin();

  client.setServer(thingsboardServer, 1883);

  if (!client.connected())
  {
    reconnectThingsboard();
  }

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
}

void loop()
{
  //ledBlueFlash();
  timeUpdate();
  timerCheck();
  wifiCheck();
  updateThingsboard();
  // button.tick();
  // receiveCode();
  ArduinoOTA.handle();
}