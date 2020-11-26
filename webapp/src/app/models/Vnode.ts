import { Observable } from 'rxjs';

export interface Vnode {
  name: string;
  fullPath: string;
  extension: string;
  ftype: string;
  size: number;
  children?: Observable<Vnode[]>;
}
