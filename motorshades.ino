#include "EspMQTTClient.h"
#include "ArduinoJson.h"
#include "Secrets.h"

#include "Shades.pb.h"

#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

uint8_t buffer[128];
size_t message_length;
bool status;

char lifecycleTopic[100] = "";
char positionJsonTopic[100] = "";
char positionReplyJsonTopic[100] = "";
char positionProtoTopic[100] = "";
char positionReplyProtoTopic[100] = "";

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

  sprintf(lifecycleTopic, "motorshades/%s/lifecycle", DEVICE_NAME);
  sprintf(positionJsonTopic, "motorshades/%s/position/json", DEVICE_NAME);
  sprintf(positionReplyJsonTopic, "motorshades/%s/position-reply/json", DEVICE_NAME);
  sprintf(positionProtoTopic, "motorshades/%s/position/proto", DEVICE_NAME);
  sprintf(positionReplyProtoTopic, "motorshades/%s/position-reply/proto", DEVICE_NAME);

  pinMode(motorPinA1, OUTPUT);
  pinMode(motorPinA2, OUTPUT);

  digitalWrite(motorPinA1, LOW);
  digitalWrite(motorPinA2, LOW);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater("admin", ADMIN_PASSWORD);
  client.enableOTA(ADMIN_PASSWORD, 8888);
  client.enableLastWillMessage(lifecycleTopic, "{\"state\":\"disconnected\"}");
}

void onConnectionEstablished() {
  client.subscribe(positionProtoTopic, [](const String & payload) {
    payload.toCharArray(jsonBuffer, JSON_BUFFER_MAX);

    example_Adjustment adjustment = example_Adjustment_init_zero;
    pb_istream_t stream = pb_istream_from_buffer((const pb_byte_t*)(payload.c_str()), payload.length());
    status = pb_decode(&stream, example_Adjustment_fields, &adjustment);

    if (!status) {
      Serial.println("Error decoding message");
      return;
    }

    Serial.println("Received adjustment message:");
    Serial.print("> pin: ");
    Serial.println(adjustment.pin);
    Serial.print("> duration_ms: ");
    Serial.println(adjustment.duration_ms);
    Serial.print("> duty_cycle: ");
    Serial.println(adjustment.duty_cycle);

    // client.publish(positionReplyProtoTopic, replyBuffer);
  });

  client.subscribe(positionJsonTopic, [](const String & payload) {
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

    client.publish(positionReplyJsonTopic, replyBuffer);

    // TODO: Handle delay without blocking the MQTT client
    delay(duration);

    digitalWrite(motorPinA1, LOW);
    digitalWrite(motorPinA2, LOW);
  });

  client.publish(lifecycleTopic, "{\"state\":\"connected\"}");
}

void loop() {
  client.loop();
}
