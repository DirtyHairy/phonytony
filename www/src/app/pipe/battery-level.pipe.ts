import { Pipe, PipeTransform } from '@angular/core';

import { BatteryLevel } from '../model/message';

@Pipe({ name: 'batteryLevel' })
export class BatteryLevelPipe implements PipeTransform {
    transform(value: BatteryLevel): string {
        switch (value) {
            case BatteryLevel.critical:
                return 'kritisch';

            case BatteryLevel.full:
                return 'voll';

            case BatteryLevel.low:
                return 'niedrig';

            case BatteryLevel.poweroff:
                return 'Box schaltet jetzt ab';

            default:
                return 'invalid';
        }
    }
}
