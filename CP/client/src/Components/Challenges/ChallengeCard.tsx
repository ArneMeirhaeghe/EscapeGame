// ChallengeCard.js
import React from "react";
import Tag from "../Tag";

interface ChallengeStep {
	step: string;
	status: string;
}

interface ChallengeCardProps {
	title: string;
	challenge: string;
	steps: ChallengeStep[];
	status: string;
	connected: string;
	children?: React.ReactNode;
	event: string; // Event name to listen for
	eventHandler: (data: any) => void; // Function to handle the data received from the WebSocket event
}

const ChallengeCard: React.FC<ChallengeCardProps> = ({ title, challenge, steps, status, connected = false, children }) => {
	return (
		<div className="p-4">
			<p className="capitalize text-center flex gap-2 items-center justify-center pb-2">
				{connected ? (
					<>
						<svg
							xmlns="http://www.w3.org/2000/svg"
							width="16"
							height="16"
							fill="#000000"
							className="bi bi-lock"
							viewBox="0 0 16 16"
						>
							<path
								d="M8 1a2 2 0 0 1 2 2v4H6V3a2 2 0 0 1 2-2m3 6V3a3 3 0 0 0-6 0v4a2 2 0 0 0-2 2v5a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2M5 8h6a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H5a1 1 0 0 1-1-1V9a1 1 0 0 1 1-1"
								fill="#00F38A"
							/>
						</svg>
						Connected
					</>
				) : (
					<>
						<svg
							xmlns="http://www.w3.org/2000/svg"
							width="16"
							height="16"
							fill="#000000"
							className="bi bi-unlock"
							viewBox="0 0 16 16"
						>
							<path
								d="M11 1a2 2 0 0 0-2 2v4a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V9a2 2 0 0 1 2-2h5V3a3 3 0 0 1 6 0v4a.5.5 0 0 1-1 0V3a2 2 0 0 0-2-2M3 8a1 1 0 0 0-1 1v5a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1V9a1 1 0 0 0-1-1z"
								fill="#f43f5e"
							/>
						</svg>
						Disconnected
					</>
				)}
			</p>
			<h2 className="font-bold text-xl text-center uppercase text-primary">{title}</h2>
			<p className="font-bold text-4xl text-center uppercase">{challenge}</p>
			<ul className="challenge-card-steps pt-4">
				{/* {steps.map((stepData, index) => (
					<li key={index} className="flex items-center pb-2 mb-2 border-b border-primary border-opacity-10 h-14">
						<div className="inline-flex items-center">
							<label className="flex items-center cursor-pointer relative">
								<input
									type="checkbox"
									checked={stepData.status === "Completed"} // Controlled checkbox state
									disabled // Disable the checkbox to prevent user interaction
									className="peer h-5 w-5 cursor-pointer transition-all appearance-none rounded shadow hover:shadow-md border-2 border-primary checked:bg-primary checked:border-primary"
									id={`check-${index}`} // Unique ID for each checkbox
								/>
								<span className="absolute text-white opacity-0 peer-checked:opacity-100 top-1/2 left-1/2 transform -translate-x-1/2 -translate-y-1/2 pointer-events-none">
									<svg
										xmlns="http://www.w3.org/2000/svg"
										className="h-3.5 w-3.5"
										viewBox="0 0 20 20"
										fill="currentColor"
										stroke="currentColor"
										strokeWidth="1"
									>
										<path
											fillRule="evenodd"
											d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z"
											clipRule="evenodd"
										></path>
									</svg>
								</span>
							</label>
						</div>
						<label htmlFor={`check-${index}`} className="mx-2 flex-grow">
							{stepData.step}
						</label>
						<Tag status={stepData.status} />
					</li>
				))} */}
			</ul>
			<div className="challenge-card-children">
				{children}
				<div className="flex flex-col items-center mt-2">
					<h2>Status:</h2>
					<p>{status ? status : "undefined"}</p>
				</div>
			</div>
		</div>
	);
};

export default ChallengeCard;
