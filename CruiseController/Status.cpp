#include "Status.h"

#include <ArduinoJson.h>

#include "ConfigStore.h"
#include "Outputs.h"

void setupStatusInputs() {
}

String buildStatusJson() {
  StaticJsonDocument<384> doc;
  doc["pulseMs"] = getPulseMs();

  JsonObject outputs = doc["outputs"].to<JsonObject>();
  outputs["main"] = getOutputState("main");
  outputs["res"] = getOutputState("res");
  outputs["set"] = getOutputState("set");
  outputs["brake"] = getOutputState("brake");

  String json;
  serializeJson(doc, json);
  return json;
}
