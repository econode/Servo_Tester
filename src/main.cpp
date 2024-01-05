#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <ESP32Servo.h>
#include <Preferences.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <cstring>
#include <string>
#include <esp_task_wdt.h>
#include <defaults.h>


// Servo Object
Servo myservo;

// Used to track structure of Preferences name space.
#define PREFERENCES_NAME_SPACE "SERVO"
#define PREFERENCES_VERSION 2
Preferences preferences;


struct servoConfig_t {
  const bool defaultServoEnabled = SERVO_ENABLED;
  const uint16_t defaultPinServo = PIN_SERVO;
  const uint16_t defaultFreq = SERVO_FREQUENCY_HZ;
  const uint16_t defaultPulseMin = SERVO_MINIMUM_PULSE_WIDTH;
  const uint16_t defaultPulseMax = SERVO_MAXIMUM_PULSE_WIDTH;
  const uint16_t defaultSettleTime = SERVO_SETTLE_TIME;

  uint8_t pinServo;
  bool servoEnabled;
  uint16_t freq;
  uint16_t pulseMin;
  uint16_t pulseMax;
  uint16_t settleTime;
};
servoConfig_t servoConfig;


struct servoState_t {
  // const uint16_t defaultUpDegrees = DEFAULT_UpDegrees;
  uint16_t upDegrees;
};
servoState_t volatile servoState;


// Forward declerations
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void sweepStart(void);
void updateServo(void);
void servoEnable(void);
void preferencesWrite();
void preferencesRead();
void servoEnable();
void servoDisable();
void ws_updateServo();
void ws_servoEnable();
void ws_sweepStart();

void preferencesWrite(){
  preferences.putUShort("version", PREFERENCES_VERSION);
  preferences.putBool("servoEnabled", servoConfig.servoEnabled );
  preferences.putUShort("pinServo", servoConfig.pinServo );
  preferences.putUShort("freq", servoConfig.freq );
  preferences.putUShort("pulseMin", servoConfig.pulseMin );
  preferences.putUShort("pulseMax", servoConfig.pulseMax );
  preferences.putUShort("settleTime", servoConfig.settleTime );
}

void preferencesRead(){
  if( preferences.isKey("version") == false || preferences.getUShort("version") != PREFERENCES_VERSION ){
    // Empty name space or name space structure has changed, push defaults
    servoConfig.servoEnabled = servoConfig.defaultServoEnabled;
    servoConfig.pinServo = servoConfig.defaultPinServo;
    servoConfig.freq = servoConfig.defaultFreq;
    servoConfig.pulseMin = servoConfig.defaultPulseMin;
    servoConfig.pulseMax = servoConfig.defaultPulseMax;
    servoConfig.settleTime = servoConfig.defaultSettleTime;
    preferencesWrite();
    return;
  }
  servoConfig.servoEnabled = preferences.getBool("servoEnabled");
  servoConfig.pinServo = preferences.getUShort("pinServo");
  servoConfig.freq = preferences.getUShort("freq");
  servoConfig.pulseMin = preferences.getUShort("pulseMin");
  servoConfig.pulseMax = preferences.getUShort("pulseMax");
  servoConfig.settleTime = preferences.getUShort("settleTime");
}

const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;
IPAddress ip_address;
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Buffer space for JSON data processing
StaticJsonDocument<2048> json_rx;
StaticJsonDocument<2048> json_tx;

String templateProcessor(const String& paramName){
  // Process template
  if(paramName == "wsGatewayAddr"){
    // normally ws://192.168.4.1/ws
    return String("ws://") + ip_address.toString() + String("/ws");
  }
  if( paramName == "servoFreq" ){
    return String(servoConfig.freq);
  }
  if( paramName == "servoMinPulse" ){
    return String(servoConfig.pulseMin);
  }
  if( paramName == "servoMaxPulse" ){
    return String(servoConfig.pulseMax);
  }
  if( paramName == "servoSettleTime" ){
    return String(servoConfig.settleTime);
  }
  if( paramName == "servoEnabled" ){
    return String(servoConfig.servoEnabled);
  }
  if( paramName == "servoGpioPin" ){
    return String(servoConfig.pinServo);
  }
  // We didn't find anything return empty string
  return String();
}


void server_routes(){
  // https://github.com/me-no-dev/ESPAsyncWebServer#handlers-and-how-do-they-work
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/index.html");
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request){
    // Send response page using index.html via templateProcessor
    Serial.printf("Send index.html page\n");
    request->send(LittleFS,"/html/index.html","text/html",false,templateProcessor);
  });

  // Serve static files
  server.serveStatic("/", LittleFS,"/html/");

    // Websockets init.
  ws.onEvent(wsOnEvent);
  server.addHandler(&ws);
}

