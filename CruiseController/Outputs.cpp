#include "Outputs.h"

#include "Config.h"
#include "Logger.h"

namespace {
bool outMain = false;
bool outRes = false;
bool outSet = false;
bool outCancel = false;

int activeLevel() { return OUTPUT_ACTIVE_LEVEL == ActiveLevel::ACTIVE_HIGH ? HIGH : LOW; }
int inactiveLevel() { return OUTPUT_ACTIVE_LEVEL == ActiveLevel::ACTIVE_HIGH ? LOW : HIGH; }

void writePin(uint8_t pin, bool on) {
  digitalWrite(pin, on ? activeLevel() : inactiveLevel());
}

void writeAllPins() {
  writePin(Pins::OUT_MAIN, outMain);
  writePin(Pins::OUT_RES, outRes);
  writePin(Pins::OUT_SET, outSet);
#if !USE_BRAKE_INPUT
  writePin(Pins::PIN_9, outCancel);
#endif
}

// Clears every output's *state variable* without touching the pins yet.
// setOutputForButton()/allOutputsOff() call writeAllPins() once afterwards,
// so switching from one output to another never has both physically
// energized at the same time, even for a single GPIO write cycle.
void clearStates() {
  outMain = false;
  outRes = false;
  outSet = false;
  outCancel = false;
}
}  // namespace

void setupOutputPins() {
  // Configure as INPUT (high-Z) first, then explicitly force the inactive
  // level as soon as the pin becomes an output, so there is no window where
  // the pin could glitch to its active level while direction/level are set
  // up separately.
  pinMode(Pins::OUT_MAIN, INPUT);
  pinMode(Pins::OUT_RES, INPUT);
  pinMode(Pins::OUT_SET, INPUT);

  clearStates();

  digitalWrite(Pins::OUT_MAIN, inactiveLevel());
  digitalWrite(Pins::OUT_RES, inactiveLevel());
  digitalWrite(Pins::OUT_SET, inactiveLevel());
  pinMode(Pins::OUT_MAIN, OUTPUT);
  pinMode(Pins::OUT_RES, OUTPUT);
  pinMode(Pins::OUT_SET, OUTPUT);

#if !USE_BRAKE_INPUT
  pinMode(Pins::PIN_9, INPUT);
  digitalWrite(Pins::PIN_9, inactiveLevel());
  pinMode(Pins::PIN_9, OUTPUT);
#endif
  // If USE_BRAKE_INPUT is 1, GPIO9's pinMode(INPUT) is set up by
  // BrakeInput::setupBrakeInput() instead -- this module never touches it
  // in that build, so setCancelOutput() below is a deliberate no-op.

  writeAllPins();
}

void setMainOutput(bool on) {
  if (on) clearStates();
  outMain = on;
  writeAllPins();
}

void setResOutput(bool on) {
  if (on) clearStates();
  outRes = on;
  writeAllPins();
}

void setSetOutput(bool on) {
  if (on) clearStates();
  outSet = on;
  writeAllPins();
}

void setCancelOutput(bool on) {
#if USE_BRAKE_INPUT
  if (on) {
    logLine("setCancelOutput(true) ignored -- GPIO9 is configured as BRAKE input in this build");
  }
  return;
#else
  if (on) clearStates();
  outCancel = on;
  writeAllPins();
#endif
}

void allOutputsOff() {
  clearStates();
  writeAllPins();
}

void setOutputForButton(LeverButton button, bool on) {
  switch (button) {
    case LeverButton::MAIN: setMainOutput(on); break;
    case LeverButton::CANCEL: setCancelOutput(on); break;
    case LeverButton::RES: setResOutput(on); break;
    case LeverButton::SET: setSetOutput(on); break;
    default: break;
  }
}

bool getOutputState(LeverButton button) {
  switch (button) {
    case LeverButton::MAIN: return outMain;
    case LeverButton::CANCEL: return outCancel;
    case LeverButton::RES: return outRes;
    case LeverButton::SET: return outSet;
    default: return false;
  }
}
