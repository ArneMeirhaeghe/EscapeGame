#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi and MQTT Configuration
const char* ssid = "DFCN_DiFi";
const char* password = "azerty123";
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;

const char* hostname = "ESP32-CH5";
const char* mqtt_topic = "mqtt/defcon/ch5/control";      // For start command
const char* mqtt_reset_topic = "mqtt/defcon/control";    // For reset command
const char* mqtt_publish_topic = "mqtt/defcon/ch5/status";
const char* mqtt_connected_topic = "mqtt/defcon/ch5/connected";

WiFiClient espClient;
PubSubClient client(espClient);

// Disk UIDs
String diskUIDs[] = {
  "53 D5 C6 9E 61 00 01", // Disk 1
  "53 D4 C6 9E 61 00 01", // Disk 2
  "53 D3 C6 9E 61 00 01"  // Disk 3
};

#define NUM_DISKS 3

// Rotary encoder pins
const int clkPin = 2;
const int dtPin = 3;
const int swPin = 4;

// RFID reader pins
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

// File selection
String files[] = { "attack locs", "soldier list", "ranks" };
int numberOfFiles = sizeof(files) / sizeof(files[0]);
int fileIndex = 0;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RGB LED Pins
const int ledPins[NUM_DISKS][2] = {
  {5, 6},  // Red and Green for LED 1
  {7, 8},  // Red and Green for LED 2
  {A0, A1} // Red and Green for LED 3
};

// State variables
bool systemStarted = false;
int diskIndex = 0;
int lastClkState;

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing system...");

  // WiFi and MQTT setup
  setupWiFi();
  setupMQTT();

  // Initialize SPI and RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  // Initialize rotary encoder pins
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);
  lastClkState = digitalRead(clkPin);

  // Initialize LCD
  lcd.init();
  lcd.clear();
  lcd.noBacklight();

  // Initialize RGB LEDs
  resetLEDs();

  Serial.println("Setup complete. Waiting for MQTT message...");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (systemStarted) {
    handleRFID();
  }
}

void setupWiFi() {
  WiFi.disconnect();
  delay(100);
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
    if (client.connect("ESP32_Client")) {
      client.subscribe(mqtt_topic);
      client.subscribe(mqtt_reset_topic);

      // Send connected status
      DynamicJsonDocument doc(256);
      doc["connected"] = true;
      String jsonMessage;
      serializeJson(doc, jsonMessage);
      client.publish(mqtt_connected_topic, jsonMessage.c_str());
    } else {
      delay(5000);
    }
  }
}

void handleRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Read UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  if (diskIndex < NUM_DISKS && uid.substring(1) == diskUIDs[diskIndex]) {
    // Correct disk
    setLEDToGreen(diskIndex);
    lcd.backlight();
    lcd.clear();
    lcd.print("Disk ");
    lcd.print(diskIndex + 1);
    lcd.print(" detected!");
    delay(2000);

    diskIndex++;
    if (diskIndex < NUM_DISKS) {
      lcd.clear();
      lcd.print("Insert Disk ");
      lcd.print(diskIndex + 1);
      setLEDToRed(diskIndex);
    } else {
      lcd.clear();
      lcd.print("Access Granted!");
      delay(2000);
      fileSelection();
    }
  } else {
    lcd.clear();
    lcd.print("Error: Invalid Disk");
    resetSystem();
  }
}

void fileSelection() {
  lcd.clear();
  lcd.print("Select File:");
  updateLCD();

  while (true) {
    int currentClkState = digitalRead(clkPin);
    if (currentClkState != lastClkState) {
      if (digitalRead(dtPin) != currentClkState) {
        fileIndex++;
      } else {
        fileIndex--;
      }

      fileIndex = (fileIndex + numberOfFiles) % numberOfFiles;
      updateLCD();
    }

    lastClkState = currentClkState;

    if (digitalRead(swPin) == LOW) {
      sendFileStatus();
      lcd.clear();
      lcd.print("File Sent: ");
      lcd.print(files[fileIndex]);
      delay(2000);
      resetSystem();
      break;
    }
    delay(50);
  }
}

void sendFileStatus() {
  DynamicJsonDocument doc(256);
  doc["status"] = "completed";
  doc["file"] = files[fileIndex];
  String jsonData;
  serializeJson(doc, jsonData);
  client.publish(mqtt_publish_topic, jsonData.c_str());
}

void resetLEDs() {
  for (int i = 0; i < NUM_DISKS; i++) {
    pinMode(ledPins[i][0], OUTPUT);
    pinMode(ledPins[i][1], OUTPUT);
    digitalWrite(ledPins[i][0], LOW);
    digitalWrite(ledPins[i][1], LOW);
  }
}

void setLEDToRed(int index) {
  digitalWrite(ledPins[index][0], HIGH);
  digitalWrite(ledPins[index][1], LOW);
}

void setLEDToGreen(int index) {
  digitalWrite(ledPins[index][0], LOW);
  digitalWrite(ledPins[index][1], HIGH);
}

void resetSystem() {
  systemStarted = false;
  diskIndex = 0;
  lcd.clear();
  lcd.noBacklight();
  resetLEDs();
}

void updateLCD() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(files[fileIndex]);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    String topicStr = String(topic);
    if (topicStr == mqtt_topic && doc["command"] == "start") {
      systemStarted = true;
      lcd.backlight();
      lcd.clear();
      lcd.print("Insert Disk 1");
      setLEDToRed(0);
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}
