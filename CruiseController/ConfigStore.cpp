#include "ConfigStore.h"

#include <Preferences.h>

namespace {
constexpr uint32_t DEFAULT_PULSE_MS = 200;
constexpr uint32_t MAX_PULSE_MS = 5000;

Preferences prefs;
}

String configJson;

String defaultConfigJson() {
  StaticJsonDocument<128> doc;
  doc["pulseMs"] = DEFAULT_PULSE_MS;

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

  StaticJsonDocument<128> normalized;
  normalized["pulseMs"] = pulseMs;
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
