import { ComponentFixture, TestBed } from '@angular/core/testing';

import { StatusCardPlaybackComponent } from './status-card-playback.component';

describe('StatusCardPlaybackComponent', () => {
  let component: StatusCardPlaybackComponent;
  let fixture: ComponentFixture<StatusCardPlaybackComponent>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      declarations: [ StatusCardPlaybackComponent ]
    })
    .compileComponents();
  });

  beforeEach(() => {
    fixture = TestBed.createComponent(StatusCardPlaybackComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
