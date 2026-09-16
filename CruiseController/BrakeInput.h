#pragma once

#include <Arduino.h>

// Only meaningful when USE_BRAKE_INPUT is 1 (see Config.h). When 0, GPIO9 is
// used as the CANCEL output instead and isBrakeActive() always returns
// false, so callers do not need to branch on the feature flag themselves.

void setupBrakeInput();
void updateBrakeInput(uint32_t now);
bool isBrakeActive();
