import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { PhonytonyToolbarComponent } from './phonytony-toolbar.component';

describe('PhonytonyToolbarComponent', () => {
  let component: PhonytonyToolbarComponent;
  let fixture: ComponentFixture<PhonytonyToolbarComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ PhonytonyToolbarComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(PhonytonyToolbarComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
