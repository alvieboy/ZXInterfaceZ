import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { MatPaginator } from '@angular/material/paginator';
import { MatTableDataSource } from '@angular/material/table';

@Component({
  selector: 'app-devices',
  templateUrl: './devices.component.html',
  styleUrls: ['./devices.component.css']
})
export class DevicesComponent implements OnInit, AfterViewInit {

  displayedColumns: string[] = ['id', 'vendor', 'product', 'connected'];
  devices = new MatTableDataSource<DeviceElement>(ELEMENT_DATA);

  @ViewChild(MatPaginator) paginator: MatPaginator;

  ngAfterViewInit() {
    this.devices.paginator = this.paginator;
  }

  constructor(
  ) { }

  ngOnInit(): void {
  }
}

export interface DeviceElement {
    id: string;
    vendor: string;
    product: string;
    connected: string; // TODO Data types
}

const ELEMENT_DATA: DeviceElement[] = [
  { id: "2345:a872", vendor: "Laaa", product: "Plcd", connected: "Yes" },
]; // TODO Todo call API
