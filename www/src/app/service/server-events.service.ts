import { BehaviorSubject, Observable } from 'rxjs';
import { EMPTY_MESSAGE, Message } from '../model/message';

import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

@Injectable({
    providedIn: 'root',
})
export class ServerEventsService {
    constructor() {
        // tslint:disable-next-line: no-any
        this.source.addEventListener('status' as any, (e: MessageEvent) => this.messages.next(JSON.parse(e.data)));
    }

    private messages = new BehaviorSubject<Message>(EMPTY_MESSAGE);
    private source = new EventSource(this.url());

    getMessages(): Observable<Message> {
        return this.messages;
    }

    private url(): string {
        // tslint:disable-next-line: no-any
        return environment.boxUrl + '/events';
    }
}
