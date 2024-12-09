#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi en MQTT configuratie
const char* ssid = "RPI5-DiFi";
const char* password = "ChangeMe";
const char* mqtt_server = "10.3.141.1";
const int mqtt_port = 1885;

// MQTT topics
const char* mqtt_control_topic = "mqtt/defcon/ch3/control";    // Voor startcommando's
const char* mqtt_reset_topic = "mqtt/defcon/control";          // Voor resetcommando's
const char* mqtt_status_topic = "mqtt/defcon/ch3/status";      // Voor statusupdates

WiFiClient espClient;
PubSubClient client(espClient);

// Pinconfiguratie
const int reedSwitchPins[7] = {2, 3, 4, 5, 6, 7, 8};  // Pinnen voor reed switches
const int relayPin = 12;                              // Pin voor het relais
const int keySwitchPin = 9;                           // Pin voor het sleutelcontact

// Statusvariabelen
bool systemActive = false;     // True wanneer de challenge actief is
bool isCompleted = false;      // True wanneer de challenge is voltooid
bool lastAllClosed = false;    // Voor het detecteren van statuswijzigingen van de reed switches

// Houdt de vorige status van elke reed switch bij
bool lastReedStates[7];

void setup() {
  Serial.begin(115200);

  // WiFi en MQTT setup
  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Pin setup
  for (int i = 0; i < 7; i++) {
    pinMode(reedSwitchPins[i], INPUT_PULLUP);
    lastReedStates[i] = digitalRead(reedSwitchPins[i]);
  }
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Relais uit bij start
  pinMode(keySwitchPin, INPUT_PULLUP);

  Serial.println("Setup voltooid. Wacht op startcommando.");
}

void loop() {
  // Alleen proberen te herverbinden als we niet verbonden zijn
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  if (systemActive && !isCompleted) {
    checkSwitches();
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Verbinden met MQTT...");
    if (client.connect("ESP32_Client")) {
      Serial.println("Verbonden met MQTT.");

      // Abonneer op control en reset topics
      client.subscribe(mqtt_control_topic);
      client.subscribe(mqtt_reset_topic);
      Serial.println("Geabonneerd op control en reset topics.");
    } else {
      Serial.print("MQTT-verbinding mislukt, rc=");
      Serial.print(client.state());
      Serial.println(". Opnieuw proberen in 5 seconden.");
      delay(5000);
    }
  }
}

void checkSwitches() {
  // Controleer of het sleutelcontact is geactiveerd
  if (digitalRead(keySwitchPin) == LOW) {
    // Sleutelcontact is aan
    bool allClosed = true;

    // Controleer elke reed switch
    for (int i = 0; i < 7; i++) {
      bool currentState = (digitalRead(reedSwitchPins[i]) == LOW);
      if (currentState != lastReedStates[i]) {
        // Status van reed switch is veranderd
        Serial.print("Reed switch ");
        Serial.print(i + 1);
        Serial.print(" is nu ");
        Serial.println(currentState ? "GESLOTEN" : "OPEN");
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
    } else {
      if (lastAllClosed != allClosed) {
        // Status gewijzigd van alle gesloten naar niet alle gesloten
        sendMQTTStatus("incorrect");
        Serial.println("Status gewijzigd naar incorrect.");
      }
    }
    lastAllClosed = allClosed;
  } else {
    // Sleutelcontact is uit; geen actie
  }
}

void onComplete() {
  isCompleted = true;
  systemActive = false;

  // Verstuur correct status
  sendMQTTStatus("correct");
  Serial.println("Alle reed switches maken contact. Challenge voltooid!");

  // Activeer relais gedurende 3 seconden
  digitalWrite(relayPin, HIGH);
  delay(3000);
  digitalWrite(relayPin, LOW);
}

void sendMQTTStatus(const char* status) {
  DynamicJsonDocument doc(128);
  doc["status"] = status;

  String jsonData;
  serializeJson(doc, jsonData);

  client.publish(mqtt_status_topic, jsonData.c_str());
  Serial.print("Verzonden naar MQTT: ");
  Serial.println(jsonData);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Ontvangen MQTT-bericht op topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    const char* command = doc["command"];
    String topicStr = String(topic);

    if (topicStr == mqtt_control_topic && strcmp(command, "start") == 0) {
      onStart();
    } else if (topicStr == mqtt_reset_topic && strcmp(command, "reset") == 0) {
      onReset();
    }
  }
}

void onStart() {
  if (!isCompleted) {
    systemActive = true;
    Serial.println("Challenge gestart. Monitoring van reed switches begonnen.");
  }
}

void onReset() {
  systemActive = false;
  isCompleted = false;
  lastAllClosed = false;

  // Reset de status van de reed switches
  for (int i = 0; i < 7; i++) {
    lastReedStates[i] = digitalRead(reedSwitchPins[i]);
  }

  Serial.println("Systeem gereset. Wacht op startcommando.");
}

void connectToWiFi() {
  Serial.print("Verbinden met WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nVerbonden met WiFi.");
}
