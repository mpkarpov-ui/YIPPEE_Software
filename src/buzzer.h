#pragma once
#include <Arduino.h>
#include "pins.h"
// Header only file of buzzer beeps

void BUZZ_GPS_LOCK_ACQUIRED() {
    tone(BUZZER, 3500, 50);
    delay(50);
    tone(BUZZER, 3650, 50);
    delay(50);
    tone(BUZZER, 3500, 50);
    delay(50);
    tone(BUZZER, 3650, 50);
    delay(50);
    tone(BUZZER, 3500, 50);
    delay(50);
    tone(BUZZER, 3650, 50);
    delay(50);
    tone(BUZZER, 3500, 50);
}

void BUZZ_GPS_LOCK_LOST() {
    tone(BUZZER, 3000, 100);
    delay(100);
    tone(BUZZER, 2800, 100);
    delay(100);
}

void BUZZ_GPS_SAT_IN_VIEW() {
    tone(BUZZER, 3200, 100);
}

void BUZZ_YIPPEE() {
    tone(BUZZER, 5200, 130);
    delay(120);
    tone(BUZZER, 4800, 100);
    delay(100);
}

void BUZZ_YIPPEE_LANDING() {
    tone(BUZZER, 4000, 500);
}

// Single high pitch buzz to indicate a successful launch detect.
// Important that it doesn't block
void BUZZ_YIPPEE_LAUNCH() {
    tone(BUZZER, 5200, 350);
}

void BUZZ_FAIL_GPS() {
    tone(BUZZER, 3000, 100);
    delay(200);
    tone(BUZZER, 3000, 100);
    delay(200);
    tone(BUZZER, 3000, 100);
    delay(200);
}

void BUZZ_FAIL_MEM() {
    tone(BUZZER, 3000, 100);
    delay(200);
    tone(BUZZER, 3000, 100);
    delay(200);
}

void BUZZ_FAIL_LORA() {
    tone(BUZZER, 3000, 100);
    delay(100);
}

void BUZZ_FAIL_TRANSMIT() {
    tone(BUZZER, 3000, 50);
}

void BUZZ_INIT() {
    tone(BUZZER, 2500, 60);
    delay(60);
}