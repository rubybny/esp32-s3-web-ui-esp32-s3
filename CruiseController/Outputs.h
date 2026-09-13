#pragma once

#include <Arduino.h>

void setupOutputPins();
bool isValidOutputName(const char* name);
bool getOutputState(const char* name);
bool setOutputState(const char* name, bool state);
void resetOutputs();
