#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;

const char* hostname = "Rotating_Disks_ESP_32";

// MQTT Topics
const char* mqtt_reset_topic = "mqtt/defcon/control";  // For reset command
const char* mqtt_control_topic = "mqtt/defcon/ch3/control";  // For start command
const char* mqtt_publish_topic = "mqtt/defcon/ch3/status";
const char* mqtt_connected_topic = "mqtt/defcon/ch3/connected";

WiFiClient espClient;
PubSubClient client(espClient);

// Pins and Configuration
const int reedSwitchPins[7] = {2, 3, 4, 5, 6, 7, 8}; // Pins for reed switches
const int relayPin = 13;                            // Relay pin
const int keySwitchPin = 9;                         // Key switch pin

// RGB LED Pins
const int redPin = 10;   // Pin for red part of RGB LED
const int greenPin = 11; // Pin for green part of RGB LED
const int bluePin = 12;  // Pin for blue part of RGB LED

// State Variables
bool systemStarted = false;
bool isCompleted = false;
bool lastAllClosed = false;                         // Tracks if all switches are closed
bool lastReedStates[7];                             // Stores previous reed states
bool lastKeyState = HIGH;

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

  // Initialize RGB LED pins
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Turn off LED initially
  setRGBColor(0, 0, 0);

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

  // Continuously monitor the key state for instant LED updates
  checkKeyState();
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
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (client.connect("Rotating_Disks_ESP_32")) {
      client.subscribe(mqtt_control_topic);
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

void checkSwitchStates() {
  if (lastKeyState == LOW && !isCompleted) {
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

void checkKeyState() {
  bool keyState = digitalRead(keySwitchPin);
  if (keyState != lastKeyState) {
    Serial.print("Key state changed: ");
    Serial.println(keyState == LOW ? "ON" : "OFF");
    lastKeyState = keyState;
    updateLEDState();
  }
}

void onComplete() {
  isCompleted = true;
  systemStarted = false;

  // Send correct status
  sendMQTTStatus("completed");
  Serial.println("All reed switches closed. Challenge complete!");

  setRGBColor(0, 255, 0);

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
      setRGBColor(255, 0, 0);
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}

// Function to set RGB LED color
void setRGBColor(int red, int green, int blue) {
  Serial.print("Updating LED color to: R=");
  Serial.print(red);
  Serial.print(" G=");
  Serial.print(green);
  Serial.print(" B=");
  Serial.println(blue);
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// Function to update RGB LED state
void updateLEDState() {
  if (!systemStarted && !isCompleted) {
    setRGBColor(0, 0, 0);
  } else if (systemStarted && !isCompleted) {
    if (lastKeyState == LOW) {
      setRGBColor(255, 165, 0);
    } else {
      setRGBColor(255, 0, 0);
    }
  } else if (isCompleted) {
    setRGBColor(0, 255, 0);
  }
}
