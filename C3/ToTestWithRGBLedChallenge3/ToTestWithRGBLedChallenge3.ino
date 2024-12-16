#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;

const char* hostname = "ESP32-CH3";

// MQTT Topics
const char* mqtt_reset_topic = "mqtt/defcon/control";  // For reset command
const char* mqtt_control_topic = "mqtt/defcon/ch3/control";  // For start command
const char* mqtt_publish_topic = "mqtt/defcon/ch3/status";
const char* mqtt_connected_topic = "mqtt/defcon/ch3/connected";

WiFiClient espClient;
PubSubClient client(espClient);

// Pins and Configuration
const int reedSwitchPins[7] = {2, 3, 4, 5, 6, 7, 8}; // Pins for reed switches
const int relayPin = 12;                            // Relay pin
const int keySwitchPin = 9;                         // Key switch pin

// State Variables
bool systemStarted = false;
bool isCompleted = false;
bool lastAllClosed = false;                         // Tracks if all switches are closed
bool lastReedStates[7];                             // Stores previous reed states

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing system...");

  setupWiFi();
  setupMQTT();

  // Initialize reed switch and key switch pins
  for (int i = 0; i < 7; i++) {
    pinMode(reedSwitchPins[i], INPUT_PULLUP);
    lastReedStates[i] = (digitalRead(reedSwitchPins[i]) == LOW);
  }
  pinMode(keySwitchPin, INPUT_PULLUP);

  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  Serial.println("Setup complete. Waiting for MQTT message...");
}

void loop() {
  if (!client.connected() && !isCompleted) {
    reconnectMQTT();
  }
  client.loop();

  if (systemStarted && !isCompleted) {
    checkSwitchStates();
  }
}

void setupWiFi() {
  Serial.println("Disconnecting WiFi...");
  WiFi.disconnect();
  delay(100);

  Serial.print("Connecting to WiFi...");
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
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
      client.subscribe(mqtt_control_topic);
      client.subscribe(mqtt_reset_topic);
      Serial.println("MQTT Connected!");

      // Send connected status as JSON
      DynamicJsonDocument doc(256);
      doc["connected"] = true;
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      client.publish(mqtt_connected_topic, jsonMessage.c_str());

      Serial.print("Connected message sent: ");
      Serial.println(jsonMessage);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void checkSwitchStates() {
  if (digitalRead(keySwitchPin) == LOW) {  // Key switch must be active
    bool allClosed = true;

    for (int i = 0; i < 7; i++) {
      bool currentState = (digitalRead(reedSwitchPins[i]) == LOW);
      if (currentState != lastReedStates[i]) {
        Serial.print("Reed switch ");
        Serial.print(i + 1);
        Serial.print(" is now ");
        Serial.println(currentState ? "CLOSED" : "OPEN");
        lastReedStates[i] = currentState;
      }
      if (!currentState) {
        allClosed = false;
      }
    }

    if (allClosed) {
      if (!isCompleted) {
        onComplete();
      }
    } else if (lastAllClosed != allClosed) {
      sendMQTTStatus("incorrect");
      Serial.println("Status changed to incorrect.");
    }
    lastAllClosed = allClosed;
  }
}

void onComplete() {
  isCompleted = true;
  systemStarted = false;

  // Send correct status
  sendMQTTStatus("correct");
  Serial.println("All reed switches closed. Challenge complete!");

  // Activate relay for 3 seconds
  digitalWrite(relayPin, HIGH);
  delay(3000);
  digitalWrite(relayPin, LOW);
}

void sendMQTTStatus(const char* status) {
  DynamicJsonDocument doc(128);
  doc["status"] = status;

  String jsonData;
  serializeJson(doc, jsonData);

  client.publish(mqtt_publish_topic, jsonData.c_str());
  Serial.print("Sent to MQTT: ");
  Serial.println(jsonData);
}

void resetSystem() {
  systemStarted = false;
  isCompleted = false;
  lastAllClosed = false;

  // Reset reed switch states
  for (int i = 0; i < 7; i++) {
    lastReedStates[i] = (digitalRead(reedSwitchPins[i]) == LOW);
  }

  Serial.println("System reset via MQTT.");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    String topicStr = String(topic);

    if (topicStr == mqtt_control_topic && doc["command"] == "start") {
      systemStarted = true;
      isCompleted = false;
      Serial.println("System started via MQTT.");
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}
