import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Observable, of, pipe } from 'rxjs';
import { map } from 'rxjs/operators';

import { VersionElement } from '../models/VersionElement';

@Injectable()
export class VersionService {
  versionUrl: string = '/req/version';
  constructor(private http: HttpClient) { }

  getVersion(): Observable<VersionElement[]> {

    return this.http.get<Version>(this.versionUrl).pipe(map(o => {
      var arr: VersionElement[] = [];

      for (var i in o) {
        if (o.hasOwnProperty(i) && i != "success") {
          arr.push({ key: i, value: o[i]});
        }
      }
      return arr;
    }));
  }
}

interface Version {
  success: boolean,
  espChip: string,
  espVersion: string,
  espRevision: number,
  espCores: number,
  espFlash: string,
  softwareVersion: string,
  fpgaVersion: string,
  gitVersion: string,
  buildDate: string,
}
