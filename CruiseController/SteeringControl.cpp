#include "SteeringControl.h"

#include <Arduino.h>

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
constexpr uint32_t BUTTON_DEBOUNCE_MS = 60;

bool autoUpSet = false;
bool autoDownRes = false;
bool autoCancel = false;
bool mainButtonWasDown = false;
bool mainPulseActive = false;
uint32_t mainPulseUntil = 0;
String candidateButton = "NONE";
String stableButton = "NONE";
uint32_t candidateSince = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

void setAutoMomentary(const String& outputName, bool& autoFlag, bool active);

String updateDebouncedButton(const String& rawButton, uint32_t now) {
  if (rawButton != candidateButton) {
    candidateButton = rawButton;
    candidateSince = now;
    return stableButton;
  }

  if (stableButton != candidateButton && elapsed(now, candidateSince + BUTTON_DEBOUNCE_MS)) {
    stableButton = candidateButton;
  }

  return stableButton;
}

void clearAutoOutputs() {
  setAutoMomentary("upSet", autoUpSet, false);
  setAutoMomentary("downRes", autoDownRes, false);
  setAutoMomentary("cancel", autoCancel, false);
}

void setAutoMomentary(const String& outputName, bool& autoFlag, bool active) {
  if (active) {
    if (!autoFlag || !getOutputState(outputName)) {
      setOutputState(outputName, true);
    }
    autoFlag = true;
    return;
  }

  if (autoFlag) {
    setOutputState(outputName, false);
    autoFlag = false;
  }
}
}

String getDebouncedSteeringButton() {
  return stableButton;
}

void updateSteeringOutputs() {
  const int adc = analogRead(Pins::ADC_STEER);
  const uint32_t now = millis();
  const String button = updateDebouncedButton(detectButton(adc), now);

  if (digitalRead(Pins::BRAKE_IN) == Pins::BRAKE_IN_ACTIVE_LEVEL) {
    clearAutoOutputs();
    mainPulseActive = false;
    mainButtonWasDown = false;
    resetOutputs();
    return;
  }

  setAutoMomentary("upSet", autoUpSet, button == "UpSet");
  setAutoMomentary("downRes", autoDownRes, button == "DownRes");
  setAutoMomentary("cancel", autoCancel, button == "Cancel");

  const bool mainButtonDown = button == "Main";
  if (mainButtonDown && !mainButtonWasDown && !mainPulseActive) {
    setOutputState("main", true);
    mainPulseActive = true;
    mainPulseUntil = now + getTimingMs("mainHoldMs", 1000);
  }
  mainButtonWasDown = mainButtonDown;

  if (mainPulseActive && elapsed(now, mainPulseUntil)) {
    setOutputState("main", false);
    mainPulseActive = false;
  }
}
