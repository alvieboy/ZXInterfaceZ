import { Injectable } from '@angular/core';
import { HttpClient, HttpEvent, HttpRequest, HttpHeaders } from '@angular/common/http';
import { Observable } from 'rxjs';

@Injectable()
export class UploadService {
  uploadFwUrl: string = '/upload/fwupgrade';

  constructor(private http: HttpClient) { }

  uploadFirmware(file: File): Observable<HttpEvent<any>> {
    const headers : HttpHeaders = new HttpHeaders({
      'Content-Type': 'binary/octet-stream'});

    const req = new HttpRequest('POST', this.uploadFwUrl, file, {
      headers: headers,
      reportProgress: true,
      responseType: 'json'
    });

    return this.http.request(req);
  }
}
