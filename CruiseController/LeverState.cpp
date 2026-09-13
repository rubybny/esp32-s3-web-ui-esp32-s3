#include "LeverState.h"

#include "AdcFilter.h"
#include "Calibration.h"
#include "Config.h"
#include "Logger.h"

namespace {
bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

// Raw candidate: whatever the current filtered ADC value classifies to,
// re-evaluated every call. Not debounced by itself.
LeverButton candidateState = LeverButton::NONE;
uint32_t candidateSince = 0;

// The state everything else in the firmware (outputs, logging) actually
// acts on. Only ever changes inside updateLeverState(), and only after the
// candidate has held steady for StateTiming::CONFIRM_MS.
LeverButton confirmedState = LeverButton::NONE;
LeverButton previousState = LeverButton::NONE;
uint32_t confirmedSince = 0;
bool stateChangedThisTick = false;

// The core neutral-lockout flag. Set the instant confirmedState becomes
// anything other than NONE; cleared only when confirmedState becomes NONE
// again. While set, updateLeverState() refuses to confirm any candidate
// except NONE -- see the long comment in updateLeverState() for why this is
// the fix for the MAIN-release misread bug.
bool lockedOut = false;

bool mainLongPressLatched = false;
}  // namespace

void setupLeverState() {
  candidateState = LeverButton::NONE;
  confirmedState = LeverButton::NONE;
  previousState = LeverButton::NONE;
  lockedOut = false;
  mainLongPressLatched = false;
}

LeverButton detectLeverState(int filteredAdc, LeverButton confirmed) {
  // Each real button's band is widened by ADC_HYSTERESIS, but only while it
  // is the currently-confirmed one. This means a reading sitting exactly on
  // an edge does not chatter the candidate back and forth once we've
  // actually settled on that button; it still uses the tight, un-widened
  // band before we've committed to it.
  auto inBand = [&](int min, int max, LeverButton b) {
    int margin = (confirmed == b) ? AdcBands::ADC_HYSTERESIS : 0;
    return filteredAdc >= (min - margin) && filteredAdc <= (max + margin);
  };

  int mMin, mMax, cMin, cMax, rMin, rMax, sMin, sMax;
  getAdcBand(LeverButton::MAIN, &mMin, &mMax);
  getAdcBand(LeverButton::CANCEL, &cMin, &cMax);
  getAdcBand(LeverButton::RES, &rMin, &rMax);
  getAdcBand(LeverButton::SET, &sMin, &sMax);

  if (inBand(mMin, mMax, LeverButton::MAIN)) return LeverButton::MAIN;
  if (inBand(cMin, cMax, LeverButton::CANCEL)) return LeverButton::CANCEL;
  if (inBand(rMin, rMax, LeverButton::RES)) return LeverButton::RES;
  if (inBand(sMin, sMax, LeverButton::SET)) return LeverButton::SET;

  int neutralMin = getAdcNeutralMin();
  int neutralMargin = (confirmed == LeverButton::NONE) ? AdcBands::ADC_HYSTERESIS : 0;
  if (filteredAdc >= (neutralMin - neutralMargin)) return LeverButton::NONE;

  // Falls in a gap between bands, or between SET- and NEUTRAL_MIN. This is
  // exactly where a release sweep passes through on its way back to open,
  // and also where a mis-seated connector or a genuinely ambiguous reading
  // lands. Deliberately not guessed at as "closest button".
  return LeverButton::UNKNOWN;
}

void updateLeverState(uint32_t now) {
  stateChangedThisTick = false;

  LeverButton raw = detectLeverState(filterAdc(), confirmedState);
  if (raw != candidateState) {
    candidateState = raw;
    candidateSince = now;
  }

  if (!elapsed(now, candidateSince + StateTiming::CONFIRM_MS)) return;
  if (candidateState == confirmedState) return;

  // ---------------------------------------------------------------------
  // Neutral lockout. This is the single rule that fixes the two release-
  // time misreads the old firmware had:
  //   - MAIN held then released could transiently read as CANCEL/SET while
  //     the ADC value swept back up through their numeric bands.
  //   - The very same mechanism made a second MAIN press unreliable when
  //     leftover state from the first press's release hadn't fully reset.
  //
  // The fix: once confirmedState leaves NONE, do not let it become any
  // *other* value except NONE itself, no matter what the candidate says.
  // A sweep through CANCEL/RES/SET's bands during release is therefore
  // invisible to the rest of the firmware -- confirmedState simply stays on
  // the original button (or UNKNOWN, which drives no output either) until
  // the lever is unambiguously back at NONE. Only then can a brand new
  // operation -- even a second press of the same button -- be accepted.
  // ---------------------------------------------------------------------
  if (lockedOut && candidateState != LeverButton::NONE) {
    return;
  }

  previousState = confirmedState;
  confirmedState = candidateState;
  confirmedSince = now;
  stateChangedThisTick = true;
  lockedOut = (confirmedState != LeverButton::NONE);

  if (confirmedState != LeverButton::MAIN) mainLongPressLatched = false;

  logf("STATE %s -> %s (adc=%d filtered)", leverButtonName(previousState), leverButtonName(confirmedState),
       filterAdc());
}

LeverButton getConfirmedLeverState() { return confirmedState; }
LeverButton getPreviousLeverState() { return previousState; }
LeverButton getCandidateLeverState() { return candidateState; }
bool leverStateJustChanged() { return stateChangedThisTick; }

uint32_t getLeverStateHoldMs(uint32_t now) { return now - confirmedSince; }

bool isMainLongPress() {
  if (confirmedState != LeverButton::MAIN) return false;
  if (!mainLongPressLatched && getLeverStateHoldMs(millis()) >= StateTiming::MAIN_LONG_PRESS_MS) {
    mainLongPressLatched = true;
    logLine("MAIN long press threshold reached");
  }
  return mainLongPressLatched;
}
