#pragma once

#include <Arduino.h>

#include "LeverTypes.h"

void setupLeverState();

// Classifies one filtered ADC value into a candidate button. `confirmed` is
// the currently-confirmed state, used only to apply hysteresis to whichever
// band is presently active (see Config.h AdcBands::ADC_HYSTERESIS).
LeverButton detectLeverState(int filteredAdc, LeverButton confirmed);

// Runs the candidate -> confirm -> neutral-lockout state machine for the
// current filtered ADC reading. Must be called every loop() iteration with
// the current time; only actually changes state at most once per call.
void updateLeverState(uint32_t now);

LeverButton getConfirmedLeverState();
LeverButton getPreviousLeverState();
LeverButton getCandidateLeverState();  // for debug logging only -- not debounced

// True for exactly the loop iteration in which confirmed state changed.
bool leverStateJustChanged();

// How long (ms) the current confirmed state has been held.
uint32_t getLeverStateHoldMs(uint32_t now);

// True once MAIN has been continuously held for
// StateTiming::MAIN_LONG_PRESS_MS, latched until MAIN is released (does not
// re-fire every loop while held).
bool isMainLongPress();
