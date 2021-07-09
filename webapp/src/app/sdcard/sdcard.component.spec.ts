import { ComponentFixture, TestBed, waitForAsync } from '@angular/core/testing';

import { SdcardComponent } from './sdcard.component';

describe('SdcardComponent', () => {
  let component: SdcardComponent;
  let fixture: ComponentFixture<SdcardComponent>;

  beforeEach(waitForAsync(() => {
    TestBed.configureTestingModule({
      declarations: [ SdcardComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(SdcardComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
