#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <functional>

namespace Pins {
constexpr uint8_t ADC_STEER = 4;
constexpr uint8_t ADC_VIN = 11;
constexpr uint8_t ADC_VOUT = 12;
constexpr uint8_t BRAKE_IN = 10;
constexpr uint8_t OUT_MAIN = 5;
constexpr uint8_t OUT_UPSET = 6;
constexpr uint8_t OUT_DOWNRES = 7;
constexpr uint8_t OUT_CANCEL = 8;
constexpr uint8_t OUT_BRAKE = 9;
}  // namespace Pins

constexpr const char* AP_SSID = "CC-S3";
constexpr const char* AP_PASS = "ccs3setup";
constexpr int DETECT_THRESHOLD = 100;
constexpr uint32_t WS_STATUS_INTERVAL_MS = 500;

IPAddress apIp(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences prefs;

bool outMain = false;
bool outUpSet = false;
bool outDownRes = false;
bool outCancel = false;
bool outBrake = false;
float adcLiving = 0.0f;
uint32_t lastWsStatusAt = 0;

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

void addCorsHeaders(AsyncWebServerResponse* response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  response->addHeader("Access-Control-Allow-Private-Network", "true");
}

void sendJson(AsyncWebServerRequest* request, const String& json, int code = 200) {
  AsyncWebServerResponse* response = request->beginResponse(code, "application/json", json);
  addCorsHeaders(response);
  request->send(response);
}

void sendError(AsyncWebServerRequest* request, int code, const char* message) {
  StaticJsonDocument<160> doc;
  doc["ok"] = false;
  doc["error"] = message;
  String json;
  serializeJson(doc, json);
  sendJson(request, json, code);
}

bool parseConfig(StaticJsonDocument<1024>& doc) {
  DeserializationError err = deserializeJson(doc, configJson);
  if (err) {
    configJson = defaultConfigJson();
    prefs.putString("config", configJson);
    return !deserializeJson(doc, configJson);
  }
  return true;
}

int getLearnedValue(const char* side, const char* name) {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return 0;
  return doc["learned"][side][name] | 0;
}

void setLearnedValue(const char* side, const char* name, int adc) {
  StaticJsonDocument<1024> doc;
  if (!parseConfig(doc)) return;
  doc["learned"][side][name] = adc;
  configJson = "";
  serializeJson(doc, configJson);
  prefs.putString("config", configJson);
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

float rawToVoltage(int raw) {
  return (static_cast<float>(raw) / 4095.0f) * 3.3f;
}

float round2(float value) {
  return roundf(value * 100.0f) / 100.0f;
}

void setOutputPin(const String& name, bool state) {
  if (name == "main") {
    outMain = state;
    digitalWrite(Pins::OUT_MAIN, state);
  } else if (name == "upSet") {
    outUpSet = state;
    digitalWrite(Pins::OUT_UPSET, state);
  } else if (name == "downRes") {
    outDownRes = state;
    digitalWrite(Pins::OUT_DOWNRES, state);
  } else if (name == "cancel") {
    outCancel = state;
    digitalWrite(Pins::OUT_CANCEL, state);
  } else if (name == "brakeOut") {
    outBrake = state;
    digitalWrite(Pins::OUT_BRAKE, state);
  }
}

bool isValidOutputName(const String& name) {
  return name == "main" || name == "upSet" || name == "downRes" || name == "cancel" || name == "brakeOut";
}

String buildStatusJson() {
  int adc = analogRead(Pins::ADC_STEER);
  if (adcLiving <= 0.1f) {
    adcLiving = adc;
  } else {
    adcLiving = adcLiving * 0.85f + static_cast<float>(adc) * 0.15f;
  }

  StaticJsonDocument<768> doc;
  doc["adc"] = adc;
  doc["adcLiving"] = static_cast<int>(roundf(adcLiving));
  doc["vin"] = round2(rawToVoltage(analogRead(Pins::ADC_VIN)));
  doc["vout"] = round2(rawToVoltage(analogRead(Pins::ADC_VOUT)));
  doc["brakeIn"] = digitalRead(Pins::BRAKE_IN) == LOW;

  JsonObject currentMa = doc["currentMa"].to<JsonObject>();
  currentMa["brakeIn"] = 0;
  currentMa["main"] = 0;
  currentMa["upSet"] = 0;
  currentMa["downRes"] = 0;
  currentMa["cancel"] = 0;
  currentMa["brakeOut"] = 0;

  JsonObject outputs = doc["outputs"].to<JsonObject>();
  outputs["main"] = outMain;
  outputs["upSet"] = outUpSet;
  outputs["downRes"] = outDownRes;
  outputs["cancel"] = outCancel;
  outputs["brakeOut"] = outBrake;
  doc["button"] = detectButton(adc);

  String json;
  serializeJson(doc, json);
  return json;
}

void handleJsonBody(
    AsyncWebServerRequest* request,
    uint8_t* data,
    size_t len,
    size_t index,
    size_t total,
    std::function<void(AsyncWebServerRequest*, StaticJsonDocument<1024>&)> handler) {
  if (index != 0) return;
  if (len != total) {
    sendError(request, 413, "chunked bodies are not supported");
    return;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err) {
    sendError(request, 400, "invalid json");
    return;
  }
  handler(request, doc);
}

void setupRoutes() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");

  server.onNotFound([](AsyncWebServerRequest* request) {
    if (request->method() == HTTP_OPTIONS) {
      AsyncWebServerResponse* response = request->beginResponse(204);
      addCorsHeaders(response);
      request->send(response);
      return;
    }

    if (LittleFS.exists("/index.html")) {
      AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/index.html", "text/html");
      addCorsHeaders(response);
      request->send(response);
      return;
    }

    request->send(404, "text/plain", "not found");
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendJson(request, buildStatusJson());
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    sendJson(request, configJson);
  });

  server.on(
      "/api/config",
      HTTP_POST,
      [](AsyncWebServerRequest*) {},
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        handleJsonBody(request, data, len, index, total,
                       [](AsyncWebServerRequest* req, StaticJsonDocument<1024>& doc) {
                         String next;
                         serializeJson(doc, next);
                         configJson = next;
                         prefs.putString("config", configJson);
                         sendJson(req, "{\"ok\":true}");
                       });
      });

  server.on(
      "/api/output",
      HTTP_POST,
      [](AsyncWebServerRequest*) {},
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        handleJsonBody(request, data, len, index, total,
                       [](AsyncWebServerRequest* req, StaticJsonDocument<1024>& doc) {
                         String name = doc["name"] | "";
                         bool state = doc["state"] | false;
                         if (!isValidOutputName(name)) {
                           sendError(req, 400, "invalid output name");
                           return;
                         }
                         setOutputPin(name, state);
                         sendJson(req, buildStatusJson());
                       });
      });

  server.on(
      "/api/learn",
      HTTP_POST,
      [](AsyncWebServerRequest*) {},
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        handleJsonBody(request, data, len, index, total,
                       [](AsyncWebServerRequest* req, StaticJsonDocument<1024>& doc) {
                         const char* side = doc["side"] | "";
                         const char* name = doc["name"] | "";
                         int adc = doc["adc"] | analogRead(Pins::ADC_STEER);
                         bool valid = false;
                         for (const auto& entry : LEARNED_ENTRIES) {
                           if (strcmp(side, entry.side) == 0 && strcmp(name, entry.name) == 0) {
                             valid = true;
                             break;
                           }
                         }
                         if (!valid) {
                           sendError(req, 400, "invalid learn target");
                           return;
                         }
                         setLearnedValue(side, name, adc);
                         sendJson(req, configJson);
                       });
      });

  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
    configJson = defaultConfigJson();
    prefs.putString("config", configJson);
    outMain = outUpSet = outDownRes = outCancel = outBrake = false;
    digitalWrite(Pins::OUT_MAIN, LOW);
    digitalWrite(Pins::OUT_UPSET, LOW);
    digitalWrite(Pins::OUT_DOWNRES, LOW);
    digitalWrite(Pins::OUT_CANCEL, LOW);
    digitalWrite(Pins::OUT_BRAKE, LOW);
    sendJson(request, configJson);
  });

  ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type, void*, uint8_t*, size_t) {
    if (type == WS_EVT_CONNECT) {
      Serial.println("WebSocket client connected");
    }
  });
  server.addHandler(&ws);
}

