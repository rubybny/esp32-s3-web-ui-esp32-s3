#include "ActionHandler.h"

#include "BrakeInput.h"
#include "Config.h"
#include "LeverState.h"
#include "Logger.h"
#include "Outputs.h"

namespace {
bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

// Which button, if any, currently has its output physically driven. Tracked
// separately from LeverState's confirmedState because of the minimum-on-
// time floor below: the output can still be "active" for a brief moment
// after the lever itself has already been confirmed back to NONE.
LeverButton activeOutput = LeverButton::NONE;
uint32_t outputMinHoldUntil = 0;

// Latches once brake is detected, and is only released once BOTH the brake
// input has cleared AND the lever has independently been confirmed back to
// NONE. This is deliberately not just "brake cleared": the spec calls for
// the same neutral-return rule brake triggers as any other operation, so a
// foot still resting near the lever right as brakes release cannot
// immediately re-trigger an operation.
bool brakeLockout = false;

// A confirmed MAIN press fires a single fixed-width output pulse
// (StateTiming::MAIN_PULSE_MS), independent of how long the lever itself
// stays physically pressed: releasing early does not cut the pulse short,
// and continuing to hold does not extend or repeat it -- exactly one pulse
// per confirmed press.
bool mainPulseActive = false;
uint32_t mainPulseUntil = 0;

void driveOutput(LeverButton button, uint32_t now) {
  if (button == activeOutput) return;

  if (activeOutput != LeverButton::NONE && !elapsed(now, outputMinHoldUntil)) {
    // Still honoring the minimum-on-time for whatever is currently driven;
    // do not cut it short even though the lever has already moved on.
    return;
  }

  allOutputsOff();
  activeOutput = LeverButton::NONE;

  if (isRealButton(button)) {
    setOutputForButton(button, true);
    activeOutput = button;
    outputMinHoldUntil = now + StateTiming::MIN_OUTPUT_ON_MS;
    logf("OUTPUT %s ON", leverButtonName(button));
  }
}
}  // namespace

void setupActionHandler() {
  activeOutput = LeverButton::NONE;
  outputMinHoldUntil = 0;
  brakeLockout = false;
  mainPulseActive = false;
  mainPulseUntil = 0;
}

void handleLeverAction(uint32_t now) {
  LeverButton leverState = getConfirmedLeverState();

#if USE_BRAKE_INPUT
  bool brakeActive = isBrakeActive();
  if (brakeActive && !brakeLockout) {
    logLine("BRAKE detected -> forcing all outputs OFF");
    brakeLockout = true;
  }

  if (brakeLockout) {
    if (activeOutput != LeverButton::NONE) {
      allOutputsOff();
      activeOutput = LeverButton::NONE;
      logLine("OUTPUT forced OFF by brake");
    }

    if (!brakeActive && leverState == LeverButton::NONE) {
      brakeLockout = false;
      logLine("BRAKE cleared and lever NEUTRAL -> operations re-enabled");
    } else {
      return;
    }
  }
#endif

  if (leverStateJustChanged() && leverState == LeverButton::MAIN) {
    mainPulseActive = true;
    mainPulseUntil = now + StateTiming::MAIN_PULSE_MS;
  }
  if (mainPulseActive && elapsed(now, mainPulseUntil)) {
    mainPulseActive = false;
  }

  // While the MAIN pulse is running, it alone decides the output -- even if
  // the lever has already been released (confirmed back to NONE) or is still
  // held past the pulse window. Once the pulse ends, MAIN drives nothing
  // again until the lever returns to NONE and a fresh press re-triggers it
  // (LeverState's neutral lockout already guarantees leverState cannot jump
  // straight from MAIN to another real button without passing through NONE).
  LeverButton outputTarget;
  if (mainPulseActive) {
    outputTarget = LeverButton::MAIN;
  } else if (leverState == LeverButton::MAIN) {
    outputTarget = LeverButton::NONE;
  } else {
    outputTarget = leverState;
  }

  driveOutput(outputTarget, now);
}
