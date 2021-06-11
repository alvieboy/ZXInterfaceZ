import express from 'express';
import { Request, Response } from 'express';
import jsonServer from 'json-server';
import ws from 'ws';
import expressWs from 'express-ws';

const {
  PORT = 3000,
} = process.env;

const instance = expressWs(jsonServer.create());
const server = instance.app;
const router = jsonServer.router('db.json');
const middlewares = jsonServer.defaults();

server.use(jsonServer.bodyParser)
server.use(jsonServer.rewriter({
  '/req/list\\?path=/': '/_sdcard_root',
  '/req/list\\?path=/a-b/SCRSHOT': '/_sdcard_a-b_SCRSHOT',
  '/req/list\\?path=/:path': '/_sdcard_:path',
  '/req/*': '/$1',
  '/upload/*': '/$1'
}));
server.use(middlewares);

server.get('/foo', (req: Request, res: Response) => {
  res.send({
    message: 'hello YOU',
  });
});

server.ws('/echo', (ws: ws, req: Request) => {
  ws.on('message', (msg: any) => {
    ws.send(msg);
  });
});

server.use(router);

server.listen(PORT, () => {
  console.log('JSON server started at http://localhost:'+PORT);
});
