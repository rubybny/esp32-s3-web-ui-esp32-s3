#pragma once

#include <Arduino.h>

// Single-character commands over USB serial, replacing the old phone/web UI
// for calibration. No-ops (with a log message) when ENABLE_ADC_LEARNING is
// 0, except 'h'/'p' which always work.
void setupSerialCommands();
void updateSerialCommands();
