import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';

import { HomeComponent } from './home/home.component';
import { SdcardComponent } from './sdcard/sdcard.component';
import { StatusComponent } from './status/status.component';
import { WifiComponent } from './wifi/wifi.component';
import { DevicesComponent } from './devices/devices.component';
import { NotfoundComponent } from './notfound/notfound.component';

const routes: Routes = [
  { path: 'home', component: HomeComponent },
  // { path: 'sdcard', component: SdcardComponent },
  // { path: 'sdcard', component: SdcardComponent, children: [ { path: '**', component: SdcardComponent } ] },
  { path: 'wifi', component: WifiComponent },
  { path: 'devices', component: DevicesComponent },
  { path: 'status', component: StatusComponent },
  { path: '', redirectTo: '/home', pathMatch: 'full' },
  { path: '**', component: NotfoundComponent }
];

@NgModule({
  imports: [RouterModule.forRoot(routes, { relativeLinkResolution: 'legacy' })],
  exports: [RouterModule]
})
export class AppRoutingModule { }
