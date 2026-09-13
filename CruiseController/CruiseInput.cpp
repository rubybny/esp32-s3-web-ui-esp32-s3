#include "CruiseInput.h"

#include "Config.h"
#include "Logger.h"
#include "Outputs.h"

namespace {
enum class Button { NONE, MAIN, CANCEL, RES, SET };

const char* buttonLabel(Button b) {
  switch (b) {
    case Button::MAIN: return "MAIN";
    case Button::CANCEL: return "CANCEL";
    case Button::RES: return "RES+";
    case Button::SET: return "SET-";
    default: return "NONE";
  }
}

const char* outputNameFor(Button b) {
  switch (b) {
    case Button::MAIN: return "main";
    case Button::CANCEL: return "cancel";
    case Button::RES: return "res";
    case Button::SET: return "set";
    default: return "";
  }
}

uint32_t pulseMsFor(Button b) {
  switch (b) {
    case Button::MAIN: return CruiseConfig::PULSE_MAIN_MS;
    case Button::CANCEL: return CruiseConfig::PULSE_CANCEL_MS;
    case Button::RES: return CruiseConfig::PULSE_RES_MS;
    case Button::SET: return CruiseConfig::PULSE_SET_MS;
    default: return 0;
  }
}

Button classifyButton(int adc) {
  using namespace CruiseConfig;
  if (adc < BOUND_MAIN_CANCEL) return Button::MAIN;
  if (adc < BOUND_CANCEL_RES) return Button::CANCEL;
  if (adc < BOUND_RES_SET) return Button::RES;
  if (adc < ADC_OPEN_MIN) return Button::SET;
  return Button::NONE;
}

bool elapsed(uint32_t now, uint32_t target) {
  return static_cast<int32_t>(now - target) >= 0;
}

int currentAdc = 4095;

// Raw classification must hold steady for DEBOUNCE_MS before it becomes
// `confirmedButton`. Only the confirmed value ever touches the outputs, so
// a single noisy ADC sample can no longer fire a wrong or unintended pulse.
Button candidateButton = Button::NONE;
uint32_t candidateSince = 0;
Button confirmedButton = Button::NONE;

// Tracks which output the confirmed button turned on, and the earliest
// time it may be released, so a very brief tap still produces a pulse the
// receiving module can register.
Button activeOutputButton = Button::NONE;
uint32_t minHoldUntil = 0;
}  // namespace

void setupCruiseInput() {
  analogReadResolution(12);
  pinMode(Pins::ADC_CRUISE, INPUT);
}

void updateCruiseInput() {
  currentAdc = analogRead(Pins::ADC_CRUISE);
  const Button raw = classifyButton(currentAdc);
  const uint32_t now = millis();

  if (raw != candidateButton) {
    candidateButton = raw;
    candidateSince = now;
  } else if (confirmedButton != candidateButton && elapsed(now, candidateSince + CruiseConfig::DEBOUNCE_MS)) {
    const Button previous = confirmedButton;
    confirmedButton = candidateButton;
    logf("BUTTON %s -> %s (adc=%d)", buttonLabel(previous), buttonLabel(confirmedButton), currentAdc);
  }

  if (confirmedButton == Button::NONE) {
    if (activeOutputButton != Button::NONE && elapsed(now, minHoldUntil)) {
      resetOutputs();
      logf("OUTPUT %s OFF", outputNameFor(activeOutputButton));
      activeOutputButton = Button::NONE;
    }
    return;
  }

  if (confirmedButton == activeOutputButton) return;

  const char* outputName = outputNameFor(confirmedButton);
  setOutputState(outputName, true);
  activeOutputButton = confirmedButton;
  minHoldUntil = now + pulseMsFor(confirmedButton);
  logf("OUTPUT %s ON (min %lums)", outputName, static_cast<unsigned long>(pulseMsFor(confirmedButton)));
}

int getCruiseAdc() { return currentAdc; }

const char* getCruiseButton() { return buttonLabel(confirmedButton); }
