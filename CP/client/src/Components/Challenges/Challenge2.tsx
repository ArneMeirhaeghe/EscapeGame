import React, { useEffect, useState } from "react";
import { API_URL } from "../../consts";
import ChallengeCard from "./ChallengeCard";
import useSocket from "../../Hooks/useSocket";

interface ChallengeStep {
	step: string;
	status: string;
}

const Challenge2 = () => {
	const [connectedStatus, setConnectedStatus] = useState(false);
	const connectedData = useSocket(API_URL, "ws/defcon/ch2/connected");

	const [steps, setSteps] = useState<ChallengeStep[]>([
		{ step: "Adjust the switches to match the binary code", status: "Pending" },
		{ step: "Compartment is now accessible", status: "Pending" },
	]);

	const data = useSocket(API_URL, "ws/defcon/ch2/status");

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
		if (connectedData) {
			console.log("Connected Data CH: ", connectedData);
			setConnectedStatus(connectedData.connected);
		}
	});

	return (
		<ChallengeCard
			title="Binary Switches"
			challenge="Challenge 2"
			steps={steps}
			connected={connectedStatus}
		></ChallengeCard>
	);
};

export default Challenge2;
