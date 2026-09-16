#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// All tunables live here as compile-time constants / #defines. There is no
// web UI and no phone-based tuning: every value below is set before flashing.
// Values marked "PLACEHOLDER" are not yet confirmed against the real vehicle
// and lever hardware -- change them here once measured, then reflash.
// ---------------------------------------------------------------------------

// ===== Feature toggles ======================================================

// Prints RAW/filtered ADC, state transitions, output changes and a periodic
// heartbeat over USB serial. Turn off once tuned to reduce serial spam.
#define DEBUG_MODE 1

// Enables learning lever ADC centers into NVS (Preferences) via serial
// commands (see SerialCommands.cpp) instead of using the fixed constants
// below. Learned values persist across reboots; if none are stored yet, the
// fixed constants are used as-is.
#define ENABLE_ADC_LEARNING 1

// GPIO9's final role is not decided yet (see Pins::PIN_9 below): it either
// drives the CANCEL relay/PhotoMOS as an output, or it reads a 12V-derived
// BRAKE signal as an input. Only one role can be active at a time.
//   0 = GPIO9 is the CANCEL output (current assumption, matches Rev.2 wiring
//       where CANCEL uses its own AQY210EHA driven straight from the MCU).
//   1 = GPIO9 is a BRAKE input. CANCEL then has no output pin -- driving it
//       would need a hardware change, so setCancelOutput() becomes a no-op
//       and logs a warning if this mode is selected.
#define USE_BRAKE_INPUT 0

// ===== GPIO assignment ======================================================

namespace Pins {
constexpr uint8_t ADC_CRUISE = 4;   // Lever resistor ladder, 10k pull-up to 3.3V.
constexpr uint8_t OUT_MAIN = 5;     // -> analog switch / PhotoMOS driving vehicle MAIN line.
constexpr uint8_t OUT_RES = 6;      // -> analog switch / PhotoMOS driving vehicle RES+ line.
constexpr uint8_t OUT_SET = 7;      // -> analog switch / PhotoMOS driving vehicle SET- line.
constexpr uint8_t PIN_9 = 9;        // CANCEL output XOR brake input -- see USE_BRAKE_INPUT above.
}  // namespace Pins

// ===== Output polarity ======================================================

// Whether driving a vehicle line "on" means writing GPIO HIGH or LOW.
// The analog-switch/PhotoMOS candidates (74LVC2G66 / SN74HC4066 / AQY210EHA)
// aren't finalized, so this is a single switch rather than being hardcoded
// as digitalWrite(pin, HIGH) throughout the code.
enum class ActiveLevel : uint8_t { ACTIVE_LOW, ACTIVE_HIGH };
constexpr ActiveLevel OUTPUT_ACTIVE_LEVEL = ActiveLevel::ACTIVE_HIGH;

// Polarity of the BRAKE input signal when USE_BRAKE_INPUT is 1. The 12V ACC
// signal will go through some level-shifting/opto stage before reaching
// GPIO9; whether that stage inverts is still unknown.
constexpr ActiveLevel BRAKE_ACTIVE_LEVEL = ActiveLevel::ACTIVE_HIGH;

