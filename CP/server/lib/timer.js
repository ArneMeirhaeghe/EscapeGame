let timerInstance = null;

class Timer {
  #onTick = null;

  constructor() {
    this.elapsedTime = 1800;
    this.timerId = null;
  }

  set onTick(onTick) {
    console.log('tick');
    this.#onTick = onTick;
  }

  start() {
    if (this.timerId) return; // Prevent multiple intervals from being created

    this.timerId = setInterval(() => {
      this.elapsedTime -= 1;

      if (this.#onTick) {
        this.#onTick(this.elapsedTime);
      }

      if (this.elapsedTime <= 0) {
        this.stop(); // Stop the timer when elapsedTime reaches 0
      }
    }, 1000);
  }

  stop() {
    clearInterval(this.timerId);
    this.timerId = null; // Clear the timer ID
    this.elapsedTime = 1800; // Reset to initial value
    if (this.#onTick) {
      this.#onTick(1800);
    }
  }

  static getInstance() {
    if (timerInstance === null) {
      timerInstance = new Timer();
    }

    return timerInstance;
  }
}

export default Timer;
