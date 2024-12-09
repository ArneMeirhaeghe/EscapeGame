class MqttTopicHandler {
  constructor(client, subscribeTopic, controlTopic, controlMessageCondition, controlCommand) {
    this.client = client;
    this.subscribeTopic = subscribeTopic;
    this.controlTopic = controlTopic;
    this.controlMessageCondition = controlMessageCondition;
    this.controlCommand = controlCommand;
  }

  // Method to subscribe to a given topic
  subscribe() {
    this.client.subscribe(this.subscribeTopic);
    console.log(`Subscribed to topic: ${this.subscribeTopic}`);
  }

  // Method to handle incoming messages
  handleMessage(topic, message) {
    if (topic === this.subscribeTopic) {
      const parsedMessage = JSON.parse(message.toString());
      if (this.controlMessageCondition(parsedMessage)) {
        this.publishControlMessage();
      }
    }
  }

  // Method to publish control messages
  publishControlMessage() {
    const controlMessage = JSON.stringify({ command: this.controlCommand });
    this.client.publish(this.controlTopic, controlMessage);
    console.log(`Published to ${this.controlTopic}: ${controlMessage}`);
  }

  // Method to attach the message handler
  attachHandler() {
    this.client.on("message", (topic, message) => this.handleMessage(topic, message));
  }
}

export default MqttTopicHandler;
