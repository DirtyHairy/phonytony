import { Component, Input, OnInit } from '@angular/core';

@Component({
    selector: 'app-status-card-line',
    templateUrl: './status-card-line.component.html',
    styleUrls: ['./status-card-line.component.scss'],
})
export class StatusCardLineComponent {
    @Input() public label!: string;

    constructor() {}
}
