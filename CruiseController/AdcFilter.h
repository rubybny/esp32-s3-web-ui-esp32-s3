#pragma once

#include <Arduino.h>

// Non-blocking ADC sampling + median filtering for the lever input.
// Call update() every loop() iteration; it paces itself against millis() so
// it never blocks and never uses delay().

void setupAdcFilter();

// Feeds one new raw sample into the filter if SAMPLE_INTERVAL_MS has
// elapsed since the last one. Cheap to call every loop iteration.
void updateAdcFilter(uint32_t now);

// Most recent single raw analogRead() value (for debug/logging only --
// nothing safety-relevant should read this).
int readAdc();

// Median of the last SAMPLE_COUNT raw samples. This is the value all button
// classification is based on.
int filterAdc();
