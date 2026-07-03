#pragma once

#include <Arduino.h>

void setupOutputPins();
bool isValidOutputName(const String& name);
bool getOutputState(const String& name);
bool setOutputState(const String& name, bool state);
bool pulseOutput(const String& name, uint32_t durationMs);
void updateOutputPulses();
void resetOutputs();
