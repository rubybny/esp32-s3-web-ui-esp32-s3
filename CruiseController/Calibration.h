#pragma once

#include <Arduino.h>

#include "LeverTypes.h"

// Learned ADC calibration, backed by NVS (Preferences) when
// ENABLE_ADC_LEARNING is 1. When disabled, every getter below simply returns
// the fixed compile-time bands from Config.h and the setters are no-ops --
// callers do not need to branch on the feature flag themselves.

void loadAdcCalibration();
void saveAdcCalibration();

// Erases stored calibration and reverts to the Config.h defaults.
void resetAdcCalibration();

// Current effective band for a real button (MAIN/CANCEL/RES/SET), after
// applying any learned center shift on top of the configured band width.
void getAdcBand(LeverButton button, int* outMin, int* outMax);

// Current effective "released" threshold (see AdcBands::ADC_NEUTRAL_MIN).
int getAdcNeutralMin();

// Learns a new center for `button` from a single ADC reading (normally the
// current filtered value, captured via a serial command -- see
// SerialCommands.cpp). MAIN and CANCEL/RES/SET keep the same band *width*
// configured in Config.h; only the center moves. Returns false for NONE/
// UNKNOWN, which cannot be learned this way.
bool learnAdcCenter(LeverButton button, int adcValue);

// Learns the "released" threshold from a current open-circuit reading,
// leaving a safety margin below it so the learned threshold stays robust to
// small dips (see Calibration.cpp for the exact margin).
bool learnAdcNeutral(int adcValue);

void printAdcCalibration();
