import { Component, OnInit } from '@angular/core';

import { ManagementService } from 'src/app/service/management.service';

@Component({
    selector: 'app-status-card-memory',
    templateUrl: './status-card-memory.component.html',
    styleUrls: ['./status-card-memory.component.scss'],
})
export class StatusCardMemoryComponent {
    public messages$ = this.managementService.messages();

    constructor(private managementService: ManagementService) {}
}
