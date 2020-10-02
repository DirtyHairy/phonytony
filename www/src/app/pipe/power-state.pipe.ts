import { Pipe, PipeTransform } from '@angular/core';

import { PowerState } from '../model/message';

@Pipe({ name: 'powerState' })
export class PowerStatePipe implements PipeTransform {
    transform(value: PowerState): string {
        switch (value) {
            case PowerState.charging:
                return 'wird geladen';

            case PowerState.discharging:
                return 'wird entladen';

            case PowerState.standby:
                return 'Netz';

            default:
                return 'invalid';
        }
    }
}
