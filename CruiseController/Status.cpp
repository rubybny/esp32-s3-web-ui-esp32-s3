#include "Status.h"

#include <ArduinoJson.h>

#include "ConfigStore.h"
#include "NaviOutput.h"
#include "Outputs.h"
#include "Pins.h"
#include "SteeringControl.h"

namespace {
float rawToVoltage(int raw) {
  return (static_cast<float>(raw) / 4095.0f) * 3.3f;
}

float round2(float value) {
  return roundf(value * 100.0f) / 100.0f;
}
}

void setupStatusInputs() {
  analogReadResolution(12);
  pinMode(Pins::ADC_STEER, INPUT);
  pinMode(Pins::ADC_VIN, INPUT);
  pinMode(Pins::ADC_VOUT, INPUT);
  pinMode(Pins::BRAKE_IN, Pins::BRAKE_IN_MODE);
}

String buildStatusJson() {
  int adc = analogRead(Pins::ADC_STEER);

  StaticJsonDocument<512> doc;
  doc["adc"] = adc;
  doc["button"] = getDebouncedSteeringButton();
  doc["vin"] = round2(rawToVoltage(analogRead(Pins::ADC_VIN)));
  doc["vout"] = round2(rawToVoltage(analogRead(Pins::ADC_VOUT)));
  doc["brakeIn"] = digitalRead(Pins::BRAKE_IN) == Pins::BRAKE_IN_ACTIVE_LEVEL;

  JsonObject outputs = doc["outputs"].to<JsonObject>();
  outputs["main"] = getOutputState("main");
  outputs["upSet"] = getOutputState("upSet");
  outputs["downRes"] = getOutputState("downRes");
  outputs["cancel"] = getOutputState("cancel");
  outputs["brakeOut"] = getOutputState("brakeOut");
  JsonObject navi = outputs["navi"].to<JsonObject>();
  navi["volUp"] = getNaviOutputState("VolUp");
  navi["volDown"] = getNaviOutputState("VolDown");
  navi["seekPlus"] = getNaviOutputState("SeekPlus");
  navi["seekMinus"] = getNaviOutputState("SeekMinus");
  navi["mode"] = getNaviOutputState("Mode");

  String json;
  serializeJson(doc, json);
  return json;
}
