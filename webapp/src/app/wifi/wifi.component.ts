import { Component, OnInit } from '@angular/core';
import { HttpEventType, HttpResponse } from '@angular/common/http';
import { Observable } from 'rxjs';

import { WifiService } from '../services/wifi.service';

@Component({
  selector: 'app-wifi',
  templateUrl: './wifi.component.html',
  styleUrls: ['./wifi.component.css']
})
export class WifiComponent implements OnInit {

  progress = 0;
  message = '';

  constructor(private wifiService: WifiService) { }

  ngOnInit(): void {
  }

  scanWifi() {
    this.progress = 0;

    this.wifiService.startScan().subscribe(
      event => {
        if (event.type === HttpEventType.UploadProgress) {
          this.progress = Math.round(100 * event.loaded / event.total);
        } else if (event instanceof HttpResponse) {
          this.message = event.body.message; // TODO Show snack-bar
        }
      },
      err => {
        this.progress = 0;
        this.message = 'Failed to scan Wifi!';
      });
  }
}
