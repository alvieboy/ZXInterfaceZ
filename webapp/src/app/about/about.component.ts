import { Component, OnInit } from '@angular/core';
import { HttpEventType, HttpResponse } from '@angular/common/http';
import { Observable } from 'rxjs';

import { UploadService } from '../services/upload.service';
import { FwUploadStatus, Level } from '../models/FwUploadStatus';

@Component({
  selector: 'app-about',
  templateUrl: './about.component.html',
  styleUrls: ['./about.component.css']
})
export class AboutComponent implements OnInit {

  fwFile = '';
  status = {
    action: '',
    level: Level.Info,
    percent: -1,
    phase: '',
  };

  constructor(private uploadService: UploadService) { }

  ngOnInit(): void {
  }

  firmwareInputChange(fileInputEvent: any) {

    this.fwFile = fileInputEvent.target.files[0];
    this.uploadService.uploadFirmware(this.fwFile).subscribe(
      status => {
        console.log(status);
        this.status = status;
      }
    );
    console.log(this.fwFile)
  }

  uploading(): Boolean {
    return this.status != null && this.status.percent >= 0;
  }
}
