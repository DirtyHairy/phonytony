import { AppComponent } from './app.component';
import { BatteryLevelPipe } from './pipe/battery-level.pipe';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { BrowserModule } from '@angular/platform-browser';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatIconModule } from '@angular/material/icon';
import { MatToolbarModule } from '@angular/material/toolbar';
import { NgModule } from '@angular/core';
import { PowerStatePipe } from './pipe/power-state.pipe';
import { VoltagePipe } from './pipe/voltage.pipe';

@NgModule({
    declarations: [AppComponent, BatteryLevelPipe, PowerStatePipe, VoltagePipe],
    imports: [BrowserModule, BrowserAnimationsModule, MatToolbarModule, MatButtonModule, MatIconModule, MatCardModule],
    providers: [],
    bootstrap: [AppComponent],
})
export class AppModule {}
