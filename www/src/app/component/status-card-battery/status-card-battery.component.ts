import { Component, OnInit } from '@angular/core';

import { ManagementService } from 'src/app/service/management.service';

@Component({
    selector: 'app-status-card-battery',
    templateUrl: './status-card-battery.component.html',
    styleUrls: ['./status-card-battery.component.scss'],
})
export class StatusCardBatteryComponent {
    public messages$ = this.managementService.messages();

    constructor(private managementService: ManagementService) {}
}
