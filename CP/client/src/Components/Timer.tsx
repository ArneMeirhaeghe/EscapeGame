import { useEffect, useState } from "react";
import io from "socket.io-client";
import { API_URL } from "../consts.ts";
import mqtt from "mqtt";

export default function Timer() {
  const [currentTime, setCurrentTime] = useState("30:00");

  function formatTime(seconds: number) {
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = seconds % 60;
    const formattedMinutes = String(minutes).padStart(2, "0");
    const formattedSeconds = String(remainingSeconds).padStart(2, "0");
    return `${formattedMinutes}:${formattedSeconds}`;
  }

  useEffect(() => {
    const socket = io(API_URL);

    socket.on("timerTick", (data) => {
      setCurrentTime(formatTime(data));
    });

    return () => {
      socket.disconnect();
    };
  }, []);



  return <div className="text-9xl text-7SEG timer text-primary">{currentTime}</div>;
}
