import Timer from './timer.js';

const timerSingleton = (() => {
  let instance;

  const createInstance = () => {
    const timer = new Timer();
    return timer;
  };

  return {
    getInstance: () => {
      if (!instance) {
        instance = createInstance();
      }
      return instance;
    },
  };
})();

export default timerSingleton;
