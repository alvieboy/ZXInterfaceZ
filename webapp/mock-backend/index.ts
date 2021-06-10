import * as express from 'express';
import { Request, Response } from 'express';
import * as jsonServer from 'json-server';

const server = jsonServer.create();
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

server.use(router);

const {
  PORT = 3000,
} = process.env;

server.listen(PORT, () => {
  console.log('JSON server started at http://localhost:'+PORT);
});
