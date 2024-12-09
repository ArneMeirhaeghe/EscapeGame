import mqttSingleton from "../lib/mqttSingleton.js";
import Timer from "../lib/timer.js";
import timerSingleton from "../lib/timerSingleton.js";

/**
 * Start the internal timer
 * @param {*} req
 * @param {*} res
 */
export const startTimer = (req, res) => {
	try {
		timerSingleton.getInstance().start();
		const message = JSON.stringify({ command: "start" });
		mqttSingleton.getClient().publish("mqtt/defcon/control", message);

		res.status(200).send("Timer started");
	} catch (e) {
		console.error(e);
	}
};

/**
 * Stop the internal timer
 * @param {*} req
 * @param {*} res
 */
export const stopTimer = (req, res) => {
	try {
		timerSingleton.getInstance().stop();
		const message = JSON.stringify({ command: "stop" });
		mqttSingleton.getClient().publish("mqtt/defcon/control", message);
		res.status(200).send("Timer stopped!");
	} catch (e) {
		console.error(e);
	}
};

export const resetGame = (req, res) => {
	try {
		timerSingleton.getInstance().stop();
		const message = JSON.stringify({ command: "reset" });
		mqttSingleton.getClient().publish("mqtt/defcon/control", message);
		// io.emit("timerTick", elapsedTime);
		res.status(200).send("Game reset!");
	} catch (e) {
		console.error(e);
	}
};
