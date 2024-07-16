/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-websocket-server-sensor/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include "base64.h"

#include <ESP32Servo.h>

#define pinSuciedad 36
#define pinCO2 32
#define pinFiltro 4


String IP_CAMARA = "http://192.168.43.152/";

Servo servo;
int valServo;

HTTPClient http;

// Replace with your network credentials

const char* ssid = "Gorrionsss";
const char* password = "angelchupapito";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;
unsigned long camaraDelay = 5000;


// Get Sensor Readings and return JSON object
String getSensorReadings(){
  // Aquí tenemos que leer del sensor de suciedad
  readings["suciedad"] = String(analogRead(pinSuciedad));
  readings["co2"] = String(analogRead(pinCO2));
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void activarServoFuncion() {
  for (int pos = 50; pos >= 10; pos -= 1) { 
    servo.write(pos);              
    delay(15);   
  }
}

// Initialize LittleFS
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
    // Check if the message is "getReadings"
    if (strcmp((char*)data, "getReadings") == 0) {
      //if it is, send current sensor readings
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    } else {
      if (strcmp((char*) data, "activarServo") == 0) {
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

String imageToBase64(uint8_t *imageData, size_t length){
    base64_encodestate b64state;
    base64_init_encodestate(&b64state);

    const int outputSize = base64_needed_encoded_length(length);
    char *base64Buffer = new char[outputSize];

    int encodedLength = base64_encode_block((char *)imageData, length, base64Buffer, &b64state);
    encodedLength += base64_encode_blockend(base64Buffer + encodedLength, &b64state);

    String base64String(base64Buffer);
    delete[] base64Buffer;

    return base64String;


}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initWebSocket();
  servo.attach(4);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  Serial.println("Cosas iniciadas");

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    //Serial.println(sensorReadings);

    if ((millis() - lastTime) > camaraDelay) {
      // MOMENTO TOMAR IMAGEN Y GUARDARLA
      String ipSolicitar = IP_CAMARA + "capture";
      // Aquí hay que hacer la petición GET
      Serial.println(ipSolicitar);
      http.begin(ipSolicitar);
      int httpCode = http.GET();

      vTaskDelay(1000 / portTICK_PERIOD_MS);

      if (httpCode == HTTP_CODE_OK) {
        // Ahora hemos hecho que primero obtenga la imagen

        http.end();

        Serial.println("La imagen se ha cogido perfectamente");
        ipSolicitar = IP_CAMARA + "saved-photo";
        Serial.println(ipSolicitar);
        http.begin(ipSolicitar);

        Serial.println("Se ha solicitado la imagen");

        httpCode = http.GET();
        Serial.print(httpCode);

        if (httpCode == HTTP_CODE_OK) {

          Serial.println("La imagen se ha guardado");

          // Tenemos la imagen, ahora falta pasarla a base64
          // Obtenemos la imagen
          WiFiClient * stream = http.getStreamPtr();

          Serial.println("Imagen obtenida");

          // Calculamos tamaño imagen
          int len = http.getSize();
          uint8_t *imagenBuffer = new uint8_t[len];
          int index = 0;

          Serial.println("Se va a entrar en el loop");

          while (http.connected() && len > 0) {
            size_t size = stream->available();

            if (size) {
              // Lee los bytes de la imagen en el buffer
              int c = stream->readBytes(imagenBuffer + index, size);

              index += c;
              len -= c;
            }

            vTaskDelay(1 / portTICK_PERIOD_MS);
          }

          Serial.println("Se ha salido del loop");

          // Tenemos el texto claro en imagenBuffer

          vTaskDelay(1000 / portTICK_PERIOD_MS);
          
          // Ahora nos hace falta añadirla al documento JSON (getSensorReadings)
            String imagenBase64 = imageToBase64(imagenBuffer, index);
            readings["imagen"] = imagenBase64;         

          //Serial.println(imagenBase64Char);

          delete[] imagenBuffer;
        } else {
          Serial.println("No se ha podido cargar la imagen");
        }
      } else {
        Serial.println("No se ha podido tomar la imagen");
      }
      
      http.end();

    }

    notifyClients(sensorReadings);
    lastTime = millis();
  }
  ws.cleanupClients();
}