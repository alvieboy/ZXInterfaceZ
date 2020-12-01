import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders, HttpParams } from '@angular/common/http';
import { Observable, pipe, zip, of } from 'rxjs';
import { map as mapTo } from 'rxjs/operators';

import { Vnode } from '../models/Vnode';

@Injectable()
export class SdcardService {
  listUrl: string = '/req/list';
  constructor(private http: HttpClient) { }

  getRoot(): Vnode[] {
    return [{
      name: '/',
      fullPath: '/',
      ftype: 'd',
      extension: '',
      size: 0, // TODO allow unknown for dirs
    }];
  }

  getChildren(path: string): Observable<Vnode[]> {

    const options = { params: { path: path }};
    const res = this.http.get<ApiDir>(this.listUrl, options);
    console.log(`GET ${this.listUrl}?path=${path}`);
    res.subscribe(res => {
      console.log(res);
    });
    return res.pipe(mapTo(dir => {
      return dir.entries.map(child => {
        const nameParts = child.name.split('.');
        const extension = nameParts.length > 1 ? nameParts[nameParts.length -1] : '';
        const fullPath = path == '/' ? `/${child.name}` : `${path}/${child.name}`;

        return {
          name: child.name,
          fullPath: fullPath,
          ftype: child.type[0],
          extension: extension,
          size: child.size,
        }
      })
    }));
  }

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
        fullPath: 'TODO',
        ftype: 'd',
        extension: '',
        size: dir.entries.length,
        children: zip(...this.getChildren2(dir.entries, path)),
      };
    }));
  }

  getChildren2(entries: ApiEntry[], path: string): Observable<Vnode>[] {
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
      fullPath: 'TODO',
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
