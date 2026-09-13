#pragma once

#include <Arduino.h>

void setupCruiseInput();
void updateCruiseInput();
int getCruiseAdc();
const char* getCruiseButton();
