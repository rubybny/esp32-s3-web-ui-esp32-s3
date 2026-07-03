#include "CruiseInput.h"

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr int DETECT_THRESHOLD = 300;

int currentAdc = 4095;
String rawButton = "NONE";
String candidateButton = "NONE";
String stableButton = "NONE";
String displayButton = "NONE";
String activeInputOutput = "";
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
  rawButton = detectCruiseButton(currentAdc);
  const String button = updateDebouncedButton(rawButton, millis());
  displayButton = rawButton != "NONE" ? rawButton : button;

  if (rawButton == "NONE") {
    if (activeInputOutput.length() > 0) {
      resetOutputs();
      activeInputOutput = "";
    }
    return;
  }

  const String outputName = outputNameForButton(rawButton);
  if (outputName.length() == 0) return;
  if (outputName == activeInputOutput) return;

  setOutputState(outputName, true);
  activeInputOutput = outputName;
}

int getCruiseAdc() {
  return currentAdc;
}

String getCruiseButton() {
  return displayButton;
}
