// lib/mqttToWebSocketHandler.js

import mqttSingleton from "./mqttSingleton.js";

class MqttToWebSocketHandler {
  constructor(topic, socketServer, eventName) {
    this.topic = topic;
    this.socketServer = socketServer;
    this.eventName = eventName;
  }

  // Method to subscribe to the MQTT topic
  subscribe() {
    // Subscribe to the topic
    mqttSingleton.getClient().subscribe(this.topic, (err) => {
      if (err) {
        console.error(`Error subscribing to topic: ${this.topic}`, err);
      } else {
        console.log(`Successfully subscribed to topic: ${this.topic}`);
      }
    });

    // Register the message handler for the specific topic
    this.registerMessageHandler();
  }

  // Method to register the message handler for the topic
  registerMessageHandler() {
    // We use the topic to listen to the 'message' event dynamically
    mqttSingleton.getClient().client.on("message", (topic, message) => {
      if (topic === this.topic) {
        this.handleMessage(message);
      }
    });
  }

  // Method to handle incoming MQTT messages
  handleMessage(message) {
    try {
      // Convert the Buffer to string and parse as JSON
      const parsedMessage = JSON.parse(message.toString());
      // Emit the parsed message to the specified WebSocket event
      this.socketServer.emit(this.eventName, parsedMessage);
    } catch (error) {
      console.error(`Error parsing JSON from MQTT message on topic ${this.topic}:`, error);
    }
  }
}

export default MqttToWebSocketHandler;
