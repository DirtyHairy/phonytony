import { Pipe, PipeTransform } from '@angular/core';

@Pipe({ name: 'voltage' })
export class VoltagePipe implements PipeTransform {
    transform(value: number): string {
        return (value / 1000).toFixed(2) + ' V';
    }
}
