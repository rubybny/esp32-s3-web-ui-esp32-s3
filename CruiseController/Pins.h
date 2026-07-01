#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t ADC_STEER = 4;
constexpr uint8_t ADC_VIN = 11;
constexpr uint8_t ADC_VOUT = 12;
constexpr uint8_t BRAKE_IN = 10;
constexpr uint8_t OUT_MAIN = 5;
constexpr uint8_t OUT_UPSET = 6;
constexpr uint8_t OUT_DOWNRES = 7;
constexpr uint8_t OUT_CANCEL = 8;
constexpr uint8_t OUT_BRAKE = 9;

constexpr uint8_t OUT_NAV_VOLUP = 35;
constexpr uint8_t OUT_NAV_VOLDOWN = 36;
constexpr uint8_t OUT_NAV_SEEKPLUS = 37;
constexpr uint8_t OUT_NAV_SEEKMINUS = 38;
constexpr uint8_t OUT_NAV_MODE = 39;

constexpr uint8_t BRAKE_IN_MODE = INPUT_PULLDOWN;
constexpr uint8_t BRAKE_IN_ACTIVE_LEVEL = HIGH;
}
