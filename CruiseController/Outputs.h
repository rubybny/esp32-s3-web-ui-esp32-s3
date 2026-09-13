#pragma once

#include <Arduino.h>

#include "LeverTypes.h"

// Must be called as early as possible in setup(), before anything else runs,
// so the vehicle-facing pins never float or glitch high during boot.
void setupOutputPins();

void setMainOutput(bool on);
void setResOutput(bool on);
void setSetOutput(bool on);
void setCancelOutput(bool on);

// Drives every output off. Safe to call unconditionally; used for the
// UNKNOWN/NONE lever states, brake overrides, and startup.
void allOutputsOff();

// Convenience used by ActionHandler: turns exactly one line on (clearing the
// others first) based on a confirmed real button.
void setOutputForButton(LeverButton button, bool on);

bool getOutputState(LeverButton button);
