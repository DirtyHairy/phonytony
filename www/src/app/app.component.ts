import { Component } from '@angular/core';
import { ManagementService } from './service/management.service';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.scss'],
})
export class AppComponent {
    public messages$ = this.managementService.messages();

    constructor(public managementService: ManagementService) {}
}
