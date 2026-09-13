#pragma once

#include <Arduino.h>

// All tunables live here as compile-time constants. There is no runtime
// configuration (no web UI, no NVS/Preferences) -- edit values below and
// reflash to change behavior.

namespace Pins {
constexpr uint8_t ADC_CRUISE = 4;
constexpr uint8_t OUT_MAIN = 5;
constexpr uint8_t OUT_RES = 6;
constexpr uint8_t OUT_SET = 7;
constexpr uint8_t OUT_BRAKE = 9;
}  // namespace Pins

namespace CruiseConfig {
// Raw 12-bit ADC targets measured on the cruise switch resistor ladder.
// MUST stay sorted ascending: MAIN < CANCEL < RES+ < SET-.
constexpr int ADC_MAIN = 0;
constexpr int ADC_CANCEL = 96;
constexpr int ADC_RES = 153;
constexpr int ADC_SET = 341;

// Above this reading the switch is considered "not pressed" (open circuit
// reads ~4095). Anything between SET- and this value is treated as NONE
// rather than guessed at, since it isn't a wired button position.
constexpr int ADC_OPEN_MIN = 3800;

// Classification boundaries, placed at the midpoint between neighboring
// targets so ADC noise on either side of a target still classifies
// correctly.
constexpr int BOUND_MAIN_CANCEL = (ADC_MAIN + ADC_CANCEL) / 2;
constexpr int BOUND_CANCEL_RES = (ADC_CANCEL + ADC_RES) / 2;
constexpr int BOUND_RES_SET = (ADC_RES + ADC_SET) / 2;

// A raw ADC reading must classify to the same button for this long before
// it is accepted. This is what actually gates output changes -- a single
// noisy sample can no longer fire an output.
constexpr uint32_t DEBOUNCE_MS = 30;

// Minimum time an output stays driven once activated, even if the button
// is released immediately after. Per-output so RES+/SET- taps can be
// tuned separately from MAIN/CANCEL if needed.
constexpr uint32_t PULSE_MAIN_MS = 200;
constexpr uint32_t PULSE_RES_MS = 200;
constexpr uint32_t PULSE_SET_MS = 200;
constexpr uint32_t PULSE_CANCEL_MS = 200;
}  // namespace CruiseConfig

namespace LogConfig {
constexpr unsigned long BAUD_RATE = 115200;
// Status heartbeat printed even when nothing changes, so a PC connected
// after boot can still see live ADC/output state.
constexpr uint32_t HEARTBEAT_MS = 2000;
}  // namespace LogConfig
