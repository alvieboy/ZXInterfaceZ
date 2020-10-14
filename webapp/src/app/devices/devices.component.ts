import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { MatPaginator } from '@angular/material/paginator';
import { MatTableDataSource } from '@angular/material/table';

import { DeviceService } from '../services/device.service';
import { Device } from '../models/Device';

@Component({
  selector: 'app-devices',
  templateUrl: './devices.component.html',
  styleUrls: ['./devices.component.css']
})
export class DevicesComponent implements OnInit, AfterViewInit {

  displayedColumns: string[] = ['id', 'vendor', 'product', 'connected'];
  devices = new MatTableDataSource<Device>([]);

  @ViewChild(MatPaginator) paginator: MatPaginator;

  ngAfterViewInit() {
    this.devices.paginator = this.paginator;
  }

  constructor(private deviceService: DeviceService) { }

  ngOnInit(): void {
    this.deviceService.getDevices().subscribe(devices => {
      this.devices = new MatTableDataSource<Device>(devices);
    })
  }
}
