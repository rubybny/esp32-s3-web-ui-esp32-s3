#include "Outputs.h"

#include "Pins.h"

namespace {
bool outMain = false;
bool outUpSet = false;
bool outDownRes = false;
bool outCancel = false;
bool outBrake = false;
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
  if (name == "main") {
    outMain = state;
    digitalWrite(Pins::OUT_MAIN, state ? HIGH : LOW);
    return true;
  }
  if (name == "upSet") {
    outUpSet = state;
    digitalWrite(Pins::OUT_UPSET, state ? HIGH : LOW);
    return true;
  }
  if (name == "downRes") {
    outDownRes = state;
    digitalWrite(Pins::OUT_DOWNRES, state ? HIGH : LOW);
    return true;
  }
  if (name == "cancel") {
    outCancel = state;
    digitalWrite(Pins::OUT_CANCEL, state ? HIGH : LOW);
    return true;
  }
  if (name == "brakeOut") {
    outBrake = state;
    digitalWrite(Pins::OUT_BRAKE, state ? HIGH : LOW);
    return true;
  }
  return false;
}

void resetOutputs() {
  setOutputState("main", false);
  setOutputState("upSet", false);
  setOutputState("downRes", false);
  setOutputState("cancel", false);
  setOutputState("brakeOut", false);
}
