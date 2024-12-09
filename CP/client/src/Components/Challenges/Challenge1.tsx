// Challenge1.js
import React, { useEffect, useState } from "react";
import { API_URL } from "../../consts";
import ChallengeCard from "./ChallengeCard";
import useSocket from "../../Hooks/useSocket";

interface ChallengeStep {
	step: string;
	status: string;
}

const Challenge1 = () => {
	const [steps, setSteps] = useState<ChallengeStep[]>([
		{ step: "Turn the radio on", status: "Pending" },
		{ step: "Players adjust the antenna", status: "Pending" },
		{ step: "Broadcast has been successfully received", status: "Pending" },
		{ step: "Morse code message is received", status: "Pending" },
	]);

	const [challengeStatus, setChallengeStatus] = useState("Undefined");
	const [connectedStatus, setConnectedStatus] = useState(false);

	const data = useSocket(API_URL, "ws/defcon/ch1/steps/status");
	const challengeData = useSocket(API_URL, "ws/defcon/ch1/status");
	const connectedData = useSocket(API_URL, "ws/defcon/ch1/connected");

	useEffect(() => {
		if (data) {
			console.log("Radio Control Data: ", data);
			const updatedStep = data.step;
			const updatedStatus = data.status;

			setSteps((prevSteps) => {
				return prevSteps.map((step) => (step.step === updatedStep ? { ...step, status: updatedStatus } : step));
			});
		}
	}, [data]);

	useEffect(() => {
		if (challengeData) {
			console.log("Challenge Data: ", challengeData);
			setChallengeStatus(challengeData.status);
		}
	}, [challengeData]);

	useEffect(() => {
		if (connectedData) {
			console.log("Connected Data CH1: ", connectedData.connected);
			setConnectedStatus(connectedData.connected);
		}
	});

	return (
		<ChallengeCard
			title="Radio Hack"
			challenge="Challenge 1"
			steps={steps}
			status={challengeStatus}
			connected={connectedStatus}
		></ChallengeCard>
	);
};

export default Challenge1;
