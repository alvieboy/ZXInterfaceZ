import ws from 'ws';
import { Request } from 'express';

export function fwUpload(ws: ws, req: Request) {
  ws.on('message', (msg: any) => {
    ws.send(msg);
  });
}
