import React, { useEffect, useRef, useState } from "react";
import levels from "../assets/levels.json";
import "./game.css";

const GAME_WIDTH = 1920;
const GAME_HEIGHT = 1080;

const Game: React.FC = () => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [currentPosition, setCurrentPosition] = useState({ x: 100, y: 100 });
  const [currentDirection, setCurrentDirection] = useState({ x: 1, y: 0 }); // Set initial direction to move right
  const [currentLevelIndex, setCurrentLevelIndex] = useState(0);
  const [gameState, setGameState] = useState("start");
  const [walls, setWalls] = useState(levels[0].walls);
  const [trail, setTrail] = useState<{ x: number; y: number }[]>([]);
  const [isMusicPlaying, setIsMusicPlaying] = useState(false);
  const [keyMapping, setKeyMapping] = useState({
    up: "",
    down: "",
    left: "",
    right: "",
  });
  const [bgImage, setBgImage] = useState<HTMLImageElement | null>(null); // State for background image
  const [startImage, setStartImage] = useState<HTMLImageElement | null>(null);
  const [endImage, setEndImage] = useState<HTMLImageElement | null>(null);
  const [keyPressSound, setKeyPressSound] = useState<HTMLAudioElement | null>(null);
  const [backgroundMusic, setBackgroundMusic] = useState<HTMLAudioElement | null>(null);
  const [completionSound, setCompletionSound] = useState<HTMLAudioElement | null>(null);
  const [editButton, setEditButton] = useState(false);

  const playerSize = { width: 22, height: 22 };

  useEffect(() => {
    setKeyPressSound(new Audio("/keyPress.mp3"));
    setBackgroundMusic(new Audio("/backgroundMusic.mp3"));
    setCompletionSound(new Audio("/completion.mp3"));

    window.addEventListener("resize", resizeCanvas);
    resizeCanvas();

    return () => {
      window.removeEventListener("resize", resizeCanvas);
    };
  }, []);

  useEffect(() => {
    if (backgroundMusic) {
      backgroundMusic.loop = true;
      backgroundMusic.volume = 0.5;
    }
  }, [backgroundMusic]);

  useEffect(() => {
    if (gameState === "running") {
      const interval = setInterval(movePlayer, 20);
      return () => clearInterval(interval);
    }
  }, [gameState, currentPosition, currentDirection]);

  useEffect(() => {
    const newStartImage = new Image();
    newStartImage.src = "/start.png"; // Replace with your actual path to the start image
    newStartImage.onload = () => {
      setStartImage(newStartImage);
    };
  }, [currentLevelIndex]);

  useEffect(() => {
    const newEndImage = new Image();
    newEndImage.src = "/end.png"; // Replace with your actual path to the start image
    newEndImage.onload = () => {
      setEndImage(newEndImage);
    };
  }, [currentLevelIndex]);

  useEffect(() => {
    if (editButton) {
      const element = document.getElementById("play-music-button");
      if (element) {
        element.click();
      }
    }
  }, [editButton]);

  const resizeCanvas = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const aspectRatio = GAME_WIDTH / GAME_HEIGHT;
    const windowWidth = window.innerWidth;
    const windowHeight = window.innerHeight;
    const padding = 220;

    let newWidth, newHeight;
    if (windowWidth / windowHeight > aspectRatio) {
      newHeight = windowHeight - padding;
      newWidth = newHeight * aspectRatio;
    } else {
      newWidth = windowWidth - padding;
      newHeight = newWidth / aspectRatio;
    }

    canvas.width = GAME_WIDTH;
    canvas.height = GAME_HEIGHT;
    canvas.style.width = `${newWidth}px`;
    canvas.style.height = `${newHeight}px`;
    canvas.style.margin = `${padding / 2}px`;

    drawGame();
  };

  const playBackgroundMusic = () => {
    if (!backgroundMusic) return;

    backgroundMusic
      .play()
      .then(() => {
        setIsMusicPlaying(true);
      })
      .catch((error) => {
        console.error("Error playing background music:", error);
      });
  };

  const playRandomizedPitchSound = () => {
    if (!keyPressSound) return;

    keyPressSound.playbackRate = Math.random() * 0.4 + 1.2;
    keyPressSound.play();
  };

  const drawGame = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    // Draw the background
    if (bgImage) {
      ctx.drawImage(bgImage, 0, 0, canvas.width, canvas.height); // Draw background image
    }

    drawWalls(); // Draw walls after the background
    drawTrail(); // Draw trail before the player
    drawPlayer(); // Draw player after the trail

    drawStartPoint(); // Draw start point image
    drawEndPoint(); // Draw end point image
  };

  function drawTrail() {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    ctx.fillStyle = "rgba(47, 170, 112, 1)"; // Color and transparency for the trail
    trail.forEach((pos) => {
      const scaledX = (pos.x / GAME_WIDTH) * canvas.width;
      const scaledY = (pos.y / GAME_HEIGHT) * canvas.height;
      const playerWidth = (playerSize.width / GAME_WIDTH) * canvas.width;

      ctx.beginPath();
      ctx.arc(scaledX, scaledY, playerWidth / 5, 0, Math.PI * 5);
      ctx.fill();
    });
  }

  function drawWalls() {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    walls.forEach((wall) => {
      const scaledX = (wall.x / GAME_WIDTH) * canvas.width;
      const scaledY = (wall.y / GAME_HEIGHT) * canvas.height;
      const scaledWidth = (wall.width / GAME_WIDTH) * canvas.width;
      const scaledHeight = (wall.height / GAME_HEIGHT) * canvas.height;
      const borderRadius = 8; // Adjust this value for the desired border radius

      ctx.fillStyle = "rgba(255, 0, 0, 0)"; // Set fill color
      drawRoundedRect(scaledX, scaledY, scaledWidth, scaledHeight, borderRadius); // Draw the wall with rounded corners
    });
  }

  function drawRoundedRect(x: number, y: number, width: number, height: number, borderRadius: number) {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    ctx.beginPath();
    ctx.moveTo(x + borderRadius, y); // Move to the top-left corner

    // Top side
    ctx.lineTo(x + width - borderRadius, y);
    // Top-right corner
    ctx.arcTo(x + width, y, x + width, y + borderRadius, borderRadius);
    // Right side
    ctx.lineTo(x + width, y + height - borderRadius);
    // Bottom-right corner
    ctx.arcTo(x + width, y + height, x + width - borderRadius, y + height, borderRadius);
    // Bottom side
    ctx.lineTo(x + borderRadius, y + height);
    // Bottom-left corner
    ctx.arcTo(x, y + height, x, y + height - borderRadius, borderRadius);
    // Left side
    ctx.lineTo(x, y + borderRadius);
    // Top-left corner
    ctx.arcTo(x, y, x + borderRadius, y, borderRadius);

    ctx.closePath();
    ctx.fill();
  }

  function drawPlayer() {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const playerWidth = (playerSize.width / GAME_WIDTH) * canvas.width;

    const scaledX = (currentPosition.x / GAME_WIDTH) * canvas.width;
    const scaledY = (currentPosition.y / GAME_HEIGHT) * canvas.height;

    ctx.fillStyle = "rgb(47, 170, 112)";
    ctx.beginPath();
    ctx.arc(scaledX, scaledY, playerWidth / 2, 0, Math.PI * 2);
    ctx.fill();
  }

  function drawStartPoint() {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const imageWidth = 75; // Actual width of the image
    const imageHeight = 100; // Actual height of the image

    const scaledX = (levels[currentLevelIndex].startCoords.x + imageWidth / 2 / GAME_WIDTH) * canvas.width;
    const scaledY = (levels[currentLevelIndex].startCoords.y / GAME_HEIGHT) * canvas.height;

    // Draw the image centered at the start point
    if (startImage) {
      ctx.drawImage(startImage, scaledX - imageWidth / 2, scaledY - imageHeight / 2, imageWidth, imageHeight);
    }
  }

  function drawEndPoint() {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const imageWidth = 75; // Actual width of the image
    const imageHeight = 100; // Actual height of the image

    const scaledX = ((levels[currentLevelIndex].endCoords.x - imageWidth / 2) / GAME_WIDTH) * canvas.width; // No need to adjust for image width
    const scaledY = (levels[currentLevelIndex].endCoords.y / GAME_HEIGHT) * canvas.height;

    // Save the current context state
    ctx.save();

    // Move the origin to the center of the end point
    ctx.translate(scaledX, scaledY);

    // Rotate the context by 180 degrees (PI radians)
    const rotationRadians = levels[currentLevelIndex].endCoords.rotation * (Math.PI / 180); // Convert to radians

    // Rotate the context
    ctx.rotate(rotationRadians); // Rotate by the specified angle

    // Draw the image centered at the new origin
    if (endImage) {
      ctx.drawImage(endImage, -imageWidth / 2, -imageHeight / 2, imageWidth, imageHeight);
    }

    // Restore the context to its original state
    ctx.restore();
  }

  const movePlayer = () => {
    setTrail((prevTrail) => [...prevTrail, { ...currentPosition }]);
    setCurrentPosition((prevPosition) => ({
      x: prevPosition.x + currentDirection.x * levels[currentLevelIndex].speed,
      y: prevPosition.y + currentDirection.y * levels[currentLevelIndex].speed,
    }));

    if (checkWallCollision() || checkBounds()) {
      resetGame();
    } else if (checkEndPoint()) {
      advanceToNextLevel();
    }
    drawGame();
  };

  const checkWallCollision = () => {
    const playerX = currentPosition.x;
    const playerY = currentPosition.y;
    return walls.some(
      (wall) => playerX > wall.x && playerX < wall.x + wall.width && playerY > wall.y && playerY < wall.y + wall.height
    );
  };

  const checkBounds = () => {
    return (
      currentPosition.x < 0 ||
      currentPosition.x > GAME_WIDTH ||
      currentPosition.y < 0 ||
      currentPosition.y > GAME_HEIGHT
    );
  };

  const checkEndPoint = () => {
    const playerX = currentPosition.x;
    const playerY = currentPosition.y;
    const endCoords = levels[currentLevelIndex].endCoords;
    const hitBoxSize = 50;
    return (
      playerX > endCoords.x - hitBoxSize &&
      playerX < endCoords.x + hitBoxSize &&
      playerY > endCoords.y - hitBoxSize &&
      playerY < endCoords.y + hitBoxSize
    );
  };

  const resetGame = () => {
    setCurrentPosition({ ...levels[currentLevelIndex].startCoords });
    setCurrentDirection({ x: 1, y: 0 });
    setTrail([]);
    setGameState("running");
  };

  const advanceToNextLevel = () => {
    if (currentLevelIndex < levels.length - 1) {
      setCurrentLevelIndex((prevIndex) => {
        const newIndex = prevIndex + 1;
        setKeyMapping(getRandomKeyMapping());
        resetPlayerPosition(newIndex);
        return newIndex;
      });
    } else {
      displayEndMessage();
    }
  };

  const resetPlayerPosition = (levelIndex: number) => {
    setCurrentPosition({ ...levels[levelIndex].startCoords });
    setCurrentDirection({ x: 1, y: 0 }); // Reset direction to move right
    setWalls([...levels[levelIndex].walls]);
    setTrail([]);
    loadBackgroundImage(levelIndex);
    drawGame();
  };

  const loadBackgroundImage = (levelIndex: number) => {
    const newBgImage = new Image();
    newBgImage.src = levels[levelIndex].backgroundImage;
    newBgImage.onload = () => {
      setBgImage(newBgImage); // Set the loaded image to state
      drawGame(); // Draw the game after the background image is loaded
    };
  };

  const playCompletionSound = () => {
    if (!completionSound) return;

    completionSound.play();
  };

  const stopSounds = () => {
    if (backgroundMusic) {
      backgroundMusic.pause();
      backgroundMusic.currentTime = 0;
      setIsMusicPlaying(false);
    }
  };

  const displayEndMessage = () => {
    setGameState("end");
    playCompletionSound();
    stopSounds();
  };

  const startGame = () => {
    setEditButton(true);
    setKeyMapping(getRandomKeyMapping());
    setGameState("running");
    resetPlayerPosition(currentLevelIndex);
  };

  const handleKeyDown = (event: KeyboardEvent) => {
    const { up, down, left, right } = keyMapping;

    if (gameState === "start") {
      startGame();
    } else {
      switch (event.code) {
        case up:
          setCurrentDirection({ x: 0, y: -1 });
          playRandomizedPitchSound();
          break;
        case down:
          setCurrentDirection({ x: 0, y: 1 });
          playRandomizedPitchSound();
          break;
        case left:
          setCurrentDirection({ x: -1, y: 0 });
          playRandomizedPitchSound();
          break;
        case right:
          setCurrentDirection({ x: 1, y: 0 });
          playRandomizedPitchSound();
          break;
      }
    }
  };

  const getRandomKeyMapping = () => {
    const keys = ["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"];
    const shuffledKeys = keys.sort(() => Math.random() - 0.5); // Shuffle the keys randomly

    return {
      up: shuffledKeys[0],
      down: shuffledKeys[1],
      left: shuffledKeys[2],
      right: shuffledKeys[3],
    };
  };

  useEffect(() => {
    document.addEventListener("keydown", handleKeyDown);
    return () => {
      document.removeEventListener("keydown", handleKeyDown);
    };
  }, [gameState, currentLevelIndex, currentDirection]);

  return (
    <div id="game-container">
      <canvas id="game-canvas" ref={canvasRef} className={gameState === "start" ? "hidden" : ""}></canvas>
      {gameState === "start" && (
        <div id="start-screen" className="screen">
          <button id="start-button" onClick={startGame}>
            Press any button to start the hack
          </button>
        </div>
      )}
      {gameState === "end" && (
        <div id="end-screen" className="screen">
          <h1 id="end-message">You've reached the endpoint!</h1>
          <button id="restart-button" onClick={() => setGameState("start")}>
            Restart Game
          </button>
        </div>
      )}
      {gameState === "end" && (
        <div id="completion-screen" className="screen">
          <img src="https://pbs.twimg.com/media/GNmnQ8LXUAAcPhJ.png" alt="Completion Image" />
        </div>
      )}
      <button id="play-music-button" onClick={playBackgroundMusic} style={{ display: "none" }}></button>
      <div className="scanline"></div>
    </div>
  );
};

export default Game;
