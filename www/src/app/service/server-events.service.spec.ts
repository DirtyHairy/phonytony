import { ServerEventsService } from './server-events.service';
import { TestBed } from '@angular/core/testing';

describe('ServerEventsService', () => {
    let service: ServerEventsService;

    beforeEach(() => {
        TestBed.configureTestingModule({});
        service = TestBed.inject(ServerEventsService);
    });

    it('should be created', () => {
        expect(service).toBeTruthy();
    });
});
