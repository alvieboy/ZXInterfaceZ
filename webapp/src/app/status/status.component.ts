import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { MatPaginator } from '@angular/material/paginator';
import { MatTableDataSource } from '@angular/material/table';

import { VersionService } from '../services/version.service';
import { VersionElement } from '../models/VersionElement';

@Component({
  selector: 'app-status',
  templateUrl: './status.component.html',
  styleUrls: ['./status.component.css']
})
export class StatusComponent implements OnInit, AfterViewInit {

  displayedColumns: string[] = ['key', 'value'];
  stats = new MatTableDataSource<VersionElement>([]);

  @ViewChild(MatPaginator) paginator: MatPaginator;

  ngAfterViewInit() {
    this.stats.paginator = this.paginator;
  }

  constructor(private versionService: VersionService) { }

  ngOnInit(): void {
    this.versionService.getVersion().subscribe(version => {
      this.stats = new MatTableDataSource<VersionElement>(version);
    })
  }
}
