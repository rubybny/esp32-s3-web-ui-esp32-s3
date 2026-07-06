#include "CruiseInput.h"

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

namespace {
constexpr uint32_t DEBOUNCE_MS = 30;

int currentAdc = 4095;
String rawButton = "NONE";
String candidateButton = "NONE";
String stableButton = "NONE";
String displayButton = "NONE";
String activeInputOutput = "";
uint32_t candidateSince = 0;
uint32_t minOutputUntil = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

String detectCruiseButton(int adc) {
  struct Entry {
    const char* key;
    const char* label;
    int value;
  };

  Entry entries[] = {
      {"main", "MAIN", getLearnedAdc("main")},
      {"cancel", "CANCEL", getLearnedAdc("cancel")},
      {"res", "RES+", getLearnedAdc("res")},
      {"set", "SET-", getLearnedAdc("set")},
  };

  if (adc <= 0 && entries[0].value == 0) return String("MAIN");
  if (adc >= 4095) return String("NONE");

  for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
    for (size_t j = i + 1; j < sizeof(entries) / sizeof(entries[0]); j++) {
      if (entries[j].value < entries[i].value) {
        Entry tmp = entries[i];
        entries[i] = entries[j];
        entries[j] = tmp;
      }
    }
  }

  for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
    const Entry& entry = entries[i];
    if (entry.value <= 0 && String(entry.key) != "main") continue;
    if (entry.value >= 4095) continue;
    if (String(entry.key) == "main") continue;

    int lower = 1;
    int upper = 4094;

    for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
      if (entries[j].value <= 0 || entries[j].value >= 4095) continue;
      lower = ((entries[j].value + entry.value) / 2) + 1;
      break;
    }

    for (size_t j = i + 1; j < sizeof(entries) / sizeof(entries[0]); j++) {
      if (entries[j].value <= 0 || entries[j].value >= 4095) continue;
      upper = (entry.value + entries[j].value) / 2;
      break;
    }

    if (adc >= lower && adc <= upper) return String(entry.label);
  }

  return String("NONE");
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
  uint32_t now = millis();
  const String button = updateDebouncedButton(rawButton, now);
  displayButton = rawButton != "NONE" ? rawButton : button;

  if (rawButton == "NONE") {
    if (activeInputOutput.length() > 0) {
      if (elapsed(now, minOutputUntil)) {
        resetOutputs();
        activeInputOutput = "";
      }
    }
    return;
  }

  const String outputName = outputNameForButton(rawButton);
  if (outputName.length() == 0) return;
  if (outputName == activeInputOutput) return;

  setOutputState(outputName, true);
  activeInputOutput = outputName;
  minOutputUntil = now + getPulseMsForOutput(outputName.c_str());
}

int getCruiseAdc() {
  return currentAdc;
}

String getCruiseButton() {
  return displayButton;
}
