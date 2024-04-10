#include <Arduino.h>

#include <ESP32Servo.h> // by Kevin Harrington
#include <ESPAsyncWebSrv.h> // by dvarrel
#include <iostream>
#include <sstream>
#include <AsyncTCP.h> // by dvarrel
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include "motor.hpp"

// defines
#define bucketServoPin  23
#define auxServoPin 22
#define lightPin1 18
#define lightPin2 5

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define ARMUP 5
#define ARMDOWN 6
#define STOP 0

// global constants

const char* ssid = "MiniSkidi";

// global variables

Servo bucketServo;
Servo auxServo;

bool horizontalScreen;//When screen orientation is locked vertically this rotates the D-Pad controls so that forward would now be left.
bool removeArmMomentum = false;

Motor rightMotor(25, 26);
Motor leftMotor(33, 32);
Motor armMotor(21, 19);

AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

void moveCar(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue);
  if (!horizontalScreen)
  {
    switch (inputValue)
    {

      case UP:
        rightMotor.Forward();
        leftMotor.Forward();
        break;

      case DOWN:
        rightMotor.Backward();
        leftMotor.Backward();
        break;

      case LEFT:
        rightMotor.Forward();
        leftMotor.Backward();
        break;

      case RIGHT:
        rightMotor.Backward();
        leftMotor.Forward();
        break;

      case STOP:
        if(removeArmMomentum) {
          armMotor.RemoveMomentum();
          removeArmMomentum = false;
        }
        armMotor.Stop();
        rightMotor.Stop();
        leftMotor.Stop();
        break;

      case ARMUP:
        armMotor.Forward();
        break;

      case ARMDOWN:
        armMotor.Backward();
        removeArmMomentum = true;
        break;

      default:
        armMotor.Stop();
        rightMotor.Stop();
        leftMotor.Stop();
        break;
    }
  } else {
    switch (inputValue)
    {
      case UP:
        rightMotor.Backward();
        leftMotor.Forward();
        break;

      case DOWN:
        rightMotor.Forward();
        leftMotor.Backward();
        break;

      case LEFT:
        rightMotor.Backward();
        leftMotor.Backward();
        break;

      case RIGHT:
        rightMotor.Forward();
        leftMotor.Forward();
        break;

      case ARMUP:
        armMotor.Forward();
        break;

      case ARMDOWN:
        armMotor.Backward();
        removeArmMomentum = true;
        break;

      case STOP:
      default:
        if(removeArmMomentum) {
          armMotor.RemoveMomentum();
          removeArmMomentum = false;
        }
        armMotor.Stop();
        rightMotor.Stop();
        leftMotor.Stop();
        break;
    }
  }
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      moveCar(STOP);
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
        int valueInt = atoi(value.c_str());
        if (key == "MoveCar")
        {
          moveCar(valueInt);
        }
        else if (key == "AUX")
        {
          auxServo.write(valueInt);
        }
        else if (key == "Bucket")
        {
          bucketServo.write(valueInt);
        }
        else if (key == "Light")
        {
          digitalWrite(lightPin2, !digitalRead(lightPin2));
        }
        else if (key == "Switch")
        {
          horizontalScreen = !horizontalScreen;
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
    default:
      break;
  }
}

void setUpPinModes()
{
  rightMotor.Initialize();
  leftMotor.Initialize();
  armMotor.Initialize();

  bucketServo.attach(bucketServoPin);
  auxServo.attach(auxServoPin);
  auxServo.write(150);
  bucketServo.write(140);

  pinMode(lightPin1, OUTPUT);
  pinMode(lightPin2, OUTPUT);
  digitalWrite(lightPin1, LOW);
}

void setup(void)
{
  Serial.begin(115200);
  setUpPinModes();

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.setHostname("miniskidi");

  IPAddress Ip(192, 168, 1, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);

  WiFi.softAP(ssid);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (MDNS.begin("miniskidi")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("mDNS responder started with hostname miniskidi");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false);
  });

  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/styles.css", String(), false);
  });

  server.onNotFound(handleNotFound);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  wsCarInput.cleanupClients();
}
