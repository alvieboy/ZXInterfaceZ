import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';
import { Observable, pipe } from 'rxjs';
import { map } from 'rxjs/operators';

import { Vnode } from '../models/Vnode';

@Injectable()
export class SdcardService {
  listUrl: string = '/req/list';
  constructor(private http: HttpClient) { }

  getDir(path: string): Observable<Vnode[]> {

    var options = { params: { path: path } };

    return this.http.get<ApiDir>(this.listUrl, options).pipe(map(dirs => {
      return dirs[0].entries.map( child => {
        var nameParts = child.name.split('.');
        var extension = '';
        if (nameParts.length > 0) {
          extension = extension[nameParts.length - 1];
        }
        return {
           name: child.name,
           extension: extension,
           ftype: child.type[0],
           size: child.size,
        }
      });
    }))
  }
}

interface ApiDir{
  path: string;
  entries: ApiEntry[];
}

interface ApiEntry {
  type: string;
  name: string;
  size: number;
}
