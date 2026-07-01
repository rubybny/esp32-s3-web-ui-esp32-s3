#include "SteeringControl.h"

#include <Arduino.h>

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
bool autoUpSet = false;
bool autoDownRes = false;
bool autoCancel = false;
bool mainButtonWasDown = false;
bool mainPulseActive = false;
uint32_t mainPulseUntil = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
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

void updateSteeringOutputs() {
  const int adc = analogRead(Pins::ADC_STEER);
  const String button = detectButton(adc);
  const uint32_t now = millis();

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
