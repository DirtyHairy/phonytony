export enum PowerState {
    discharging = 0,
    charging = 1,
    standby = 2,
    invalid = -1,
}

export enum BatteryLevel {
    poweroff = 0,
    critical = 1,
    low = 2,
    full = 3,
    invalid = -1,
}

export interface Message {
    audio: {
        isPlaying: boolean;
        currentAlbum: string;
        currentTrack: number;
        volume: number;
    };

    power: {
        voltage: number;
        level: BatteryLevel;
        state: PowerState;
    };

    heap: number;
}
