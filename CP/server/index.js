// server.js or app.js
import express from "express";
import http from "http";
import dotenv from "dotenv";
import cors from "cors";
import { Server } from "socket.io";
import { resetGame, startTimer, stopTimer } from "./controllers/timer.js";
import timerSingleton from "./lib/timerSingleton.js";
import mqttSingleton from "./lib/mqttSingleton.js";
import MqttToWebSocketHandler from "./lib/mqttToWebSocketHandler.js"; // Import the class
import MqttTopicHandler from "./lib/mqttTopicHandler.js";

let hints = [];

dotenv.config();

// Create Express app
const app = express();

// Define the HTTP server
const httpServer = http.createServer(app);

// Enable CORS
app.use(cors());

// Socket IO
const io = new Server(httpServer, {
	cors: {
		origin: "*",
		methods: ["GET", "POST"],
	},
});

// For testing purposes
io.on("connection", (socket) => {
	console.log("New user connected!");
	socket.on("disconnect", function () {
		console.log("User disconnected...");
	});

	// Listen for hints sent by clients
	socket.on("sendHint", (hint) => {
		console.log("Received hint:", hint);
		hints.push({ message: hint, timestamp: new Date() }); // Store the hint
		io.emit("hintReceived", hint); // Broadcast the new hint to all clients
	});
});

// Function to stop Ruis
const stopRuis = () => {
	const message = JSON.stringify({ command: "stop_ruis" });
	mqttSingleton.getClient().publish("defcon/control", message);
};

// Handle the timer tick event
timerSingleton.getInstance().onTick = (elapsedTime) => {
	io.emit("timerTick", elapsedTime);

	const payload = JSON.stringify({ time: elapsedTime.toString() });
	mqttSingleton.getClient().publish("defcon/timer", payload);
};

// MQTT client connected and subscribing to radio topic
const subscribeToTopics = () => {
	const topicConfigs = [
		{
			subscribeTopic: "mqtt/defcon/ch1/status",
			controlTopic: "mqtt/defcon/ch2/control",
			condition: (message) => message.status === "completed", // Condition for ch1
			controlCommand: "start", // Control command for ch2
		},
		{
			subscribeTopic: "mqtt/defcon/ch2/status",
			controlTopic: "mqtt/defcon/ch3/control",
			condition: (message) => message.status === "completed", // Condition for ch2
			controlCommand: "start", // Control command for ch3
		},
		{
			subscribeTopic: "mqtt/defcon/ch3/status",
			controlTopic: "mqtt/defcon/ch5/control",
			condition: (message) => message.status === "completed", // Condition for ch3
			controlCommand: "start", // Control command for ch4
		},
	];

	// Create instances of MqttToWebSocketHandler for each topic and event
	const ch1Handler = new MqttToWebSocketHandler("mqtt/defcon/ch1/steps/status", io, "ws/defcon/ch1/steps/status");
	const ch2Handler = new MqttToWebSocketHandler("mqtt/defcon/ch2/steps/status", io, "ws/defcon/ch2/steps/status");
	const ch3Handler = new MqttToWebSocketHandler("mqtt/defcon/ch3/steps/status", io, "ws/defcon/ch3/steps/status");
	const ch4Handler = new MqttToWebSocketHandler("mqtt/defcon/ch4/steps/status", io, "ws/defcon/ch4/steps/status");
	const ch5Handler = new MqttToWebSocketHandler("mqtt/defcon/ch5/steps/status", io, "ws/defcon/ch5/steps/status");

	const ch1StatusHandler = new MqttToWebSocketHandler("mqtt/defcon/ch1/status", io, "ws/defcon/ch1/status");

	const ch1ConnectedHandler = new MqttToWebSocketHandler("mqtt/defcon/ch1/connected", io, "ws/defcon/ch1/connected");
	const ch2ConnectedHandler = new MqttToWebSocketHandler("mqtt/defcon/ch2/connected", io, "ws/defcon/ch2/connected");
	const ch3ConnectedHandler = new MqttToWebSocketHandler("mqtt/defcon/ch3/connected", io, "ws/defcon/ch3/connected");
	const ch4ConnectedHandler = new MqttToWebSocketHandler("mqtt/defcon/ch4/connected", io, "ws/defcon/ch4/connected");
	const ch5ConnectedHandler = new MqttToWebSocketHandler("mqtt/defcon/ch5/connected", io, "ws/defcon/ch5/connected");

	// Subscribe to the topics
	ch1Handler.subscribe();
	ch2Handler.subscribe();
	ch3Handler.subscribe();
	ch4Handler.subscribe();
	ch5Handler.subscribe();

	ch1StatusHandler.subscribe();

	ch1ConnectedHandler.subscribe();
	ch2ConnectedHandler.subscribe();
	ch3ConnectedHandler.subscribe();
	ch4ConnectedHandler.subscribe();
	ch5ConnectedHandler.subscribe();

	topicConfigs.forEach((config) => {
		const mqttHandler = new MqttTopicHandler(
			mqttSingleton.getClient().client,
			config.subscribeTopic,
			config.controlTopic,
			config.condition,
			config.controlCommand
		);

		mqttHandler.subscribe();
		mqttHandler.attachHandler();
	});
};

// Define routes
app.get("/", (req, res) => {
	res.send("The LED API is working!");
});
app.get("/startGame", startTimer);
app.get("/stopGame", stopTimer);
app.get("/resetGame", resetGame);
app.get("/stopRuis", stopRuis);
app.get("/hints", (req, res) => {
	res.json(hints); // Return the stored hints as JSON
});

// Start server
const PORT = process.env.PORT || 3001;
httpServer.listen(PORT, () => {
	console.log(`Server listening on port ${PORT}`);
	// Subscribe to the topics when the server starts
	subscribeToTopics();
});

// Export app for testing purposes
export default app;
