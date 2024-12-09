import { useEffect, useState } from "react";
import io from "socket.io-client";
import { API_URL } from "../consts.ts";

export default function Terminal() {
  const [inputHint, setInputHint] = useState(""); // State to capture hint input
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

  const sendHint = async () => {
    if (inputHint.trim()) {
      const socket = io(API_URL); // Connect to WebSocket
      socket.emit("sendHint", inputHint); // Send the hint to the server
      setInputHint(""); // Clear the input field
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === "Enter") {
      e.preventDefault(); // Prevent new line insertion
      sendHint(); // Send hint when Enter is pressed
    }
  };

  return (
    <div className="timer-container">
      {/* Terminal-like output section */}
      <div className="flex items-center justify-center text-2xl mt-6 mb-4 text-primary font-terminal">Terminal</div>
      <div className="terminal-output mx-4 text-primary font-terminal">
        {hints.map((hint, index) => (
          <div key={index} className="hint-message">
            {/* Render message and timestamp */}
            <p>&gt; {hint.message}</p>
          </div>
        ))}
      </div>

      {/* Input section */}
      <div className="flex px-4 gap-2 mt-4">
        <div className="flex items-center justify-center text-primary font-terminal">daddyPi$</div>
        <input
          value={inputHint}
          onChange={(e) => setInputHint(e.target.value)}
          onKeyDown={handleKeyDown} // Listen for Enter key
          className="hint-input bg-transparent text-primary font-terminal w-full focus:outline-none"
        />
      </div>
    </div>
  );
}
