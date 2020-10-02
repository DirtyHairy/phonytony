import { Component } from '@angular/core';
import { ServerEventsService } from './service/server-events.service';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.scss'],
})
export class AppComponent {
    constructor(public serverEventsService: ServerEventsService) {}
}
