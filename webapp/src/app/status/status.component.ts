import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { HttpEventType, HttpResponse } from '@angular/common/http';
import { MatPaginator } from '@angular/material/paginator';
import { MatTableDataSource } from '@angular/material/table';
import { Observable } from 'rxjs';

import { VersionService } from '../services/version.service';
import { VersionElement } from '../models/VersionElement';
import { UploadService } from '../services/upload.service';

import { faCogs } from '@fortawesome/free-solid-svg-icons';

@Component({
  selector: 'app-status',
  templateUrl: './status.component.html',
  styleUrls: ['./status.component.css']
})
export class StatusComponent implements OnInit, AfterViewInit {

  faCogs = faCogs;

  displayedColumns: string[] = ['key', 'value'];
  stats = new MatTableDataSource<VersionElement>([]);
  fwFile: File;
  progress = 0;
  message = '';

  @ViewChild(MatPaginator) paginator: MatPaginator;

  ngAfterViewInit() {
    this.stats.paginator = this.paginator;
  }

  constructor(
    private versionService: VersionService,
    private uploadService: UploadService,
  ) { }

  ngOnInit(): void {
    this.versionService.getVersion().subscribe(version => {
      this.stats = new MatTableDataSource<VersionElement>(version);
    })
  }

  firmwareInputChange(fileInputEvent: any) {
    this.progress = 0;

    this.fwFile = fileInputEvent.target.files[0];
    this.uploadService.uploadFirmware(this.fwFile).subscribe(
      event => {
        if (event.type === HttpEventType.UploadProgress) {
          this.progress = Math.round(100 * event.loaded / event.total);
        } else if (event instanceof HttpResponse) {
          this.message = event.body.message; // TODO Show snack-bar
        }
      },
      err => {
        this.progress = 0;
        this.message = 'Could not upload the file!';
        this.fwFile = undefined;
      });
  }
}
