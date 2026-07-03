#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

extern String configJson;

void beginConfigStore();
String defaultConfigJson();
bool parseConfig(StaticJsonDocument<1024>& doc);
void saveConfigJson(const String& json);
void resetConfig();
uint32_t getTimingMs(const char* key, uint32_t fallback);
uint32_t getPulseMs();
uint32_t getPulseMsForOutput(const char* key);
int getLearnedAdc(const char* key);
bool setLearnedAdc(const char* key, int adc);
