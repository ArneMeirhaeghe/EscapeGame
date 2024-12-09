// hooks/useSocket.js
import { useEffect, useState } from "react";
import { io, Socket } from "socket.io-client";

const useSocket = (url: string, event: string) => {
  const [data, setData] = useState(null);
  const [socket, setSocket] = useState<Socket | null>(null);

  useEffect(() => {
    const socketInstance = io(url);
    setSocket(socketInstance);

    // Listen for the event from the server and update the state
    socketInstance.on(event, (message) => {
      setData(message);
    });

    // Cleanup on unmount
    return () => {
      socketInstance.disconnect();
    };
  }, [url, event]);

  return data;
};

export default useSocket;
