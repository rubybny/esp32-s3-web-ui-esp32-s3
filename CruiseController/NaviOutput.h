#pragma once

#include <Arduino.h>

void setupNaviOutputPins();
void resetNaviOutput();
bool getNaviOutputState(const String& buttonName);
void setNaviOutput(const String& buttonName);
