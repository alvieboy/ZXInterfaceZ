import ws from 'ws';
import { Request } from 'express';
import sha1 from 'sha1';

var status = [
  {
    action: 'Uploading',
    level: 'info',
    percent: 10,
    phase: 'Initializing',
  },
  {
    action: 'WARNING: Flashing sector 2',
    level: 'warning',
    percent: 30,
    phase: 'Programming OTA executable',
  },
  {
    action: 'ERROR: Flashing sector 7',
    level: 'error',
    percent: 60,
    phase: 'Programming OTA executable',
  },
  {
    action: 'Finalizing',
    level: 'info',
    percent: 98,
    phase: 'Validating',
  },
  {
    action: 'Finalizing',
    level: 'info',
    percent: 100,
    phase: 'Completed',
  }
];

var i = 0;
var buf = new ArrayBuffer(2048);

export function fwUpload(ws: ws, req: Request) {
  ws.on('message', (msg: any) => {
    console.log(msg);
    if (msg === 'START') {
      ws.send('OK');
      console.log('Starting upload');
    } else if (msg === 'STATUS') {
      ws.send(JSON.stringify(status[i++ % status.length]));
    } else { // FW file data
      console.log(`Recieved chunk of length: ${msg.length}, sha1: ${sha1(msg)}`);
    }
  });
}
