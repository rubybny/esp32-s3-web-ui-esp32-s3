#include "CruiseInput.h"

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
constexpr uint32_t DEBOUNCE_MS = 60;

int currentAdc = 4095;
String candidateButton = "NONE";
String stableButton = "NONE";
String lastPulseButton = "NONE";
uint32_t candidateSince = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

String detectCruiseButton(int adc) {
  if (adc <= 50) return "MAIN";
  if (adc >= 65 && adc <= 125) return "CANCEL";
  if (adc >= 130 && adc <= 220) return "RES+";
  if (adc >= 260 && adc <= 520) return "SET-";
  return "NONE";
}

String outputNameForButton(const String& button) {
  if (button == "MAIN") return "main";
  if (button == "RES+") return "res";
  if (button == "SET-") return "set";
  if (button == "CANCEL") return "cancel";
  return "";
}

String updateDebouncedButton(const String& rawButton, uint32_t now) {
  if (rawButton != candidateButton) {
    candidateButton = rawButton;
    candidateSince = now;
    return stableButton;
  }

  if (stableButton != candidateButton && elapsed(now, candidateSince + DEBOUNCE_MS)) {
    stableButton = candidateButton;
  }

  return stableButton;
}
}

void setupCruiseInput() {
  analogReadResolution(12);
  pinMode(Pins::ADC_CRUISE, INPUT);
}

void updateCruiseInput() {
  currentAdc = analogRead(Pins::ADC_CRUISE);
  const String button = updateDebouncedButton(detectCruiseButton(currentAdc), millis());

  if (button == "NONE") {
    lastPulseButton = "NONE";
    return;
  }

  if (button == lastPulseButton) return;

  const String outputName = outputNameForButton(button);
  if (outputName.length() == 0) return;

  pulseOutput(outputName, getPulseMs());
  lastPulseButton = button;
}

int getCruiseAdc() {
  return currentAdc;
}

String getCruiseButton() {
  return stableButton;
}
