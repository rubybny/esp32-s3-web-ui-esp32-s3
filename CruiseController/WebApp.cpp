#include "WebApp.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <functional>

#include "ConfigStore.h"
#include "Outputs.h"
#include "Pins.h"
#include "Status.h"

namespace {
constexpr const char* AP_SSID = "CC-S3";
constexpr const char* AP_PASS = "ccs3setup";
constexpr uint32_t WS_STATUS_INTERVAL_MS = 300;

IPAddress apIp(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
uint32_t lastWsStatusAt = 0;

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

void setupWifiAp() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("AP SSID: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupRoutes() {
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");

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
                         saveConfigJson(next);
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
                         if (!setOutputState(name, state)) {
                           sendError(req, 400, "invalid output name");
                           return;
                         }
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
                         if (!setLearnedValue(side, name, adc)) {
                           sendError(req, 400, "invalid learn target");
                           return;
                         }
                         sendJson(req, configJson);
                       });
      });

  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
    resetConfig();
    resetOutputs();
    sendJson(request, configJson);
  });

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
}

void setupWebSocket() {
  ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type, void*, uint8_t*, size_t) {
    if (type == WS_EVT_CONNECT) {
      client->text(buildStatusJson());
    }
  });
  server.addHandler(&ws);
}
}

void setupWebApp() {
  setupWifiAp();
  setupWebSocket();
  setupRoutes();
  server.begin();
}

void webAppLoop() {
  uint32_t now = millis();
  if (now - lastWsStatusAt >= WS_STATUS_INTERVAL_MS) {
    lastWsStatusAt = now;
    ws.textAll(buildStatusJson());
    ws.cleanupClients();
  }
}
