import { ComponentFixture, TestBed } from '@angular/core/testing';

import { StatusCardMemoryComponent } from './status-card-memory.component';

describe('StatusCardMemoryComponent', () => {
  let component: StatusCardMemoryComponent;
  let fixture: ComponentFixture<StatusCardMemoryComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ StatusCardMemoryComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StatusCardMemoryComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
