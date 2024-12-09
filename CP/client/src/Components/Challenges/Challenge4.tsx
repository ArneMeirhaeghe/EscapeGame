import React, { useEffect, useState } from "react";
import { API_URL } from "../../consts";
import ChallengeCard from "./ChallengeCard";
import useSocket from "../../Hooks/useSocket";

interface ChallengeStep {
	step: string;
	status: string;
}

const Challenge4 = () => {
	const [connectedStatus, setConnectedStatus] = useState(false);
	const connectedData = useSocket(API_URL, "ws/defcon/ch4/connected");

	const [steps, setSteps] = useState<ChallengeStep[]>([
		{ step: "Players select the correct disk", status: "Pending" },
		{ step: "Players find the correct file on the disk", status: "Pending" },
	]);

	const data = useSocket(API_URL, "ws/defcon/ch4/status");

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
			console.log("Connected Data CH4: ", connectedData);
			setConnectedStatus(connectedData.connected);
		}
	});

	return (
		<ChallengeCard
			title="Floppy disk "
			challenge="Challenge 4"
			steps={steps}
			connected={connectedStatus}
		></ChallengeCard>
	);
};

export default Challenge4;
