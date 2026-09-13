#include "Outputs.h"

#include <string.h>

#include "Config.h"

namespace {
bool outMain = false;
bool outRes = false;
bool outSet = false;
bool outCancel = false;

void writeOutputPins() {
  digitalWrite(Pins::OUT_MAIN, outMain ? HIGH : LOW);
  digitalWrite(Pins::OUT_RES, outRes ? HIGH : LOW);
  digitalWrite(Pins::OUT_SET, outSet ? HIGH : LOW);
  digitalWrite(Pins::OUT_BRAKE, outCancel ? HIGH : LOW);
}

void clearOutputStates() {
  outMain = false;
  outRes = false;
  outSet = false;
  outCancel = false;
}
}  // namespace

void setupOutputPins() {
  pinMode(Pins::OUT_MAIN, OUTPUT);
  pinMode(Pins::OUT_RES, OUTPUT);
  pinMode(Pins::OUT_SET, OUTPUT);
  pinMode(Pins::OUT_BRAKE, OUTPUT);
  resetOutputs();
}

bool isValidOutputName(const char* name) {
  return strcmp(name, "main") == 0 || strcmp(name, "res") == 0 || strcmp(name, "set") == 0 ||
         strcmp(name, "cancel") == 0;
}

bool getOutputState(const char* name) {
  if (strcmp(name, "main") == 0) return outMain;
  if (strcmp(name, "res") == 0) return outRes;
  if (strcmp(name, "set") == 0) return outSet;
  if (strcmp(name, "cancel") == 0) return outCancel;
  return false;
}

bool setOutputState(const char* name, bool state) {
  if (!isValidOutputName(name)) return false;

  // Only one output may be energized at a time: clear everything before
  // turning the requested one on.
  if (state) clearOutputStates();

  if (strcmp(name, "main") == 0) {
    outMain = state;
  } else if (strcmp(name, "res") == 0) {
    outRes = state;
  } else if (strcmp(name, "set") == 0) {
    outSet = state;
  } else if (strcmp(name, "cancel") == 0) {
    outCancel = state;
  }

  writeOutputPins();
  return true;
}

void resetOutputs() {
  clearOutputStates();
  writeOutputPins();
}
