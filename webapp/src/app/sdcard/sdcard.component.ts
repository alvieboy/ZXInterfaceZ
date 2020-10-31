import { Component, OnInit } from '@angular/core';
import { Router, ActivatedRoute, ParamMap } from '@angular/router';
import { NestedTreeControl } from '@angular/cdk/tree';
import { MatTreeNestedDataSource } from '@angular/material/tree';
import { Observable } from 'rxjs';

import { SdcardService } from '../services/sdcard.service';
import { Vnode } from '../models/Vnode';

@Component({
  selector: 'app-sdcard',
  templateUrl: './sdcard.component.html',
  styleUrls: ['./sdcard.component.css']
})
export class SdcardComponent implements OnInit {

  currentPath = '/';
  treeControl = new NestedTreeControl<Vnode>(node => node.children);
  dataSource = new MatTreeNestedDataSource<Vnode>();

  hasChild = (_: number, node: Vnode) => node.ftype == 'd'; // !!node.children && node.children.length > 0;

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private sdcardService: SdcardService,
  ) {
    this.dataSource.data = TREE_DATA;
  }

  ngOnInit(): void {
    var path = this.route.snapshot.firstChild == null ?  '' : this.route.snapshot.firstChild.url.join('/');
    this.currentPath = `/${path}`;
    console.log(this.currentPath);
    this.sdcardService.getDir('/').subscribe(d => {
      console.log(d);
      this.dataSource.data = d;
    });
  }

  fileIcon(extension: string): string {
    var icon = FILE_ICONS[extension];
    if (!icon) {
      icon = 'description';
    }

    return icon;
  }
}

const FILE_ICONS = {
  'tzx': 'save',
  'tap': 'save',
  'sna': 'memory',
}

const TREE_DATA: Vnode[] = [
  { name: 'README.md', extension: 'md', ftype: 'f', size: 33 },
  { name: 'a-b', extension: '', ftype: 'd', size: 5,  children: [] },
  { name: 'c-d', extension: '', ftype: 'd', size: 5, children: [
    { name: 'Cobble 2.tzx', extension: 'tzx', ftype: 'f', size: 5 },
    { name: 'Chessfire.tap', extension: 'tap', ftype: 'f', size: 7 },
  ] },
  { name: 'e-f', extension: '', ftype: 'd', size: 5, children: [
    { name: 'Enduro Racer.tzx', extension: 'tzx', ftype: 'f', size: 3 },
    { name: 'Fighter Bomber.sna', extension: 'sna', ftype: 'f', size: 7 },
  ] },

]
