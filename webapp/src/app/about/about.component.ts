import { Component, OnInit } from '@angular/core';
import { HttpEventType, HttpResponse } from '@angular/common/http';
import { Observable } from 'rxjs';
import { WebSocketSubject } from 'rxjs/webSocket';
import { MatSnackBar } from '@angular/material/snack-bar';

import { UploadService } from '../services/upload.service';
import { FwUploadStatus, Level } from '../models/FwUploadStatus';

@Component({
  selector: 'app-about',
  templateUrl: './about.component.html',
  styleUrls: ['./about.component.css']
})
export class AboutComponent implements OnInit {

  private socket: WebSocketSubject<FwUploadStatus | string | ArrayBuffer>;
  fwFile: File = null;
  status = {
    action: '',
    level: Level.Info,
    percent: -1,
    phase: '',
  };
  uploading = false;

  constructor(
    private uploadService: UploadService,
    private snackBar: MatSnackBar,
  ) { }

  ngOnInit(): void {
  }

  firmwareInputChange(fileInputEvent: any) {

    this.fwFile = fileInputEvent.target.files[0];
    this.socket = this.uploadService.uploadFirmware(this.fwFile)
    this.socket.subscribe(
      (status) => {
        if (isStatusMessage(status)) {
          this.status = status;
          if (this,status.percent >= 100) {
            this.completed();
          }
        }
      },
      (err) => this.snackBar.open(err),
      () => this.completed()
    );
    this.uploading = true;
    console.log(this.fwFile);
  }

  completed() {
    this.snackBar.open('Firmware upload completed!')
  }
}

function isStatusMessage(message: FwUploadStatus | string | ArrayBuffer): message is FwUploadStatus {
  return (message as FwUploadStatus).level !== undefined;
}

