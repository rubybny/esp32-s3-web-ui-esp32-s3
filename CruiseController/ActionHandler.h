#pragma once

#include <Arduino.h>

void setupActionHandler();

// Combines the confirmed lever state, the brake override (if enabled) and
// the minimum-on-time floor into the actual output pin writes. Called every
// loop() iteration after updateLeverState()/updateBrakeInput().
void handleLeverAction(uint32_t now);
