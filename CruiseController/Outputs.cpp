#include "Outputs.h"

#include "Pins.h"

namespace {
bool outMain = false;
bool outRes = false;
bool outSet = false;
bool outBrake = false;
uint32_t pulseUntil = 0;

void writeOutputPins() {
  digitalWrite(Pins::OUT_MAIN, outMain ? HIGH : LOW);
  digitalWrite(Pins::OUT_RES, outRes ? HIGH : LOW);
  digitalWrite(Pins::OUT_SET, outSet ? HIGH : LOW);
  digitalWrite(Pins::OUT_BRAKE, outBrake ? HIGH : LOW);
}

void clearOutputStates() {
  outMain = false;
  outRes = false;
  outSet = false;
  outBrake = false;
}

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}
}

void setupOutputPins() {
  pinMode(Pins::OUT_MAIN, OUTPUT);
  pinMode(Pins::OUT_RES, OUTPUT);
  pinMode(Pins::OUT_SET, OUTPUT);
  pinMode(Pins::OUT_BRAKE, OUTPUT);
  resetOutputs();
}

bool isValidOutputName(const String& name) {
  return name == "main" || name == "res" || name == "set" || name == "cancel" || name == "brake" ||
         name == "upSet" || name == "downRes" || name == "brakeOut";
}

bool getOutputState(const String& name) {
  if (name == "main") return outMain;
  if (name == "res" || name == "upSet") return outRes;
  if (name == "set" || name == "downRes") return outSet;
  if (name == "cancel" || name == "brake" || name == "brakeOut") return outBrake;
  return false;
}

bool setOutputState(const String& name, bool state) {
  if (!isValidOutputName(name)) return false;

  if (state) {
    clearOutputStates();
    pulseUntil = 0;
  }

  if (name == "main") {
    outMain = state;
  } else if (name == "res" || name == "upSet") {
    outRes = state;
  } else if (name == "set" || name == "downRes") {
    outSet = state;
  } else if (name == "cancel" || name == "brake" || name == "brakeOut") {
    outBrake = state;
  }

  writeOutputPins();
  return true;
}

bool pulseOutput(const String& name, uint32_t durationMs) {
  if (!isValidOutputName(name)) return false;

  resetOutputs();
  if (durationMs == 0) return true;

  if (!setOutputState(name, true)) return false;
  pulseUntil = millis() + durationMs;
  return true;
}

void updateOutputPulses() {
  if (pulseUntil != 0 && elapsed(millis(), pulseUntil)) {
    resetOutputs();
  }
}

void resetOutputs() {
  clearOutputStates();
  pulseUntil = 0;
  writeOutputPins();
}
