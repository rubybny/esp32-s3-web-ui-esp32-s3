#include "SerialCommands.h"

#include "AdcFilter.h"
#include "Calibration.h"
#include "Logger.h"

namespace {
void printHelp() {
  logLine("---- Serial commands ----");
  logLine("m = learn MAIN center from current filtered ADC");
  logLine("c = learn CANCEL center from current filtered ADC");
  logLine("r = learn RES+ center from current filtered ADC");
  logLine("s = learn SET- center from current filtered ADC");
  logLine("n = learn NEUTRAL threshold from current filtered ADC (release lever fully first)");
  logLine("x = reset calibration to Config.h defaults");
  logLine("p = print current calibration");
  logLine("h = this help");
  logLine("--------------------------");
}

void handleCommand(char c) {
  switch (c) {
    case 'm': learnAdcCenter(LeverButton::MAIN, filterAdc()); break;
    case 'c': learnAdcCenter(LeverButton::CANCEL, filterAdc()); break;
    case 'r': learnAdcCenter(LeverButton::RES, filterAdc()); break;
    case 's': learnAdcCenter(LeverButton::SET, filterAdc()); break;
    case 'n': learnAdcNeutral(filterAdc()); break;
    case 'x': resetAdcCalibration(); break;
    case 'p': printAdcCalibration(); break;
    case 'h':
    case '?': printHelp(); break;
    case '\r':
    case '\n': break;
    default: logf("Unknown command '%c' -- send 'h' for help", c); break;
  }
}
}  // namespace

void setupSerialCommands() { printHelp(); }

void updateSerialCommands() {
  while (Serial.available() > 0) {
    handleCommand(static_cast<char>(Serial.read()));
  }
}
