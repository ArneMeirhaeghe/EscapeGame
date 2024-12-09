import React, { useEffect, useState } from "react";
import { API_URL } from "../../consts";
import ChallengeCard from "./ChallengeCard";
import useSocket from "../../Hooks/useSocket";

interface ChallengeStep {
	step: string;
	status: string;
}

const Challenge3 = () => {
	const [connectedStatus, setConnectedStatus] = useState(false);
	const connectedData = useSocket(API_URL, "ws/defcon/ch3/connected");

	const [steps, setSteps] = useState<ChallengeStep[]>([
		{ step: "Players align the disks correctly", status: "Pending" },
		{ step: "Compartiment open", status: "Pending" },
	]);

	const data = useSocket(API_URL, "ws/defcon/ch3/status");

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
			console.log("Connected Data CH3: ", connectedData);
			setConnectedStatus(connectedData.connected);
		}
	});

	return (
		<ChallengeCard
			title="Rotating disks"
			challenge="Challenge 3"
			steps={steps}
			connected={connectedStatus}
		></ChallengeCard>
	);
};

export default Challenge3;
