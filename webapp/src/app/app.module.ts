import { BrowserModule } from '@angular/platform-browser';
import { NgModule } from '@angular/core';
import { HttpClientModule } from '@angular/common/http';

import { AppRoutingModule } from './app-routing.module';
import { MaterialModule } from './material.module';
import { AppComponent } from './app.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { StatusComponent } from './status/status.component';
import { WifiComponent } from './wifi/wifi.component';
import { SdcardComponent } from './sdcard/sdcard.component';
import { DevicesComponent } from './devices/devices.component';
import { AboutComponent } from './about/about.component';
import { HomeComponent } from './home/home.component';
import { NotfoundComponent } from './notfound/notfound.component';

import { VersionService } from './services/version.service';
import { DeviceService } from './services/device.service';
import { UploadService } from './services/upload.service';
import { SdcardService } from './services/sdcard.service';
import { WifiService } from './services/wifi.service';
import { FontAwesomeModule } from '@fortawesome/angular-fontawesome';

@NgModule({
  declarations: [
    AppComponent,
    StatusComponent,
    WifiComponent,
    SdcardComponent,
    DevicesComponent,
    AboutComponent,
    HomeComponent,
    NotfoundComponent,
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    BrowserAnimationsModule,
    MaterialModule,
    HttpClientModule,
    FontAwesomeModule,
  ],
  providers: [
    VersionService,
    DeviceService,
    UploadService,
    SdcardService,
    WifiService,
  ],
  bootstrap: [AppComponent]
})
export class AppModule { }
