#include "Outputs.h"

#include "Pins.h"

namespace {
bool outMain = false;
bool outUpSet = false;
bool outDownRes = false;
bool outCancel = false;
bool outBrake = false;

void writeOutputPins() {
  digitalWrite(Pins::OUT_MAIN, outMain ? HIGH : LOW);
  digitalWrite(Pins::OUT_UPSET, outUpSet ? HIGH : LOW);
  digitalWrite(Pins::OUT_DOWNRES, outDownRes ? HIGH : LOW);
  digitalWrite(Pins::OUT_CANCEL, outCancel ? HIGH : LOW);
  digitalWrite(Pins::OUT_BRAKE, outBrake ? HIGH : LOW);
}

void clearOutputStates() {
  outMain = false;
  outUpSet = false;
  outDownRes = false;
  outCancel = false;
  outBrake = false;
}
}

void setupOutputPins() {
  pinMode(Pins::OUT_MAIN, OUTPUT);
  pinMode(Pins::OUT_UPSET, OUTPUT);
  pinMode(Pins::OUT_DOWNRES, OUTPUT);
  pinMode(Pins::OUT_CANCEL, OUTPUT);
  pinMode(Pins::OUT_BRAKE, OUTPUT);
  resetOutputs();
}

bool isValidOutputName(const String& name) {
  return name == "main" || name == "upSet" || name == "downRes" || name == "cancel" || name == "brakeOut";
}

bool getOutputState(const String& name) {
  if (name == "main") return outMain;
  if (name == "upSet") return outUpSet;
  if (name == "downRes") return outDownRes;
  if (name == "cancel") return outCancel;
  if (name == "brakeOut") return outBrake;
  return false;
}

bool setOutputState(const String& name, bool state) {
  if (!isValidOutputName(name)) return false;

  if (state) {
    clearOutputStates();
  }

  if (name == "main") {
    outMain = state;
  } else if (name == "upSet") {
    outUpSet = state;
  } else if (name == "downRes") {
    outDownRes = state;
  } else if (name == "cancel") {
    outCancel = state;
  } else if (name == "brakeOut") {
    outBrake = state;
  }

  writeOutputPins();
  return true;
}

void resetOutputs() {
  clearOutputStates();
  writeOutputPins();
}
