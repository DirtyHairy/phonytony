import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { EMPTY_MESSAGE, Message } from '../model/message';

import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

const TIMEOUT = 3000;

@Injectable({
    providedIn: 'root',
})
export class ManagementService {
    private messages$ = new BehaviorSubject<Message | undefined>(undefined);
    private source = new EventSource(this.eventsUrl());

    constructor(private httpClient: HttpClient) {
        this.source.addEventListener('status' as any, (e: MessageEvent) => this.messages$.next(JSON.parse(e.data)));
    }

    public isConnected(): boolean {
        return this.source.readyState === this.source.OPEN;
    }

    public powerdown(): void {
        this.httpClient.post(`${environment.boxUrl}/api/powerdown`, null);
    }

    public stopWifi(): void {
        this.httpClient.post(`${environment.boxUrl}/api/stop-wifi`, null);
    }

    public messages(): Observable<Message | undefined> {
        return this.messages$;
    }

    private eventsUrl(): string {
        return `${environment.boxUrl}/api/events`;
    }
}
