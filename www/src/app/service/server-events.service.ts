import { BehaviorSubject, Observable, Subject } from 'rxjs';

import { Injectable } from '@angular/core';
import { Message } from '../model/message';

@Injectable({
    providedIn: 'root',
})
export class ServerEventsService {
    constructor() {
        // tslint:disable-next-line: no-any
        this.source.addEventListener('update' as any, (e: MessageEvent) => this.messages.next(JSON.parse(e.data)));
    }

    private messages = new BehaviorSubject<Message>({ 'idf-version': '?', heap: -1, time: '?' });
    private source = new EventSource(this.url());

    getMessages(): Observable<Message> {
        return this.messages;
    }

    private url(): string {
        // tslint:disable-next-line: no-any
        return (window as any).__served_from_box === 'yes' ? '/events' : 'http://adabox.local/events';
    }
}
