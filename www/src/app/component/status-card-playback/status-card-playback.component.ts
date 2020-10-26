import { Component } from '@angular/core';
import { ManagementService } from '../../service/management.service';

@Component({
    selector: 'app-status-card-playback',
    templateUrl: './status-card-playback.component.html',
    styleUrls: ['./status-card-playback.component.scss'],
})
export class StatusCardPlaybackComponent {
    public messages$ = this.managementService.messages();

    constructor(private managementService: ManagementService) {}
}
