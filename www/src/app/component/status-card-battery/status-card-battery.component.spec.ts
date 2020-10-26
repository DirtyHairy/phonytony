import { ComponentFixture, TestBed } from '@angular/core/testing';

import { StatusCardBatteryComponent } from './status-card-battery.component';

describe('StatusCardBatteryComponent', () => {
  let component: StatusCardBatteryComponent;
  let fixture: ComponentFixture<StatusCardBatteryComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ StatusCardBatteryComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StatusCardBatteryComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
