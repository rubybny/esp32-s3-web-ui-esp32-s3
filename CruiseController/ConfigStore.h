#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

extern String configJson;

void beginConfigStore();
String defaultConfigJson();
bool parseConfig(StaticJsonDocument<1024>& doc);
void saveConfigJson(const String& json);
void resetConfig();
bool setLearnedValue(const char* side, const char* name, int adc);
bool isValidLearnTarget(const char* side, const char* name);
String detectButton(int adc);
uint32_t getTimingMs(const char* key, uint32_t fallback);
