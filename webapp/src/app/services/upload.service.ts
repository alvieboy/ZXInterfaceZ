import { Injectable } from '@angular/core';
import { HttpClient, HttpEvent, HttpRequest, HttpHeaders } from '@angular/common/http';
import { Observable } from 'rxjs';
import { of, merge } from 'rxjs';
import { mapTo, delay } from 'rxjs/operators';

import { FwUploadStatus, Level } from '../models/FwUploadStatus';

@Injectable()
export class UploadService {

  status1 = {
    action: 'Uploading',
    level: Level.Info,
    percent: 10,
    phase: 'Initializing',
  };
  status2 = {
    action: 'Flashing sector 2',
    level: Level.Warn,
    percent: 30,
    phase: 'Programming OTA executable',
  };
  status3 = {
    action: 'Flashing sector 7',
    level: Level.Error,
    percent: 60,
    phase: 'Programming OTA executable',
  };
  status4 = {
    action: 'Finalizing',
    level: Level.Info,
    percent: 98,
    phase: 'Validating',
  };

  uploadFirmware(fwfile: File): Observable<FwUploadStatus> {
    //emit one item
    const example = of(null);
    //delay output of each by an extra second
    const message = merge(
      example.pipe(mapTo(this.status1)),
      example.pipe(mapTo(this.status2), delay(1000)),
      example.pipe(mapTo(this.status3), delay(2000)),
      example.pipe(mapTo(this.status4), delay(3000))
    );
    return message;
  }
}
