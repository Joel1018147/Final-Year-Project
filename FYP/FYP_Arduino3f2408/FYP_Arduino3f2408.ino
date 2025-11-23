#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "esp_wifi.h"
#include <ArduinoJson.h>   // <-- for JSON publishing

// WiFi credentials
const char* ssid = "iPhone";
const char* password = "Goat-2003";

// MQTT broker info
const char* mqtt_server = "d6db3227.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883;
const char* mqtt_user = "Host";
const char* mqtt_pass = "Goat-2003";

// Pin Definitions
#define trigPin 26
#define echoPin 27
#define pHSensorPin 34
#define turbiditySensorPin 32   // <-- Updated here
#define ledPin 2  // Built-in LED on GPIO 2

// Sensor variables
long duration;
int distance;
float pHValue = 0;
int turbidityValue = 0;
int turbidityThreshold = 250;

// Setup WiFi & MQTT clients
WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pHSensorPin, INPUT);
  pinMode(turbiditySensorPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, !digitalRead(ledPin));  // blink LED
    delay(500);
    Serial.print(".");
  }
  digitalWrite(ledPin, HIGH); // solid LED when connected
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set antenna config
  esp_wifi_set_ant_gpio(0);

  // Insecure connection (skip cert verification)
  espClient.setInsecure();

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Measure water level
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // Read pH sensor and turbidity
  int rawPH = analogRead(pHSensorPin);
  int rawTurbidity = analogRead(turbiditySensorPin);  // using GPIO 32 now

  // Convert raw pH reading to voltage
  float voltage = rawPH * (3.3 / 4095.0);

  // Convert voltage to pH value (Assuming 0V = pH 0, 3.0V = pH 14)
  pHValue = (voltage / 3.0) * 14.0;

  turbidityValue = rawTurbidity;

  // Display readings
  Serial.print("Water Level: "); Serial.print(distance); Serial.println(" cm");
  Serial.print("pH Value: "); Serial.println(pHValue, 2);  // show 2 decimal places
  Serial.print("Turbidity: "); Serial.println(turbidityValue);

  // Create a JSON object
  StaticJsonDocument<256> jsonDoc;
  jsonDoc["water_level"] = distance;
  jsonDoc["ph_value"] = pHValue;
  jsonDoc["turbidity_value"] = turbidityValue;

  if (turbidityValue > turbidityThreshold) {
    Serial.println("WARNING: Turbidity exceeds threshold!");
    jsonDoc["warning"] = "Turbidity high";
  }

  // Serialize JSON to a buffer
  char buffer[256];
  size_t n = serializeJson(jsonDoc, buffer);

  // Publish JSON to MQTT
  client.publish("sensor/data", buffer, n);

  delay(15000); // wait before next reading
}

void reconnectMQTT() {
  while (!client.connected()) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    Serial.print("Connecting to MQTT broker...");
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" connected");
      digitalWrite(ledPin, HIGH); // LED on if connected
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages if needed
}
