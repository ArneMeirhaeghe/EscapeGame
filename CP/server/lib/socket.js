import { Server as SocketServer} from 'socket.io';
import http from 'http';

export default class SocketWrapper {
  constructor() {
    const server = http.createServer();
    this.io = new SocketServer(3002);
    this.clients = new Set();

    this.io.on('connection', (socket) => {
      console.log(`Client ${socket.id} connected`);
      this.clients.add(socket);

      socket.on('disconnect', () => {
        console.log(`Client ${socket.id} disconnected`);
        this.clients.delete(socket);
      });
    });
  }

  emitToAll(event, data) {
    this.clients.forEach((socket) => {
      socket.emit(event, data);
    });
  }
}
