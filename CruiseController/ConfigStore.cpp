#include "ConfigStore.h"

#include <Preferences.h>

namespace {
constexpr uint32_t DEFAULT_PULSE_MS = 200;
constexpr uint32_t MAX_PULSE_MS = 5000;
constexpr int DEFAULT_MAIN_ADC = 0;
constexpr int DEFAULT_CANCEL_ADC = 96;
constexpr int DEFAULT_RES_ADC = 153;
constexpr int DEFAULT_SET_ADC = 341;

Preferences prefs;
}

String configJson;

String defaultConfigJson() {
  StaticJsonDocument<256> doc;
  doc["pulseMs"] = DEFAULT_PULSE_MS;
  JsonObject learned = doc["learned"].to<JsonObject>();
  learned["main"] = DEFAULT_MAIN_ADC;
  learned["cancel"] = DEFAULT_CANCEL_ADC;
  learned["res"] = DEFAULT_RES_ADC;
  learned["set"] = DEFAULT_SET_ADC;

  String json;
  serializeJson(doc, json);
  return json;
}

void beginConfigStore() {
  prefs.begin("cc-s3", false);
  configJson = prefs.getString("config", "");

  StaticJsonDocument<1024> doc;
  if (configJson.length() == 0 || !parseConfig(doc)) {
    resetConfig();
  }
}

bool parseConfig(StaticJsonDocument<1024>& doc) {
  DeserializationError err = deserializeJson(doc, configJson);
  if (err) return false;
  if (!doc["pulseMs"].is<uint32_t>()) return false;

  uint32_t pulseMs = doc["pulseMs"] | DEFAULT_PULSE_MS;
  if (pulseMs == 0 || pulseMs > MAX_PULSE_MS) {
    doc["pulseMs"] = DEFAULT_PULSE_MS;
  }

  JsonObject learned = doc["learned"].to<JsonObject>();
  if (!learned["main"].is<int>()) learned["main"] = DEFAULT_MAIN_ADC;
  if (!learned["cancel"].is<int>()) learned["cancel"] = DEFAULT_CANCEL_ADC;
  if (!learned["res"].is<int>()) learned["res"] = DEFAULT_RES_ADC;
  if (!learned["set"].is<int>()) learned["set"] = DEFAULT_SET_ADC;
  return true;
}

void saveConfigJson(const String& json) {
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) return;

  uint32_t pulseMs = doc["pulseMs"] | DEFAULT_PULSE_MS;
  if (pulseMs == 0 || pulseMs > MAX_PULSE_MS) {
    pulseMs = DEFAULT_PULSE_MS;
  }

  StaticJsonDocument<256> normalized;
  normalized["pulseMs"] = pulseMs;
  JsonObject learned = normalized["learned"].to<JsonObject>();
  learned["main"] = constrain(doc["learned"]["main"] | DEFAULT_MAIN_ADC, 0, 4095);
  learned["cancel"] = constrain(doc["learned"]["cancel"] | DEFAULT_CANCEL_ADC, 0, 4095);
  learned["res"] = constrain(doc["learned"]["res"] | DEFAULT_RES_ADC, 0, 4095);
  learned["set"] = constrain(doc["learned"]["set"] | DEFAULT_SET_ADC, 0, 4095);
  serializeJson(normalized, configJson);
  prefs.putString("config", configJson);
}

void resetConfig() {
  saveConfigJson(defaultConfigJson());
}

uint32_t getTimingMs(const char* key, uint32_t fallback) {
  if (String(key) == "pulseMs") return getPulseMs();
  return fallback;
}

uint32_t getPulseMs() {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return DEFAULT_PULSE_MS;

  return doc["pulseMs"] | DEFAULT_PULSE_MS;
}

int getLearnedAdc(const char* key) {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return 0;

  return doc["learned"][key] | 0;
}

bool setLearnedAdc(const char* key, int adc) {
  String name(key);
  if (name != "main" && name != "cancel" && name != "res" && name != "set") return false;

  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return false;

  doc["learned"][key] = constrain(adc, 0, 4095);
  String json;
  serializeJson(doc, json);
  saveConfigJson(json);
  return true;
}
