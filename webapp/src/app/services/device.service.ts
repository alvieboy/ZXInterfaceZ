import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Observable, pipe } from 'rxjs';
import { map } from 'rxjs/operators';

import { Device } from '../models/Device';

@Injectable()
export class DeviceService {
  listUrl: string = '/req/devlist';
  constructor(private http: HttpClient) { }

  getDevices(): Observable<Device[]> {

    return this.http.get<ApiDevices>(this.listUrl).pipe(map(devs => {
      return devs.devices.map((dev: ApiDevice) => {
        return {
          bus: dev.bus,
          driver: dev.driver,
          id: dev.id,
          vendor: dev.vendor,
          product: dev.product,
          serial: dev.serial,
          usbClasses: dev.class.split(','),
          connected: dev.connected,
          use: dev.use,
        }
      });
    }));
  }
}

interface ApiDevices {
  devices: ApiDevice[];
}

interface ApiDevice {
  bus: string;
  driver: string;
  id: string;
  vendor: string;
  product: string;
  serial: string;
  class: string;
  connected: boolean;
  use: string;
}
