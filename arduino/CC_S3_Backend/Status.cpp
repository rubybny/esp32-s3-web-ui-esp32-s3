#include "Status.h"

#include <ArduinoJson.h>

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"

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
  pinMode(Pins::BRAKE_IN, INPUT_PULLUP);
}

String buildStatusJson() {
  int adc = analogRead(Pins::ADC_STEER);

  StaticJsonDocument<512> doc;
  doc["adc"] = adc;
  doc["button"] = detectButton(adc);
  doc["vin"] = round2(rawToVoltage(analogRead(Pins::ADC_VIN)));
  doc["vout"] = round2(rawToVoltage(analogRead(Pins::ADC_VOUT)));
  doc["brakeIn"] = digitalRead(Pins::BRAKE_IN) == LOW;

  JsonObject outputs = doc["outputs"].to<JsonObject>();
  outputs["main"] = getOutputState("main");
  outputs["upSet"] = getOutputState("upSet");
  outputs["downRes"] = getOutputState("downRes");
  outputs["cancel"] = getOutputState("cancel");
  outputs["brakeOut"] = getOutputState("brakeOut");

  String json;
  serializeJson(doc, json);
  return json;
}
