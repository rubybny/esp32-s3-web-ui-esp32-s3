#include "ConfigStore.h"

#include <Preferences.h>
#include <string.h>

namespace {
constexpr int DETECT_THRESHOLD = 100;

struct LearnedEntry {
  const char* side;
  const char* name;
};

const LearnedEntry LEARNED_ENTRIES[] = {
    {"L", "VolUp"},
    {"L", "VolDown"},
    {"L", "SeekPlus"},
    {"L", "SeekMinus"},
    {"L", "Mode"},
    {"R", "Main"},
    {"R", "Cancel"},
    {"R", "UpSet"},
    {"R", "DownRes"},
};

Preferences prefs;
}

String configJson;

String defaultConfigJson() {
  StaticJsonDocument<768> doc;
  JsonObject learned = doc["learned"].to<JsonObject>();
  JsonObject left = learned["L"].to<JsonObject>();
  left["VolUp"] = 0;
  left["VolDown"] = 0;
  left["SeekPlus"] = 0;
  left["SeekMinus"] = 0;
  left["Mode"] = 0;

  JsonObject right = learned["R"].to<JsonObject>();
  right["Main"] = 0;
  right["Cancel"] = 0;
  right["UpSet"] = 0;
  right["DownRes"] = 0;

  JsonObject timing = doc["timing"].to<JsonObject>();
  timing["mainHoldMs"] = 1000;
  timing["cancelMs"] = 200;
  timing["upSetMs"] = 200;
  timing["downResMs"] = 200;
  timing["brakeOutMs"] = 300;

  String json;
  serializeJson(doc, json);
  return json;
}

void beginConfigStore() {
  prefs.begin("cc-s3", false);
  configJson = prefs.getString("config", "");
  if (configJson.length() == 0) {
    resetConfig();
  }
}

bool parseConfig(StaticJsonDocument<1024>& doc) {
  DeserializationError err = deserializeJson(doc, configJson);
  if (!err) return true;

  resetConfig();
  return !deserializeJson(doc, configJson);
}

void saveConfigJson(const String& json) {
  configJson = json;
  prefs.putString("config", configJson);
}

void resetConfig() {
  saveConfigJson(defaultConfigJson());
}

bool isValidLearnTarget(const char* side, const char* name) {
  for (const auto& entry : LEARNED_ENTRIES) {
    if (strcmp(side, entry.side) == 0 && strcmp(name, entry.name) == 0) {
      return true;
    }
  }
  return false;
}

bool setLearnedValue(const char* side, const char* name, int adc) {
  if (!isValidLearnTarget(side, name)) return false;

  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return false;

  doc["learned"][side][name] = adc;
  String json;
  serializeJson(doc, json);
  saveConfigJson(json);
  return true;
}

String detectButton(int adc) {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return "NONE";

  int bestDiff = DETECT_THRESHOLD + 1;
  const char* bestName = "NONE";

  for (const auto& entry : LEARNED_ENTRIES) {
    int learned = doc["learned"][entry.side][entry.name] | 0;
    if (learned == 0) continue;

    int diff = abs(adc - learned);
    if (diff < bestDiff) {
      bestDiff = diff;
      bestName = entry.name;
    }
  }

  return bestDiff <= DETECT_THRESHOLD ? String(bestName) : String("NONE");
}
