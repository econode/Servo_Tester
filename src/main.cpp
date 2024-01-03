#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <ESP32Servo.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include <cstring>
#include <string>
#include <esp_task_wdt.h>
#include <defaults.h>


// Servo Object
Servo myservo;


// Forward declerations
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void wsOnEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;
IPAddress ip_address;
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Buffer space for JSON data processing
StaticJsonDocument<2048> json_rx;
StaticJsonDocument<2048> json_tx;

struct servoState_t {
  bool hasFormUpdated = false;


};
servoState_t volatile servoState;

String templateProcessor(const String& paramName){
  // Process template
  if(paramName == "wsGatewayAddr"){
    // normally ws://192.168.4.1/ws
    return String("ws://") + ip_address.toString() + String("/ws");
  }
  if( paramName == "hasFormUpdated" ){
    String hasFormUpdated = servoState.hasFormUpdated?"true":"false";
    servoState.hasFormUpdated = false;
    return hasFormUpdated;
  }
  // We didn't find anything return empty string
  return String();
}

void processFormParamater( const String& fieldName, const String& fieldValue ){
  Serial.printf("field: %s = %s\n",fieldName,fieldValue);
  uint16_t intFieldValue = (uint16_t) fieldValue.toInt();
  /*
  if( fieldName == "servoUpDegrees" ){
    shifterState.upDegrees = intFieldValue;
  }
  */
}


void server_routes(){
  // https://github.com/me-no-dev/ESPAsyncWebServer#handlers-and-how-do-they-work
  server.onNotFound([](AsyncWebServerRequest *request){
    request->redirect("/index.html");
  });

  server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest * request){
    if( request->method() == HTTP_POST ){
      int paramCount = request->params();
      if( paramCount> 0 ) servoState.hasFormUpdated = true;
      Serial.printf("Number of params %d\n",paramCount);
      for( int i=0; i<paramCount; i++ ){
        AsyncWebParameter * param = request->getParam(i);
        processFormParamater(param->name(), param->value());
      }
    }
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
  }
}

void setup(){
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.begin(115200);

  // Start file system  
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(PIN_SERVO, SERVO_MINIMUM_PULSE_WIDTH, SERVO_MAXIMUM_PULSE_WIDTH); // 500 - 2400 for 9G SG90

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
