import { BehaviorSubject, Observable } from 'rxjs';

import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Message } from '../model/message';
import { environment } from 'src/environments/environment';

const TIMEOUT = 5000;

@Injectable({
    providedIn: 'root',
})
export class ManagementService {
    private messages$ = new BehaviorSubject<Message | undefined>(undefined);
    private source = new EventSource(this.eventsUrl());

    private timeoutHandle: number | undefined;
    private disconnected = true;

    constructor(private httpClient: HttpClient) {
        // tslint:disable-next-line: no-any
        this.source.addEventListener('status' as any, this.onStatusMessage);

        // No need to actually *do* anything, just register a handler in order to trigger
        // change detection
        this.source.addEventListener('error', () => undefined);
    }

    public isConnected(): boolean {
        return this.source.readyState === this.source.OPEN && !this.disconnected;
    }

    public async powerdown(): Promise<void> {
        await this.httpClient.post(`${environment.boxUrl}/api/powerdown`, null).toPromise();
    }

    public async stopWifi(): Promise<void> {
        await this.httpClient.post(`${environment.boxUrl}/api/stop-wifi`, null).toPromise();
    }

    public messages(): Observable<Message | undefined> {
        return this.messages$;
    }

    private eventsUrl(): string {
        return `${environment.boxUrl}/api/events`;
    }

    private onStatusMessage = (e: MessageEvent): void => {
        if (this.timeoutHandle !== undefined) {
            window.clearTimeout(this.timeoutHandle);
        }

        this.timeoutHandle = setTimeout(this.onMessageTimeout, TIMEOUT);
        this.disconnected = false;

        this.messages$.next(JSON.parse(e.data));
    };

    private onMessageTimeout = (): void => {
        this.disconnected = true;
        this.timeoutHandle = undefined;
    };
}
