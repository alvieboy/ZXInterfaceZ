import { Observable } from 'rxjs';

export interface Vnode {
  name: string;
  extension: string;
  ftype: string;
  size: number;
  children?: Vnode[];
}
