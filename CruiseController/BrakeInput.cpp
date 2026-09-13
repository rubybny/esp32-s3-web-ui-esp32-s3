#include "BrakeInput.h"

#include "Config.h"
#include "Logger.h"

namespace {
bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

#if USE_BRAKE_INPUT
int rawActiveLevel() { return BRAKE_ACTIVE_LEVEL == ActiveLevel::ACTIVE_HIGH ? HIGH : LOW; }

bool candidateActive = false;
uint32_t candidateSince = 0;
bool confirmedActive = false;
#endif
}  // namespace

void setupBrakeInput() {
#if USE_BRAKE_INPUT
  pinMode(Pins::PIN_9, INPUT);
  logLine("BRAKE input enabled on GPIO9 (CANCEL output is unavailable in this build)");
#else
  // Nothing to do -- GPIO9 is owned by Outputs.cpp as the CANCEL line.
#endif
}

void updateBrakeInput(uint32_t now) {
#if USE_BRAKE_INPUT
  bool raw = digitalRead(Pins::PIN_9) == rawActiveLevel();
  if (raw != candidateActive) {
    candidateActive = raw;
    candidateSince = now;
  } else if (confirmedActive != candidateActive && elapsed(now, candidateSince + BrakeConfig::DEBOUNCE_MS)) {
    confirmedActive = candidateActive;
    logf("BRAKE %s", confirmedActive ? "ACTIVE" : "released");
  }
#else
  (void)now;
#endif
}

bool isBrakeActive() {
#if USE_BRAKE_INPUT
  return confirmedActive;
#else
  return false;
#endif
}