void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
 void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  char buffer[1000];
  
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.printf("WS RX rawstring:%s\n",data);
    DeserializationError err = deserializeJson(json_rx,data);
    if (err) {
      Serial.printf("ERROR: deserializeJson() failed with: %s\n",err.c_str() );
      Serial.printf("Can not process websocket request\n");
      return;
    }
    const char* message = json_rx["message"];
    Serial.printf("WS RX->message:%s\n",message);

    // Route websocket message based on messageType
    if( strcmp(message,"updateServo") == 0 ){
      ws_updateServo();
    }
    if( strcmp(message,"servoEnable") == 0 ){
      ws_servoEnable();
    }
    if( strcmp(message,"sweepStart") == 0 ){
      ws_sweepStart();
    }
  }
}

void ws_sweepStart(void){
  char buffer[1000];
  uint16_t SweepStart = json_rx["SweepStart"];
  uint16_t SweepEnd = json_rx["SweepEnd"];
  uint16_t SweepDelay = json_rx["SweepDelay"];
  uint16_t ServoEnabled = json_rx["ServoEnabled"];
  Serial.printf("Sweep start:%d SweepEnd:%d SweepDelay:%d ServoEnabled:%d\n",SweepStart,SweepEnd,SweepDelay,ServoEnabled);
  json_tx.clear();
  json_tx["message"] = "confirmSweep";
  json_tx["SweepRun"] = 1;
  json_tx["SweepStart"] = SweepStart;
  json_tx["SweepEnd"] = SweepEnd;
  json_tx["SweepDelay"] = SweepDelay;
  json_tx["ServoEnabled"] = ServoEnabled;
  size_t lenJson = serializeJson(json_tx, buffer);
  ws.textAll(buffer,lenJson);
  Serial.printf("WS TX-> JSON %s\n",buffer);
}

void ws_updateServo(void){
  char buffer[1000];
  // WS RX rawstring:{"message":"updateServo","ServoFrequency":50,"ServoMinPulse":500,"ServoMaxPulse":2500,"ServoEnabled":1}
  uint16_t ServoFrequency = json_rx["ServoFrequency"];
  uint16_t ServoMinPulse = json_rx["ServoMinPulse"];
  uint16_t ServoMaxPulse = json_rx["ServoMaxPulse"];
  uint16_t ServoEnabled = json_rx["ServoEnabled"];
  uint16_t ServoSettleTime = json_rx["ServoSettleTime"];
  servoConfig.servoEnabled = ServoEnabled;
  servoConfig.freq = ServoFrequency;
  servoConfig.pulseMin = ServoMinPulse;
  servoConfig.pulseMax = ServoMaxPulse;
  servoConfig.settleTime = ServoSettleTime;
  preferencesWrite();
  Serial.printf("ServoFrequency:%d ServoMinPulse:%d ServoMaxPulse:%d ServoSettleTime:%d ServoEnabled:%d\n",ServoFrequency,ServoMinPulse,ServoMaxPulse,ServoSettleTime,ServoEnabled);
  json_tx.clear();
  json_tx["message"] = "confirmUpdate";
  json_tx["ServoFrequency"] = ServoFrequency;
  json_tx["ServoMinPulse"] = ServoMinPulse;
  json_tx["ServoMaxPulse"] = ServoMaxPulse;
  json_tx["ServoEnabled"] = ServoEnabled;
  json_tx["ServoSettleTime"] = ServoSettleTime;
  size_t lenJson = serializeJson(json_tx, buffer);
  ws.textAll(buffer,lenJson);
  Serial.printf("WS TX-> JSON %s\n",buffer);
}

void ws_servoEnable(void){
  uint16_t ServoEnabled = json_rx["ServoEnabled"];
  Serial.printf("ServoEnabled:%d\n",ServoEnabled);
}

void servoEnable(){
  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  myservo.detach();
	myservo.setPeriodHertz(servoConfig.freq);
	myservo.attach(servoConfig.pinServo, servoConfig.pulseMin, servoConfig.pulseMax);
}
void servoDisable(){
  myservo.detach();
}

void setup(){
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.begin(115200);

  preferences.begin( PREFERENCES_NAME_SPACE, false);
  preferencesRead();

  // Start file system  
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  
  // myservo.write(shifterState.midPointDegrees);

  WiFi.softAP(wifi_ssid,wifi_password);
  ip_address = WiFi.softAPIP();
  Serial.println(ip_address);
  dnsServer.start(53, "*", ip_address );
  server_routes();

  // Start web server
  server.begin();
  Serial.print("Webserver started on IP:");
  Serial.println(ip_address);
}

void loop(){
  esp_task_wdt_reset();
  dnsServer.processNextRequest();
  delay(50);
}
