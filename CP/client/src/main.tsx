import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
// import './reset.css'
import "./index.css";
import App from "./App.tsx";
import { BrowserRouter, Route, Routes } from "react-router-dom";
import Hints from "./Hints.tsx";
// import Game from "./Components/Game.tsx";

createRoot(document.getElementById("root")!).render(
  <StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App />} />
        <Route path="/hints" element={<Hints />} />
        {/* <Route path="/game" element={<Game />} /> */}
      </Routes>
    </BrowserRouter>
  </StrictMode>
);
