import { Component, TemplateRef, ViewChild } from '@angular/core';
import { MatDialog, MatDialogRef } from '@angular/material/dialog';

import { ManagementService } from '../../service/management.service';

@Component({
    selector: 'app-phonytony-toolbar',
    templateUrl: './phonytony-toolbar.component.html',
    styleUrls: ['./phonytony-toolbar.component.scss'],
})
export class PhonytonyToolbarComponent {
    @ViewChild('powerdownDialog') public powerdownDialog!: TemplateRef<unknown>;

    private powerdownDialogRef?: MatDialogRef<unknown>;

    constructor(private dialogService: MatDialog, private managementService: ManagementService) {}

    public onTimeRemainingClick(): void {
        console.log('time remaining click');
    }

    public onPowerdownClick(): void {
        if (this.powerdownDialogRef) {
            return;
        }

        this.powerdownDialogRef = this.dialogService.open(this.powerdownDialog);
    }

    public onPoweroff(): void {
        this.managementService.powerdown();

        if (this.powerdownDialogRef) {
            this.powerdownDialogRef.close();
            this.powerdownDialogRef = undefined;
        }
    }

    public onStopWifi(): void {
        this.managementService.stopWifi();

        if (this.powerdownDialogRef) {
            this.powerdownDialogRef.close();
            this.powerdownDialogRef = undefined;
        }
    }
}
