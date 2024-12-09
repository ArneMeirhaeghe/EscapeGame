#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;
const char* mqtt_control_topic = "mqtt/defcon/ch2/control";  // For start command
const char* mqtt_reset_topic = "mqtt/defcon/control";       // For reset command

WiFiClient espClient;
PubSubClient client(espClient);

// Pins and target configuration
const int buttonPins[8] = {9, 8, 7, 6, 5, 4, 3, 2};  // Pins D2 to D9
const int relayPin = 12;                             // Pin connected to the relay
const byte targetNumber = 3;                         // Target number

// State variables
bool systemStarted = false;
bool systemStopped = false;  // Stop sending data after correct value
byte buttonsState = 0;
byte lastButtonsState = 255; // Set initial state to detect first change
bool isCompleted = false;    // Tracks if the "completed" status has been sent

void setup() {
  Serial.begin(9600);

  // WiFi setup
  connectToWiFi();

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Initialize button pins
  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Initialize relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);  // Relay starts in the closed state

  Serial.println("Setup complete.");
}

void loop() {
  // Only try to reconnect if not already completed
  if (!client.connected() && !isCompleted) {
    reconnectMQTT();
  }
  client.loop();

  // Process button states only if the system has started and hasn't stopped
  if (systemStarted && !systemStopped) {
    checkButtonStates();
  }
}

void checkButtonStates() {
  buttonsState = 0;

  // Read button states
  for (int i = 0; i < 8; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      buttonsState |= (1 << i);
    }
  }

  // Check if button states have changed
  if (buttonsState != lastButtonsState) {
    lastButtonsState = buttonsState;

    if (buttonsState == targetNumber) {
      Serial.println("Correct target reached!");

      // Open the relay
      openRelay();

      // Send completed status
      sendMQTTStatus("completed");

      // Mark the system as completed
      isCompleted = true;
      systemStopped = true;
    } else {
      Serial.println("Incorrect target!");

      // Send incorrect status
      sendMQTTStatus("incorrect");
    }
  }
}

void openRelay() {
  digitalWrite(relayPin, HIGH);  // Open the relay
  Serial.println("Relay opened.");
  delay(3000);
}

void closeRelay() {
  digitalWrite(relayPin, LOW);  // Close the relay
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

  client.publish(mqtt_control_topic, jsonData.c_str());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT Received -> Topic: ");
  Serial.print(topic);
  Serial.print(", Message: ");
  Serial.println(message);

  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    String topicStr = String(topic);

    // Handle start command
    if (topicStr == mqtt_control_topic && doc["command"] == "start") {
      systemStarted = true;
      systemStopped = false;  // Reset stop state
      isCompleted = false;    // Reset completed state
      closeRelay();           // Ensure relay is closed at start
      Serial.println("System started via MQTT.");
    }

    // Handle reset command
    if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}

void resetSystem() {
  systemStarted = false;
  systemStopped = false;    // Reset stop state
  isCompleted = false;      // Reset completed state
  buttonsState = 0;
  lastButtonsState = 255;   // Force detection of next change
  closeRelay();             // Close the relay on reset
  Serial.println("System reset via MQTT.");
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_Client")) {
      Serial.println("connected");

      // Subscribe to topics
      client.subscribe(mqtt_control_topic);
      client.subscribe(mqtt_reset_topic);

      Serial.println("Subscribed to topics:");
      Serial.println(mqtt_control_topic);
      Serial.println(mqtt_reset_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(5000);
    }
  }
}
