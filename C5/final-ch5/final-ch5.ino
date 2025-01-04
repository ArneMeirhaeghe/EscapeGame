
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
const char* mqtt_topic = "mqtt/defcon/ch4/control";    // For start command
const char* mqtt_reset_topic = "mqtt/defcon/control";  // For reset command
const char* mqtt_publish_topic = "mqtt/defcon/ch4/status";
const char* mqtt_connected_topic = "mqtt/defcon/ch4/connected";

WiFiClient espClient;
PubSubClient client(espClient);

// Disk UIDs
String diskUIDs[] = {
  "53 D5 C6 9E 61 00 01",  // Disk 1
  "53 D4 C6 9E 61 00 01",  // Disk 2
  "53 D3 C6 9E 61 00 01"   // Disk 3
};

#define NUM_DISKS 3

// Rotary encoder pins
const int clkPin = 2;  // CLK pin connected to D2
const int dtPin = 3;   // DT pin connected to D3
const int swPin = 4;   // SW pin connected to D4 (button)

// RFID reader pins
#define SS_PIN 10                  // SDA pin on RFID to D10
#define RST_PIN 9                  // RST pin on RFID to D9
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create an instance of the RFID reader

int diskIndex = 0;  // Current expected disk index

// File selection
String files[] = { "attack locs", "soldier list", "ranks" };
int numberOfFiles = sizeof(files) / sizeof(files[0]);
int fileIndex = 0;  // Current selected file index

// Rotary encoder variables
int lastClkState;
int currentClkState;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RGB LED Pins
const int ledPins[NUM_DISKS][2] = {
  { 5, 6 },   // Red and Green for LED 1
  { 7, 8 },   // Red and Green for LED 2
  { A0, A1 }  // Red and Green for LED 3
};

// Flags
bool systemStarted = false;

void setup() {
  Serial.begin(9600);  // Start serial communication
  Serial.println("Initializing system...");

  // WiFi Setup
  connectToWiFi();

  // MQTT Setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Initialize SPI and RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID reader initialized.");

  // Initialize rotary encoder pins
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);

  lastClkState = digitalRead(clkPin);

  // Initialize LCD but keep it off initially
  lcd.init();
  lcd.noBacklight();
  lcd.clear();

  // Initialize RGB LEDs and turn them off
  resetLEDs();

  Serial.println("Setup complete. Waiting for MQTT message...");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Only proceed if system is started
  if (systemStarted) {
    rfidHandler();
  }
}

void rfidHandler() {
  // Check for RFID tag
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;  // No card detected
  }

  Serial.println("RFID card detected. Attempting to read...");

  if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Error: Unable to read RFID card.");
    return;  // Unable to read card
  }

  // Read the RFID UID
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();

  Serial.print("Detected UID: ");
  Serial.println(content);

  // Validate the disk UID
  if (diskIndex < NUM_DISKS && content.substring(1) == diskUIDs[diskIndex]) {
    Serial.print("Correct disk (Disk ");
    Serial.print(diskIndex + 1);
    Serial.println(") detected.");

    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Disk ");
    lcd.print(diskIndex + 1);
    lcd.print(" detected");

    lcd.setCursor(0, 1);
    lcd.print("Take out disk");

    // Change LED color for current disk to green
    digitalWrite(ledPins[diskIndex][0], LOW);   // Turn red OFF
    digitalWrite(ledPins[diskIndex][1], HIGH);  // Turn green ON

    delay(5000);  // Wait for the disk to be removed

    diskIndex++;
    if (diskIndex < NUM_DISKS) {
      lcd.clear();
      lcd.print("Insert Disk ");
      lcd.print(diskIndex + 1);

      // Turn the next disk's LED to red (waiting for input)
      setLEDToRed(diskIndex);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Access granted!");
      delay(2000);
      fileSelection();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error: Restarting");

    delay(2000);

    // Restart from Disk 1
    diskIndex = 0;

    lcd.clear();
    lcd.print("Insert Disk 1");

    // Reset LEDs and set the first one to red
    resetLEDs();
    setLEDToRed(diskIndex);
  }
}


void setLEDToRed(int index) {
  if (index < NUM_DISKS) {
    digitalWrite(ledPins[index][0], HIGH);  // Turn red ON
    digitalWrite(ledPins[index][1], LOW);   // Turn green OFF
  }
}

void resetLEDs() {
  for (int i = 0; i < NUM_DISKS; i++) {
    pinMode(ledPins[i][0], OUTPUT);
    pinMode(ledPins[i][1], OUTPUT);
    digitalWrite(ledPins[i][0], LOW);
    digitalWrite(ledPins[i][1], LOW);
  }
}

void resetSystem() {
  diskIndex = 0;
  lcd.clear();
  lcd.noBacklight();
  resetLEDs();
  // setLEDToRed(0); // Set the first disk LED to red
  systemStarted = false;
}

void sendMQTTMessage(const char* topic, DynamicJsonDocument& doc) {
  String jsonData;
  serializeJson(doc, jsonData);
  client.publish(topic, jsonData.c_str());
}

void sendCompleted() {
  Serial.println("send completed");
  DynamicJsonDocument doc(128);
  doc["status"] = "completed";
  sendMQTTMessage(mqtt_publish_topic, doc);
}


void fileSelection() {
  lcd.clear();
  lcd.print("Select File:");
  updateLCD();

  while (true) {
    currentClkState = digitalRead(clkPin);
    if (currentClkState != lastClkState) {
      if (digitalRead(dtPin) != currentClkState) {
        fileIndex++;
      } else {
        fileIndex--;
      }

      if (fileIndex < 0) {
        fileIndex = numberOfFiles - 1;
      } else if (fileIndex >= numberOfFiles) {
        fileIndex = 0;
      }

      updateLCD();
    }

    lastClkState = currentClkState;

    if (digitalRead(swPin) == LOW) {
        Serial.print("Wrong file");
      if (files[fileIndex] == "attack locs") {
        Serial.println("Correct file");
        sendCompleted();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("File selected:");
        lcd.setCursor(0, 1);
        lcd.print(files[fileIndex]);
        delay(1000);
        lcd.clear();
        delay(1000);
        lcd.noBacklight();
        break;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error");
        lcd.setCursor(0, 1);
        lcd.print("reading file");
        delay(2000);

        lcd.clear();
        lcd.print("Select File:");
        updateLCD();
      }
    }
    delay(50);
  }
}

void updateLCD() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(files[fileIndex]);
}



void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("not connected");
    delay(500);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    // if (client.connect("Floppy_Client")) {
    //   client.subscribe(mqtt_topic);        // Subscribe to start topic
    //   client.subscribe(mqtt_reset_topic);  // Subscribe to reset topic
    // }
    // delay(5000);

    if (client.connect("Floppy_Client")) {
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
      lcd.backlight();
      lcd.clear();
      lcd.print("Insert Disk 1");

      // Turn on first disk LED to red
      setLEDToRed(0);
    } else if (topicStr == mqtt_reset_topic && doc["command"] == "reset") {
      resetSystem();
    }
  }
}

