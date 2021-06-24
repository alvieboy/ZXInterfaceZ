import { Injectable } from '@angular/core';

import { Observable, from, timer } from 'rxjs';
import { takeWhile } from 'rxjs/operators';
import { WebSocketSubject } from 'rxjs/webSocket';

import { FwUploadStatus, Level } from '../models/FwUploadStatus';

@Injectable()
export class UploadService {

  private socket: WebSocketSubject<FwUploadStatus | string | ArrayBuffer>;
  private uploading = false;

  uploadFirmware(fwfile: File): WebSocketSubject<FwUploadStatus | string | ArrayBuffer> { // TODO remove string and ArrayBuffer
    this.uploading = true;
    var protocol = window.location.protocol.replace('http', 'ws');
    var url = `${protocol}//${window.location.host}/upload/fwupgrade`;
    console.log(`WS connecting: ${url}`);

    this.socket = new WebSocketSubject({
      url: url,
      serializer: serializer,
      binaryType: 'arraybuffer'
    });

    console.log("WS connected");

    this.socket.subscribe(
      (message) => {
        console.log(message);
        if (isStatusMessage(message)) {
          this.uploading = message.percent < 100;
        }

      },
      (err) => console.error(err),
      () => console.warn('Completed!')
    );

    var CHUNK_SIZE = 512;
    // Send file
    this.socket.next('START');
    var f = from(fwfile.arrayBuffer());
    f.subscribe(buf => {
      var i;
      for (i = 0; i < Math.ceil(buf.byteLength / CHUNK_SIZE); i++) {
        this.socket.next(buf.slice(i*CHUNK_SIZE, (i+1)*CHUNK_SIZE));
      }
    });

    timer(1, 1000).pipe(
      takeWhile(_ => this.uploading)
    ).subscribe(_ =>
      this.socket.next('STATUS')
    );

    return this.socket;
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

function isStatusMessage(message: FwUploadStatus | string | ArrayBuffer): message is FwUploadStatus {
  return (message as FwUploadStatus).level !== undefined;
}
