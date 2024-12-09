import MQTT from './mqtt.js';
import dotenv from 'dotenv';
dotenv.config();

let instance = null;

class MQTTSingleton {
  constructor() {
    if (!instance) {
      instance = this;
      this.client = new MQTT({
        brokerUrl: process.env.MQTT_BROKER_URL || 'http://192.168.1.100:1883',
      });
    }

    return instance;
  }

  /**
   * Get the MQTT client
   * @returns
   */
  getClient() {
    return this.client;
  }
}

export default new MQTTSingleton();
