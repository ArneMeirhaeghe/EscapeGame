import "./App.css";
import Challenge1 from "./Components/Challenges/Challenge1";
import Challenge2 from "./Components/Challenges/Challenge2";
import Challenge3 from "./Components/Challenges/Challenge3";
import Challenge4 from "./Components/Challenges/Challenge4";
import Challenge5 from "./Components/Challenges/Challenge5";
import Controls from "./Components/Controls";
import Terminal from "./Components/Terminal";
import Timer from "./Components/Timer";

function App() {
	return (
		<div className="grid">
			<div className="Title flex justify-center items-center text-4xl text-primary font-bold uppercase">
				Escape game
			</div>
			<div className="description p-6 flex flex-col justify-center">
				<h1 className="text-5xl uppercase font-bold mb-2">DEFCON-1</h1>
				<p>
					Lorem ipsum dolor sit amet, consectetur adipiscing elit. Duis sed lorem volutpat, molestie ipsum ut, aliquet
					leo. Vestibulum auctor sit amet ex at pharetra.
				</p>
			</div>
			<div className="control-panel flex justify-center items-center text-4xl font-bold uppercase">Control panel</div>
			<div className="game_status flex justify-center items-center text-4xl font-bold uppercase">Game started</div>
			<div className="terminal">
				<Terminal />
			</div>
			<div className="challenge1">
				<Challenge1 />
			</div>
			<div className="challenge2">
				<Challenge2 />
			</div>
			<div className="challenge3">
				<Challenge3 />
			</div>
			<div className="challenge4">
				<Challenge4 />
			</div>
			<div className="challenge5">
				<Challenge5 />
			</div>
			<div className="current_challenge"></div>
			<div className="timer font-deg7 font-bold italic flex items-center justify-center">
				<Timer />
			</div>
			<div className="video_feed"></div>
			<div className="controls">
				<Controls />
			</div>
			<div className="unknown1"></div>
			<div className="unknown2"></div>
			<div className="unknown3"></div>
			<div className="unknown4"></div>
		</div>
	);
}

export default App;
