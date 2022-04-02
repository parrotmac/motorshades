#include "EspMQTTClient.h"
#include "ArduinoJson.h"
#include "Secrets.h"

const size_t CAPACITY = JSON_OBJECT_SIZE(3);

const size_t JSON_BUFFER_MAX = 64;
char jsonBuffer[JSON_BUFFER_MAX];

const int motorPinA1 = D6;
const int motorPinA2 = D7;

EspMQTTClient client(
  WIFI_SSID,
  WIFI_PSK,
  MQTT_HOST_IP,
  MQTT_USERNAME,
  MQTT_PASSWORD,
  DEVICE_NAME
);

void setup() {
  Serial.begin(115200);

  pinMode(motorPinA1, OUTPUT);
  pinMode(motorPinA2, OUTPUT);

  digitalWrite(motorPinA1, LOW);
  digitalWrite(motorPinA2, LOW);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater("admin", ADMIN_PASSWORD);
  client.enableOTA(ADMIN_PASSWORD, 8888);
  client.enableLastWillMessage("motorshades/lifecycle", "{\"state\":\"disconnected\"}");
}

void onConnectionEstablished() {
  client.subscribe("motorshades/position/json", [](const String & payload) {
    payload.toCharArray(jsonBuffer, JSON_BUFFER_MAX);

    StaticJsonDocument<CAPACITY> doc;
    deserializeJson(doc, jsonBuffer);

    int pinNumber = doc["pin"];    

    int duration = doc["duration"];
    if (duration < 1) {
      duration = 1;
    }
    if (duration > 2000) {
      duration = 2000;
    }

    int dutyCycle = doc["dutyCycle"];
    if (dutyCycle < 0) {
      dutyCycle = 0;
    }
    if (dutyCycle > 255) {
      dutyCycle = 255;
    }

    char replyBuffer[64];
    sprintf(replyBuffer, "Pin(%d)\tDuration(%d)\tDutyCycle(%d)", pinNumber, duration, dutyCycle);

    if (pinNumber == 1) {
      analogWrite(motorPinA1, dutyCycle);
      digitalWrite(motorPinA2, LOW);
    } else if (pinNumber == 2) {
      digitalWrite(motorPinA1, LOW);
      analogWrite(motorPinA2, dutyCycle);
    } else {
      sprintf(replyBuffer, "Invalid Pin(%d)", pinNumber);
    }

    client.publish("motorshades/position/reply", replyBuffer);

    // TODO: Handle delay without blocking the MQTT client
    delay(duration);

    digitalWrite(motorPinA1, LOW);
    digitalWrite(motorPinA2, LOW);
  });

  client.publish("motorshades/lifecycle", "{\"state\":\"connected\"}");
}

void loop() {
  client.loop();
}
