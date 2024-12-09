import mqtt from 'mqtt';

class MQTT {
  constructor(options) {
    this.client = mqtt.connect(options.brokerUrl, options.options);
    this.client.on('connect', () => {
      console.log(`Connected to MQTT broker at ${options.brokerUrl}`);
    });
    this.client.on('message', (topic, message) => {
      console.log(`Received message on topic ${topic}: ${message.toString()}`);
    });
  }

  /**
   * Subscribe to a topic
   * @param {*} topic
   */
  subscribe(topic) {
    this.client.subscribe(topic, (err) => {
      if (err) {
        console.error(`Error subscribing to topic ${topic}: ${err}`);
      } else {
        console.log(`Subscribed to topic ${topic}`);
      }
    });
  }

  /**
   * Subscribe to a topic, and return the first message received on that topic
   * @param {*} topic
   * @returns
   */
  async subscribeOnce(topic) {
    return new Promise((resolve, reject) => {
      // create a handler that will be called whenever we received a message on the topic
      // immediately unsubscribe from the topic, and resolve the promise with the message
      const handler = (t, message) => {
        if(t !== topic) return;
        const m = message.toString();
        this.client.unsubscribe(topic, handler);
        resolve(m);
      };

      // subscribe to the topic, and register the handler
      this.client.subscribe(topic, (err) => {
        if (err) {
          console.error(`Error subscribing to topic ${topic}: ${err}`);
          reject(err);
        } else {
          console.log(`Subscribed to topic ${topic}`);
          this.client.on('message', handler);
        }
      });
    });
  }

  /**
   * Publish a message to a topic
   * @param {*} topic
   * @param {*} message
   */
  publish(topic, message) {
    this.client.publish(topic, message, (err) => {
      if (err) {
        console.error(`Error publishing message to topic ${topic}: ${err}`);
      } else {
        console.log(`Published message to topic ${topic}: ${message}`);
      }
    });
  }

  /**
   * Disconnect from the MQTT broker
   */
  disconnect() {
    this.client.end();
    console.log(`Disconnected from MQTT broker`);
  }
}

export default MQTT;
