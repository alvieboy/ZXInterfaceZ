import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders, HttpParams } from '@angular/common/http';
import { Observable, pipe, zip, of } from 'rxjs';
import { map as mapTo } from 'rxjs/operators';

import { Vnode } from '../models/Vnode';

@Injectable()
export class SdcardService {
  listUrl: string = '/req/list';
  constructor(private http: HttpClient) { }

  getDir(path: string): Observable<Vnode> {

    const options = { params: { path: path }};
    const res = this.http.get<ApiDir>(this.listUrl, options);
    console.log(`GET ${this.listUrl}?path=${path}`);
    res.subscribe(res => {
      console.log(res);
    });
    return res.pipe(mapTo(dir => {
      return {
        name: dir.path,
        ftype: 'd',
        extension: '',
        size: dir.entries.length,
        children: zip(...this.getChildren(dir.entries, path)),
      };
    }));
  }

  getChildren(entries: ApiEntry[], path: string): Observable<Vnode>[] {
    return entries.map(entry=> {
      if (entry.type != 'dir') {
        return this.getFileChild(entry);
      } else {
        return this.getDirChild(path, entry);
      }
    });
  }

  getFileChild(child: ApiEntry): Observable<Vnode> {
    const nameParts = child.name.split('.');
    const extension = nameParts.length > 1 ? nameParts[nameParts.length -1] : '';
    const file: Vnode = {
      name: child.name,
      ftype: child.type[0],
      extension: extension,
      size: child.size,
    };
    return of(file);
  }

  getDirChild(path: string, child: ApiEntry): Observable<Vnode> {
    return this.getDir(`${path == '/' ? '' : path}/${child.name}`);
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
