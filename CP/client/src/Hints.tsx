import { useEffect, useState } from "react";
import { API_URL } from "./consts";
import { io } from "socket.io-client";

function Hints() {
	const [hints, setHints] = useState<{ message: string; timestamp: string }[]>([]); // Store sent hints

	useEffect(() => {
		// Fetch the hints when the component mounts or refreshes
		const fetchHints = async () => {
			try {
				const response = await fetch(`${API_URL}/hints`);
				const data = await response.json();
				setHints(data); // Set the fetched hints into the state
			} catch (error) {
				console.error("Error fetching hints:", error);
			}
		};

		// Fetch the hints
		fetchHints();

		// Initialize WebSocket connection
		const socket = io(API_URL);

		// Listen for new hints from the server (if any)
		socket.on("hintReceived", (hint: string) => {
			setHints((prevHints) => [...prevHints, { message: `${hint}`, timestamp: new Date().toLocaleString() }]); // Add received hint to the list
		});

		// Listen for all hints sent before the current client connected
		socket.on("allHints", (allHints: { message: string; timestamp: string }[]) => {
			setHints(allHints); // Populate all previous hints
		});

		// Clean up on unmount
		return () => {
			socket.disconnect();
		};
	}, []); // Empty dependency array to run only once on mount

	return (
		<div className="">
			{/* Terminal-like output section */}
			<div className="flex items-center justify-center text-6xl mt-6 mb-4 text-primary font-terminal">Terminal</div>
			<div className="mx-4 text-primary text-2xl font-terminal">
				{hints.map((hint, index) => (
					<div key={index} className="hint-message">
						<p>&gt; {hint.message}</p>
					</div>
				))}
			</div>
		</div>
	);
}

export default Hints;