// ===== ADC judgement =========================================================
//
// The lever is a 2-wire resistive ladder read on a single ADC pin. Each
// button corresponds to a resistance (hence ADC) range, not one exact value:
// component tolerance, temperature and the 10k pull-up mean the true value
// drifts around the measured centers below. Everything here is a MIN/MAX
// *band*, not a single target, precisely so a bit of drift or noise does not
// push a reading out of range.
//
// Measured centers, confirmed against the real lever on 2026-09-16 (open
// build, not the placeholder guesses from the original spec text):
//   MAIN   0     (switch shorted to GND)
//   CANCEL 96
//   RES+   153
//   SET-   341
//   open (nothing pressed) reads 4095 (pulled up to 3.3V) -- this hardware's
//   lever really is open at rest, unlike the "~8.25kohm at rest" figure
//   quoted in the original spec text; trust this measurement over that one.
//
// CANCEL and RES+ sit only 57 counts apart, so there isn't much room to
// widen those two bands without them colliding -- if fluctuation while held
// ever bridges that gap, that needs fixing with better analog filtering
// (decoupling cap on the ADC line, more median samples) rather than by
// widening these further. MAIN and SET- have much more headroom on their
// open sides and are sized generously to absorb in-hand noise.
//
// Bands are intentionally narrower than the full gap between neighboring
// centers, leaving unclassified space between them. A reading that falls in
// that gap is UNKNOWN, not "whichever button is closest": guessing there is
// exactly the bug this rewrite removes (see LeverState.cpp for why).
namespace AdcBands {
constexpr int ADC_MAIN_MIN = 0;
constexpr int ADC_MAIN_MAX = 50;

constexpr int ADC_CANCEL_MIN = 70;
constexpr int ADC_CANCEL_MAX = 125;

constexpr int ADC_RES_MIN = 135;
constexpr int ADC_RES_MAX = 225;

constexpr int ADC_SET_MIN = 260;
constexpr int ADC_SET_MAX = 700;

// Above this, the lever is considered fully released. Confirmed open
// reading is 4095, so this keeps a large margin while still sitting well
// above ADC_SET_MAX.
constexpr int ADC_NEUTRAL_MIN = 3800;

// Once a band is the *currently confirmed* one, its effective MIN/MAX is
// widened by this much before a reading is considered to have left it. This
// is the "hysteresis" the spec asks for: without it, a reading sitting
// exactly on a band edge could rapidly flip the candidate back and forth.
// It is NOT applied to bands that are not currently confirmed. Kept modest
// (rather than matching MAIN/SET-'s wider margins) because CANCEL and RES+'s
// bands are already close together -- a larger value here would let
// whichever of those two is currently confirmed encroach on its neighbor.
constexpr int ADC_HYSTERESIS = 10;
}  // namespace AdcBands

// ===== ADC sampling / filtering =============================================

namespace AdcFilterConfig {
// One raw sample is taken and pushed into a ring buffer every this many ms.
// analogRead() itself is fast; pacing samples (rather than reading as fast
// as possible) spreads them over real time so the median below reflects a
// real time window, not N reads taken in the same instant.
constexpr uint32_t SAMPLE_INTERVAL_MS = 2;

// How many recent raw samples the median is taken over. Odd, within the
// 5-20 range the spec calls for. Median (not mean) is used because it
// rejects single-sample spikes completely instead of just diluting them.
constexpr uint8_t SAMPLE_COUNT = 9;
}  // namespace AdcFilterConfig

// ===== State confirmation (debounce) ========================================

namespace StateTiming {
// A candidate classification (see AdcBands) must be stable for this long
// before it is promoted to the confirmed lever state. This is what actually
// gates output changes: a single noisy filtered reading can no longer flip
// an output on its own, because updateLeverState() only ever acts on the
// confirmed state, never the raw candidate.
constexpr uint32_t CONFIRM_MS = 25;

// Once an output turns on, it is held for at least this long even if the
// lever is released again immediately, so a very quick tap still produces a
// pulse long enough for the vehicle ECU to register it.
constexpr uint32_t MIN_OUTPUT_ON_MS = 120;

// How long MAIN must be continuously held before it is reported as a long
// press (see LeverState::isMainLongPress()). Not wired to different output
// behavior yet -- exact use is still undecided -- but the detection and the
// constant exist so that behavior can be added later without restructuring.
constexpr uint32_t MAIN_LONG_PRESS_MS = 800;
}  // namespace StateTiming

// ===== Brake input (only meaningful if USE_BRAKE_INPUT is 1) ================

namespace BrakeConfig {
constexpr uint32_t DEBOUNCE_MS = 20;
}

// ===== Logging ===============================================================

namespace LogConfig {
constexpr unsigned long BAUD_RATE = 115200;
// Status heartbeat printed even when nothing changes, so a PC connected
// after boot can still see live ADC/output state without waiting for an
// operation.
constexpr uint32_t HEARTBEAT_MS = 2000;
}  // namespace LogConfig
