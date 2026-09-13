#include "App.h"

#include <Arduino.h>

#include "Config.h"
#include "CruiseInput.h"
#include "Logger.h"
#include "Outputs.h"

namespace {
uint32_t lastHeartbeatAt = 0;

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

void logHeartbeatIfDue() {
  const uint32_t now = millis();
  if (!elapsed(now, lastHeartbeatAt + LogConfig::HEARTBEAT_MS)) return;
  lastHeartbeatAt = now;

  logf("adc=%d button=%-5s main=%d res=%d set=%d cancel=%d", getCruiseAdc(), getCruiseButton(),
       getOutputState("main"), getOutputState("res"), getOutputState("set"), getOutputState("cancel"));
}
}  // namespace

void appSetup() {
  setupLogger();
  logLine("CC-S3 cruise controller starting");

  setupOutputPins();
  setupCruiseInput();

  logLine("Ready");
}

void appLoop() {
  updateCruiseInput();
  logHeartbeatIfDue();
}
