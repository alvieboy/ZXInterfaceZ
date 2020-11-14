import { Component, OnInit } from '@angular/core';
import { Router, ActivatedRoute, ParamMap } from '@angular/router';
import { NestedTreeControl } from '@angular/cdk/tree';
import { MatTreeNestedDataSource } from '@angular/material/tree';
import { Observable, of as observableOf } from 'rxjs';

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

  hasChild = (_: number, node: Vnode) => !!node.children && node.children.subscribe(children => children.length > 0);

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private sdcardService: SdcardService,
  ) {
    this.dataSource.data = [];
  }

  ngOnInit(): void {
    var path = this.route.snapshot.firstChild == null ?  '' : this.route.snapshot.firstChild.url.join('/');
    this.currentPath = `/${path}`;
    console.log(this.currentPath);
    this.sdcardService.getDir('/').subscribe(d => {
      this.dataSource.data = [d];
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
