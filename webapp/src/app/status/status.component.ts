import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { MatPaginator } from '@angular/material/paginator';
import { MatTableDataSource } from '@angular/material/table';

@Component({
  selector: 'app-status',
  templateUrl: './status.component.html',
  styleUrls: ['./status.component.css']
})
export class StatusComponent implements OnInit, AfterViewInit {

  displayedColumns: string[] = ['key', 'value'];
  stats = new MatTableDataSource<StatusElement>(ELEMENT_DATA);

  @ViewChild(MatPaginator) paginator: MatPaginator;

  ngAfterViewInit() {
    this.stats.paginator = this.paginator;
  }

  constructor(
  ) { }

  ngOnInit(): void {
  }
}

export interface StatusElement {
    key: string;
    value: string;
}

const ELEMENT_DATA: StatusElement[] = [
  { key: "esp_chip", value: "ESP32" },
  { key: "esp_version", value: "0.1" },
  { key: "esp_revision", value: "4" },
  { key: "esp_cores", value: "2" },
  { key: "esp_flash", value: "Unknown" },

  { key: "software_version", value: "v1.0 2020/03/30" },
  { key: "fpga_version", value: "a5.10 r3" },
  { key: "git_version", value: "TAG_V01-286-g461b41e-dirty" },
  { key: "build_date", value: "Tue 11 Aug 09:38:31 UTC 2020" },
]; // TODO Todo call API
