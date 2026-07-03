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

uint32_t normalizeMs(uint32_t value, uint32_t fallback = DEFAULT_PULSE_MS) {
  return value <= MAX_PULSE_MS ? value : fallback;
}

JsonObject ensureObject(JsonDocument& doc, const char* key) {
  if (doc[key].is<JsonObject>()) return doc[key].as<JsonObject>();
  return doc[key].to<JsonObject>();
}
}

String configJson;

String defaultConfigJson() {
  StaticJsonDocument<512> doc;
  doc["pulseMs"] = DEFAULT_PULSE_MS;
  JsonObject timing = ensureObject(doc, "timing");
  timing["main"] = DEFAULT_PULSE_MS;
  timing["res"] = DEFAULT_PULSE_MS;
  timing["set"] = DEFAULT_PULSE_MS;
  timing["cancel"] = DEFAULT_PULSE_MS;
  JsonObject learned = ensureObject(doc, "learned");
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
    return;
  }

  serializeJson(doc, configJson);
  prefs.putString("config", configJson);
}

bool parseConfig(StaticJsonDocument<1024>& doc) {
  DeserializationError err = deserializeJson(doc, configJson);
  if (err) return false;
  if (!doc["pulseMs"].is<uint32_t>()) return false;

  uint32_t pulseMs = doc["pulseMs"] | DEFAULT_PULSE_MS;
  pulseMs = normalizeMs(pulseMs);
  doc["pulseMs"] = pulseMs;

  JsonObject timing = ensureObject(doc, "timing");
  timing["main"] = normalizeMs(timing["main"] | pulseMs, pulseMs);
  timing["res"] = normalizeMs(timing["res"] | pulseMs, pulseMs);
  timing["set"] = normalizeMs(timing["set"] | pulseMs, pulseMs);
  timing["cancel"] = normalizeMs(timing["cancel"] | pulseMs, pulseMs);

  JsonObject learned = ensureObject(doc, "learned");
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
  pulseMs = normalizeMs(pulseMs);

  StaticJsonDocument<512> normalized;
  normalized["pulseMs"] = pulseMs;
  JsonObject timing = ensureObject(normalized, "timing");
  timing["main"] = normalizeMs(doc["timing"]["main"] | pulseMs, pulseMs);
  timing["res"] = normalizeMs(doc["timing"]["res"] | pulseMs, pulseMs);
  timing["set"] = normalizeMs(doc["timing"]["set"] | pulseMs, pulseMs);
  timing["cancel"] = normalizeMs(doc["timing"]["cancel"] | pulseMs, pulseMs);
  JsonObject learned = ensureObject(normalized, "learned");
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
  uint32_t value = getPulseMsForOutput(key);
  if (value <= MAX_PULSE_MS) return value;
  return fallback;
}

uint32_t getPulseMs() {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return DEFAULT_PULSE_MS;

  return doc["pulseMs"] | DEFAULT_PULSE_MS;
}

uint32_t getPulseMsForOutput(const char* key) {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return DEFAULT_PULSE_MS;

  uint32_t fallback = doc["pulseMs"] | DEFAULT_PULSE_MS;
  return normalizeMs(doc["timing"][key] | fallback, fallback);
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
