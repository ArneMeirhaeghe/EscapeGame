// /pot_meters_stabilized_mqtt.ino

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;

const char* hostname = "Radio_ESP_32";

// MQTT Topics
const char* mqtt_reset_topic = "mqtt/defcon/control";       // Reset Command Topic
const char* mqtt_control_topic = "mqtt/defcon/ch1/control"; // Start Command Topic
const char* mqtt_publish_topic = "mqtt/defcon/ch1/status";  // Potentiometer Status Topic
const char* mqtt_connected_topic = "mqtt/defcon/ch3/connected"; // Connected Status Topic
const char* mqtt_values_topic = "potentiometer/values";     // Potentiometer Values Topic

WiFiClient espClient;
PubSubClient client(espClient);

bool systemStarted = false;

// Pin Configuratie
#define POT1_PIN A1  // GPIO 34 voor Potentiometer 1
#define POT2_PIN A2  // GPIO 35 voor Potentiometer 2

// Stabilisatie variabelen
int previousPot1Value = -1;
int previousPot2Value = -1;

unsigned long lastMsgTime = 0;
const long interval = 500; // Interval voor berichten

void setup() {
  Serial.begin(9600);
  delay(2000); // Wacht 2 seconden
  Serial.println("Initializing system...");

  Serial.println("Before WiFi setup");
  setupWiFi();
  Serial.println("After WiFi setup, before MQTT setup");
  setupMQTT();
  Serial.println("After MQTT setup");

  publishConnectedStatus();
  Serial.println("Setup complete. Waiting for MQTT messages...");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (systemStarted) {
    readAndPublishPotentiometerValues();
  }
}

void setupWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect(hostname)) {
      client.subscribe(mqtt_control_topic);
      client.subscribe(mqtt_reset_topic);
      publishConnectedStatus();
      Serial.println("MQTT Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void readAndPublishPotentiometerValues() {
  unsigned long currentTime = millis();
  if (currentTime - lastMsgTime > interval) {
    lastMsgTime = currentTime;

    int potValue1 = stabilizeValue(analogRead(POT1_PIN));
    int potValue2 = stabilizeValue(analogRead(POT2_PIN));

    if (potValue1 != previousPot1Value || potValue2 != previousPot2Value) {
      char message[50];
      snprintf(message, sizeof(message), "%d,%d", potValue1, potValue2);
      client.publish(mqtt_values_topic, message);

      Serial.print("Published Values: ");
      Serial.println(message);

      previousPot1Value = potValue1;
      previousPot2Value = potValue2;
    }
  }
}

int stabilizeValue(int rawValue) {
  return ((rawValue + 175) / 350) * 350; // Verdeel in ~12 intervallen
}

void publishConnectedStatus() {
  client.publish(mqtt_connected_topic, "connected");
  Serial.println("Published connected status.");
}

void resetSystem() {
  Serial.println("System resetting...");
  systemStarted = false;
  previousPot1Value = -1;
  previousPot2Value = -1;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(message);

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    String topicStr = String(topic);
    if (topicStr == mqtt_control_topic && doc["command"] == "start") {
      systemStarted = true;
      Serial.println("System started!");
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  } else {
    Serial.println("Failed to parse JSON message.");
  }
}
