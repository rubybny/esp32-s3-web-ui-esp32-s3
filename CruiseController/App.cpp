#include "App.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "ConfigStore.h"
#include "Outputs.h"
#include "Status.h"
#include "WebApp.h"

void appSetup() {
  Serial.begin(115200);
  delay(200);

  setupStatusInputs();
  setupOutputPins();

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  beginConfigStore();
  setupWebApp();
}

void appLoop() {
  webAppLoop();
}
