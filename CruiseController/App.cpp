#include "App.h"

#include <Arduino.h>

#include "ActionHandler.h"
#include "AdcFilter.h"
#include "BrakeInput.h"
#include "Calibration.h"
#include "Config.h"
#include "LeverState.h"
#include "Logger.h"
#include "Outputs.h"
#include "SerialCommands.h"

namespace {
uint32_t lastHeartbeatAt = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

void printHeartbeatIfDue(uint32_t now) {
  if (!elapsed(now, lastHeartbeatAt + LogConfig::HEARTBEAT_MS)) return;
  lastHeartbeatAt = now;

  LeverButton confirmed = getConfirmedLeverState();
  debugf("HB raw=%4d filtered=%4d candidate=%-7s confirmed=%-7s held=%lums main=%d res=%d set=%d cancel=%d",
         readAdc(), filterAdc(), leverButtonName(getCandidateLeverState()), leverButtonName(confirmed),
         static_cast<unsigned long>(getLeverStateHoldMs(now)), getOutputState(LeverButton::MAIN),
         getOutputState(LeverButton::RES), getOutputState(LeverButton::SET),
         getOutputState(LeverButton::CANCEL));
}
}  // namespace

void appSetup() {
  // Output pins are configured first and forced to their inactive level
  // before anything else in the system runs (ADC, serial, calibration
  // load), so there is no window during boot where a vehicle line could be
  // driven active from floating/undefined GPIO state.
  setupOutputPins();

  setupLogger();
  logLine("CC-S3 cruise controller starting (serial-only build)");
#if USE_BRAKE_INPUT
  logLine("GPIO9 role: BRAKE input");
#else
  logLine("GPIO9 role: CANCEL output");
#endif

  setupAdcFilter();
  setupLeverState();
  setupBrakeInput();
  setupActionHandler();
  loadAdcCalibration();
  setupSerialCommands();

  logLine("Ready");
}

void appLoop() {
  uint32_t now = millis();

  updateAdcFilter(now);
  updateLeverState(now);
  updateBrakeInput(now);
  handleLeverAction(now);
  updateSerialCommands();

  printHeartbeatIfDue(now);
}
