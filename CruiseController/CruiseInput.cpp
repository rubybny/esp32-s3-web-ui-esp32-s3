#include "CruiseInput.h"

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr int DETECT_THRESHOLD = 70;

int currentAdc = 4095;
String candidateButton = "NONE";
String stableButton = "NONE";
String lastPulseButton = "NONE";
uint32_t candidateSince = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

String detectCruiseButton(int adc) {
  struct Entry {
    const char* key;
    const char* label;
  };

  const Entry entries[] = {
      {"main", "MAIN"},
      {"cancel", "CANCEL"},
      {"res", "RES+"},
      {"set", "SET-"},
  };

  int bestDiff = DETECT_THRESHOLD + 1;
  const char* bestLabel = "NONE";

  for (const auto& entry : entries) {
    int learned = getLearnedAdc(entry.key);
    int diff = abs(adc - learned);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestLabel = entry.label;
    }
  }

  return bestDiff <= DETECT_THRESHOLD ? String(bestLabel) : String("NONE");
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
