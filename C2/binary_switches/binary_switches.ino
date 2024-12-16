#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;

const char* hostname = "ESP32-CH2";

const char* mqtt_reset_topic = "mqtt/defcon/control";  // For reset command
const char* mqtt_topic = "mqtt/defcon/ch2/control";   // For start command
const char* mqtt_publish_topic = "mqtt/defcon/ch2/status";
const char* mqtt_connected_topic = "mqtt/defcon/ch2/connected";

WiFiClient espClient;
PubSubClient client(espClient);

// Pins and target configuration
const int buttonPins[8] = {9, 8, 7, 6, 5, 4, 3, 2};  // Pins D2 to D9
const int relayPin = 1;                             // Pin connected to the relay
const byte targetNumber = 169;                         // Target number

// RGB LED Pins
const int redPin = 13;   // Pin for red part of RGB LED
const int greenPin = 12; // Pin for green part of RGB LED
const int bluePin = 11;  // Pin for blue part of RGB LED

// State variables
bool systemStarted = false;
bool systemStopped = false;  // Stop sending data after correct value
byte buttonsState = 0;
byte lastButtonsState = 255; // Set initial state to detect first change
bool isCompleted = false;    // Tracks if the "completed" status has been sent

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing system...");

  setupWiFi();
  setupMQTT();

  // Initialize button pins
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Relay starts in the closed state

  // Initialize RGB LED pins
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Set default LED color to red
  setRGBColor(255, 0, 0);

  Serial.println("Setup complete. Waiting for MQTT message...");
}

void loop() {
  if (!client.connected() && !isCompleted) {
    reconnectMQTT();
  }
  client.loop();

  if (systemStarted && !systemStopped) {
    checkButtonStates();
  }

  // Update RGB LED state
  updateLEDState();
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
      client.subscribe(mqtt_topic);
      client.subscribe(mqtt_reset_topic);

      // Send connected status as JSON
      DynamicJsonDocument doc(256);
      doc["connected"] = true;
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      client.publish(mqtt_connected_topic, jsonMessage.c_str());

    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void checkButtonStates() {
  buttonsState = 0;

  for (int i = 0; i < 8; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      buttonsState |= (1 << i);
    }
  }

  if (buttonsState != lastButtonsState) {
    lastButtonsState = buttonsState;

    if (buttonsState == targetNumber) {
      Serial.println("Correct target reached!");
      openRelay();
      sendMQTTStatus("completed");
      isCompleted = true;
      systemStopped = true;
    } else {
      Serial.println("Incorrect target!");
      sendMQTTStatus("incorrect");
    }
  }
}

void openRelay() {
  digitalWrite(relayPin, HIGH);
  Serial.println("Relay opened.");
  delay(3000);
}

void closeRelay() {
  digitalWrite(relayPin, LOW);
  Serial.println("Relay closed.");
}

void sendMQTTStatus(const char* status) {
  DynamicJsonDocument doc(128);
  doc["status"] = status;

  String jsonData;
  serializeJson(doc, jsonData);

  Serial.println("Sending to MQTT Broker:");
  Serial.print("Message: ");
  Serial.println(jsonData);

  client.publish(mqtt_publish_topic, jsonData.c_str());
}

void resetSystem() {
  systemStarted = false;
  systemStopped = false;
  isCompleted = false;
  buttonsState = 0;
  lastButtonsState = 255;
  closeRelay();
  Serial.println("System reset via MQTT.");
}

// Function to set RGB LED color
void setRGBColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// Function to update RGB LED state
void updateLEDState() {
  if (!systemStarted) {
    // Red LED (system not started)
    setRGBColor(255, 0, 0);
  } else if (systemStarted && !isCompleted) {
    // Orange LED (system started)
    setRGBColor(255, 165, 0);
  } else if (isCompleted) {
    // Green LED (system completed)
    setRGBColor(0, 255, 0);
  }
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
      systemStopped = false;
      isCompleted = false;
      closeRelay();
      Serial.println("System started via MQTT.");
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}
