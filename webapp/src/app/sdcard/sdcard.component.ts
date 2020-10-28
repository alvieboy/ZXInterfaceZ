import { Component, OnInit } from '@angular/core';
import { Router, ActivatedRoute, ParamMap } from '@angular/router';
import { NestedTreeControl } from '@angular/cdk/tree';
import { MatTreeNestedDataSource } from '@angular/material/tree';

@Component({
  selector: 'app-sdcard',
  templateUrl: './sdcard.component.html',
  styleUrls: ['./sdcard.component.css']
})
export class SdcardComponent implements OnInit {

  treeControl = new NestedTreeControl<Vnode>(node => node.children);
  dataSource = new MatTreeNestedDataSource<Vnode>();

  hasChild = (_: number, node: Vnode) => !!node.children && node.children.length > 0;

  constructor(
    private route: ActivatedRoute,
    private router: Router
  ) {
    this.dataSource.data = TREE_DATA;
  }

  ngOnInit(): void {
    console.log(this.route.snapshot.firstChild.url.join('/'));
  }
}

interface Vnode {
  name: string;
  extension: string;
  ftype: string;
  size: number;
  children?: Vnode[];
}

const TREE_DATA: Vnode[] = [
  { name: 'a-b', extension: '', ftype: 'd', size: 5 },
  { name: 'c-d', extension: '', ftype: 'd', size: 5, children: [
    { name: 'Cobble 2.tzx', extension: 'tzx', ftype: 'f', size: 5 },
    { name: 'Chessfire.tap', extension: 'tap', ftype: 'f', size: 7 },
  ] },
  { name: 'e-f', extension: '', ftype: 'd', size: 5, children: [
    { name: 'Enduro Racer.tzx', extension: 'tzx', ftype: 'f', size: 3 },
    { name: 'Fighter Bomber.tap', extension: 'tap', ftype: 'f', size: 7 },
  ] },

]
