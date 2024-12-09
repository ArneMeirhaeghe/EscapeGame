import { useEffect, useState } from "react";
import io from "socket.io-client";
import { API_URL } from "../consts.ts";
import mqtt from "mqtt";

export default function Controls() {
	const start = () => {
		fetch(`${API_URL}/startGame`);
	};
	const stop = () => fetch(`${API_URL}/stopGame`);

	const reset = () => fetch(`${API_URL}/resetGame`);

	return (
		<div className="p-4 flex flex-col gap-4">
			<button className="w-full bg-green-500 rounded-none uppercase" onClick={start}>
				Start
			</button>
			<button className="w-full bg-amber-500 rounded-none uppercase" onClick={reset}>
				reset
			</button>
			<button className="w-full bg-rose-500 rounded-none uppercase" onClick={stop}>
				Stop
			</button>
		</div>
	);
}
