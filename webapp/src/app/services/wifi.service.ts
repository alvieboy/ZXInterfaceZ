import { Injectable } from '@angular/core';
import { HttpClient, HttpEvent, HttpRequest } from '@angular/common/http';
import { Observable } from 'rxjs';

@Injectable()
export class WifiService {
  startScanUrl: string = '/req/startscan';

  constructor(private http: HttpClient) { }

  startScan(): Observable<HttpEvent<any>> {
    const formData: FormData = new FormData();

    const req = new HttpRequest('GET', this.startScanUrl, {
      reportProgress: true,
      responseType: 'json'
    });

    return this.http.request(req);
  }
}
