import { AppComponent } from './app.component';
import { BatteryLevelPipe } from './pipe/battery-level.pipe';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { BrowserModule } from '@angular/platform-browser';
import { HttpClientModule } from '@angular/common/http';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatDialogModule } from '@angular/material/dialog';
import { MatIconModule } from '@angular/material/icon';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatToolbarModule } from '@angular/material/toolbar';
import { NgModule } from '@angular/core';
import { PhonytonyToolbarComponent } from './component/phonytony-toolbar/phonytony-toolbar.component';
import { PowerStatePipe } from './pipe/power-state.pipe';
import { StatusCardBatteryComponent } from './component/status-card-battery/status-card-battery.component';
import { StatusCardLineComponent } from './component/status-card-line/status-card-line.component';
import { StatusCardMemoryComponent } from './component/status-card-memory/status-card-memory.component';
import { StatusCardPlaybackComponent } from './component/status-card-playback/status-card-playback.component';
import { VoltagePipe } from './pipe/voltage.pipe';

@NgModule({
    declarations: [
        AppComponent,
        BatteryLevelPipe,
        PowerStatePipe,
        VoltagePipe,
        PhonytonyToolbarComponent,
        StatusCardLineComponent,
        StatusCardPlaybackComponent,
        StatusCardBatteryComponent,
        StatusCardMemoryComponent,
    ],
    imports: [
        BrowserModule,
        HttpClientModule,
        BrowserAnimationsModule,
        MatToolbarModule,
        MatButtonModule,
        MatIconModule,
        MatCardModule,
        MatDialogModule,
        MatProgressSpinnerModule,
    ],
    providers: [],
    bootstrap: [AppComponent],
})
export class AppModule {}
