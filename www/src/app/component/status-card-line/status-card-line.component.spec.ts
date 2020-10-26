import { ComponentFixture, TestBed } from '@angular/core/testing';

import { StatusCardLineComponent } from './status-card-line.component';

describe('StatusCardLineComponent', () => {
  let component: StatusCardLineComponent;
  let fixture: ComponentFixture<StatusCardLineComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ StatusCardLineComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StatusCardLineComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