void setupPins() {
  analogReadResolution(12);
  pinMode(Pins::ADC_STEER, INPUT);
  pinMode(Pins::ADC_VIN, INPUT);
  pinMode(Pins::ADC_VOUT, INPUT);
  pinMode(Pins::BRAKE_IN, INPUT_PULLUP);

  pinMode(Pins::OUT_MAIN, OUTPUT);
  pinMode(Pins::OUT_UPSET, OUTPUT);
  pinMode(Pins::OUT_DOWNRES, OUTPUT);
  pinMode(Pins::OUT_CANCEL, OUTPUT);
  pinMode(Pins::OUT_BRAKE, OUTPUT);
  setOutputPin("main", false);
  setOutputPin("upSet", false);
  setOutputPin("downRes", false);
  setOutputPin("cancel", false);
  setOutputPin("brakeOut", false);
}

void setupWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);
  delay(200);
  setupPins();

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  prefs.begin("cc-s3", false);
  configJson = prefs.getString("config", "");
  if (configJson.length() == 0) {
    configJson = defaultConfigJson();
    prefs.putString("config", configJson);
  }

  setupWiFi();
  setupRoutes();
  server.begin();
}

void loop() {
  uint32_t now = millis();
  if (now - lastWsStatusAt >= WS_STATUS_INTERVAL_MS) {
    lastWsStatusAt = now;
    ws.textAll(buildStatusJson());
    ws.cleanupClients();
  }
}
