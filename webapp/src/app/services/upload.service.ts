import { Injectable } from '@angular/core';

import { Observable, from, timer, of } from 'rxjs';
import { takeWhile, skipWhile, delay, concatMap } from 'rxjs/operators';
import { WebSocketSubject } from 'rxjs/webSocket';

import { FwUploadStatus, Level } from '../models/FwUploadStatus';

@Injectable()
export class UploadService {

  private socket: WebSocketSubject<FwUploadStatus | string | ArrayBuffer>;
  private uploading = false;
  private completed = false;

  uploadFirmware(fwfile: File): WebSocketSubject<FwUploadStatus | string | ArrayBuffer> { // TODO remove string and ArrayBuffer
    var protocol = window.location.protocol.replace('http', 'ws');
    var url = `${protocol}//${window.location.host}/ws/fwupgrade`;
    console.log(`WS connecting: ${url}`);

    this.socket = new WebSocketSubject({
      url: url,
      serializer: serializer,
      deserializer: deserializer,
      binaryType: 'arraybuffer'
    });

    console.log("WS connected");

    this.socket.subscribe(
      (message) => {
        console.log(message);
        if (message == "OK") {
          this.uploading = true;
          this.sendFile(fwfile);
        }
        if (isStatusMessage(message)) {
          this.completed = message.percent >= 100;
        }

      },
      (err) => console.error(err),
      () => console.warn('Completed!')
    );

    this.socket.next('START');

    return this.socket;
  }

  sendFile(fwfile: File) {

    timer(500, 1000).pipe(
      takeWhile(_ => !this.completed)
    ).subscribe(_ => {
        console.log('Requesting status');
        this.socket.next('STATUS');
      }
    );

    console.log("here");
    var i = 0;
    this.readFile(fwfile).then(chunks => {
      from(chunks).pipe(
        concatMap(chunk => of(chunk).pipe(delay(5))) // 512 * 1000/5 => 100 kB/s
      ).subscribe(
        (chunk) => {
          this.socket.next(chunk);
          console.log(`Sent chunk ${i++}`);
        },
        () => this.uploading = false
      )
    });
  }

  readFile(fwfile: File): Promise<ArrayBuffer[]> {
    var CHUNK_SIZE = 512;

    return fwfile.arrayBuffer().then(f => {
      var data: ArrayBuffer[] = [];
      for (var i = 0; i < Math.ceil(f.byteLength / CHUNK_SIZE); i++) {
        data[i] = f.slice(i*CHUNK_SIZE, (i+1)*CHUNK_SIZE);
      };
      console.log(data.length);
      return data;
    });
  }
}

function serializer(value: FwUploadStatus | string | ArrayBuffer): string | ArrayBuffer {
  if (isStatusMessage(value)) {
    return JSON.stringify(value);
  }
  if (value instanceof ArrayBuffer) {
    return value;
  }
  return value;
}

function deserializer(message: MessageEvent): FwUploadStatus | string | ArrayBuffer {
  try {
    return JSON.parse(message.data);
  } catch(e) {
    return message.data;
  }
}

function isStatusMessage(message: FwUploadStatus | string | ArrayBuffer): message is FwUploadStatus {
  return (message as FwUploadStatus).level !== undefined;
}
