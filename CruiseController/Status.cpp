#include "Status.h"

#include <ArduinoJson.h>

#include "ConfigStore.h"
#include "CruiseInput.h"
#include "Outputs.h"

void setupStatusInputs() {
}

String buildStatusJson() {
  StaticJsonDocument<384> doc;
  doc["adc"] = getCruiseAdc();
  doc["button"] = getCruiseButton();
  doc["pulseMs"] = getPulseMs();
  JsonObject timing = doc["timing"].to<JsonObject>();
  timing["main"] = getPulseMsForOutput("main");
  timing["res"] = getPulseMsForOutput("res");
  timing["set"] = getPulseMsForOutput("set");
  timing["cancel"] = getPulseMsForOutput("cancel");

  JsonObject outputs = doc["outputs"].to<JsonObject>();
  outputs["main"] = getOutputState("main");
  outputs["res"] = getOutputState("res");
  outputs["set"] = getOutputState("set");
  outputs["brake"] = getOutputState("brake");

  String json;
  serializeJson(doc, json);
  return json;
}
