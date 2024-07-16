/*********+
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-websocket-server-sensor/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

//Mahmoud was here :)

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h>

#include <ESP32Servo.h>

#define pinSuciedad 33
#define pinCO2 34
#define pinFiltro 18
int read_suciedad;
int read_gas;

Servo servo;
int valServo;

float pGas;

const float K_O2 = 1.3e-3; // Para el oxígeno a 25 °C en mol/(L·atm)
float P_total = 101.3;
float P_detected; //Gases detectados
float P_O2;
float C_O2; // Concentración de oxígeno mol/L


const char* ssid = "UTCN-Guest";
const char* password = "utcluj.ro";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


String getSensorReadings(){
  read_suciedad = analogRead(pinSuciedad);
  read_gas = analogRead(pinCO2);
  readings["suciedad"] = String(read_suciedad);
  readings["co2"] = String(concentracion_liquid());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void activarServoFuncion() {
  for (int pos = 80; pos >= 40; pos -= 1) { 
    servo.write(pos);              
    delay(60);   
  }
}

float concentracion_liquid(){
  P_detected = read_gas; 
  P_O2 = P_total - P_detected;
  if (P_O2 < 0){P_O2 *= -1;}
  //(1 atm = 101.325 kPa)
  P_O2 = P_O2 / 10.1325;
  
  // Concentración de oxigeno disuelto (Ley de Henry)
  C_O2 = K_O2 * P_O2;
  return C_O2;
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    if (strcmp((char*)data, "getReadings") == 0) {
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    } else {
      if (strcmp((char*)data, "activarServo") == 0) {
        activarServoFuncion();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
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

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
} 

void setup() {
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initWebSocket();

  // Pin servo
  servo.attach(22);
  
  pinMode(pinFiltro, OUTPUT);
  
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  Serial.println("Cosas iniciadas");

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.println(sensorReadings);
    if (read_suciedad > -30){ //CAMBIAR
      Serial.println("Demasiadas partículas");
      delay(1000);
      digitalWrite(pinFiltro,LOW);
      delay(1000);
    }else{
      delay(1000);
      digitalWrite(pinFiltro,HIGH);
      delay(1000);
    }
    notifyClients(sensorReadings);
    lastTime = millis();
  }
  
  ws.cleanupClients();
}