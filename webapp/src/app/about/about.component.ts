import { Component, OnInit } from '@angular/core';
import { HttpEventType, HttpResponse } from '@angular/common/http';
import { Observable } from 'rxjs';

import { UploadService } from '../services/upload.service';

@Component({
  selector: 'app-about',
  templateUrl: './about.component.html',
  styleUrls: ['./about.component.css']
})
export class AboutComponent implements OnInit {

  fwFile: File;
  progress = 0;
  message = '';

  constructor(private uploadService: UploadService) { }

  ngOnInit(): void {
  }


  firmwareInputChange(fileInputEvent: any) {
    this.progress = 0;

    this.fwFile = fileInputEvent.target.files[0];
    this.uploadService.uploadFirmware(this.fwFile).subscribe(
      event => {
        if (event.type === HttpEventType.UploadProgress) {
          this.progress = Math.round(100 * event.loaded / event.total);
        } else if (event instanceof HttpResponse) {
          this.message = event.body.message; // TODO Show snack-bar
        }
      },
      err => {
        this.progress = 0;
        this.message = 'Could not upload the file!';
        this.fwFile = undefined;
      });
  }
}
