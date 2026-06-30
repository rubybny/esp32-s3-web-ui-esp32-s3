#pragma once

#include <Arduino.h>

void setupOutputPins();
bool isValidOutputName(const String& name);
bool getOutputState(const String& name);
bool setOutputState(const String& name, bool state);
void resetOutputs();
