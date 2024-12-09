#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "WOT_G2_escape-box";
const char* password = "enterthegame";
const char* mqtt_server = "192.168.0.165";
const int mqtt_port = 1885;

const char* hostname = "ESP32-CH1";

const char* mqtt_reset_topic = "mqtt/defcon/control";  // For reset command
// CHANGE ME
const char* mqtt_topic = "mqtt/defcon/ch1/control";      // For start command
const char* mqtt_publish_topic = "mqtt/defcon/ch1/status";

WiFiClient espClient;
PubSubClient client(espClient);

bool systemStarted = false; // System start flag

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing system...");

  // Setup WiFi and MQTT
  setupWiFi();
  setupMQTT();

  // Initialize Switches and Leds

  Serial.println("Setup complete. Waiting for MQTT message...");
}


void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  readAndPublishPotentiometerValues();
  if (systemStarted) {
  }
}

void setupWiFi() {
  Serial.println("Disconnecting WiFi...");
  WiFi.disconnect(); // Disconnect any existing connection
  delay(100);        // Allow time for disconnection

  Serial.print("Connecting to WiFi...");
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password); // Start WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("ESP32_Client")) {
      client.subscribe(mqtt_topic);
      client.subscribe(mqtt_reset_topic);
      Serial.println("MQTT Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void readAndPublishPotentiometerValues() {
  int potValue1 = analogRead(A1);
  int potValue2 = analogRead(A2);

  char message[50];
  snprintf(message, sizeof(message), "%d,%d", potValue1, potValue2);

  client.publish("potentiometer/values", message);

  Serial.print("Gegevens verzonden: ");
  Serial.println(message);

  delay(1000);
}

// Reset system
void resetSystem() {
  // reset functionality
  systemStarted = false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    String topicStr = String(topic);
    if (topicStr == mqtt_topic && doc["command"] == "start") {
      systemStarted = true;
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}