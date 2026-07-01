#include "NaviOutput.h"

#include "Pins.h"

namespace {
bool volUp = false;
bool volDown = false;
bool seekPlus = false;
bool seekMinus = false;
bool mode = false;

void writePins() {
  digitalWrite(Pins::OUT_NAV_VOLUP, volUp ? HIGH : LOW);
  digitalWrite(Pins::OUT_NAV_VOLDOWN, volDown ? HIGH : LOW);
  digitalWrite(Pins::OUT_NAV_SEEKPLUS, seekPlus ? HIGH : LOW);
  digitalWrite(Pins::OUT_NAV_SEEKMINUS, seekMinus ? HIGH : LOW);
  digitalWrite(Pins::OUT_NAV_MODE, mode ? HIGH : LOW);
}

void clearStates() {
  volUp = false;
  volDown = false;
  seekPlus = false;
  seekMinus = false;
  mode = false;
}
}

void setupNaviOutputPins() {
  pinMode(Pins::OUT_NAV_VOLUP, OUTPUT);
  pinMode(Pins::OUT_NAV_VOLDOWN, OUTPUT);
  pinMode(Pins::OUT_NAV_SEEKPLUS, OUTPUT);
  pinMode(Pins::OUT_NAV_SEEKMINUS, OUTPUT);
  pinMode(Pins::OUT_NAV_MODE, OUTPUT);
  resetNaviOutput();
}

void resetNaviOutput() {
  clearStates();
  writePins();
}

bool getNaviOutputState(const String& buttonName) {
  if (buttonName == "VolUp") return volUp;
  if (buttonName == "VolDown") return volDown;
  if (buttonName == "SeekPlus") return seekPlus;
  if (buttonName == "SeekMinus") return seekMinus;
  if (buttonName == "Mode") return mode;
  return false;
}

void setNaviOutput(const String& buttonName) {
  clearStates();

  if (buttonName == "VolUp") {
    volUp = true;
  } else if (buttonName == "VolDown") {
    volDown = true;
  } else if (buttonName == "SeekPlus") {
    seekPlus = true;
  } else if (buttonName == "SeekMinus") {
    seekMinus = true;
  } else if (buttonName == "Mode") {
    mode = true;
  }

  writePins();
}
