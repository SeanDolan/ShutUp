#pragma once

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "shutup_espnow.h"
#include "web_assets.h"

namespace shutup {

class ConfigPortal {
public:
  bool begin(DeviceRole role, SettingsStore &settings, EspNowManager &espNow, const char *ssid) {
    role_ = role;
    settings_ = &settings;
    espNow_ = &espNow;
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    if (!WiFi.softAP(ssid, nullptr, kEspNowChannel)) {
      Serial.println("Config AP start failed");
      return false;
    }
    dns_.start(53, "*", WiFi.softAPIP());
    registerRoutes();
    server_.begin();
    Serial.print("Config AP SSID: ");
    Serial.println(ssid);
    Serial.print("Config AP IP: ");
    Serial.println(WiFi.softAPIP());
    return true;
  }

  void loop() {
    dns_.processNextRequest();
    server_.handleClient();
  }

private:
  void registerRoutes() {
    server_.on("/", HTTP_GET, [this]() { serveIndex(); });
    server_.on("/config", HTTP_GET, [this]() { serveIndex(); });
    server_.on("/generate_204", HTTP_GET, [this]() { redirectHome(); });
    server_.on("/gen_204", HTTP_GET, [this]() { redirectHome(); });
    server_.on("/fwlink", HTTP_GET, [this]() { redirectHome(); });
    server_.on("/hotspot-detect.html", HTTP_GET, [this]() { redirectHome(); });
    server_.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    server_.on("/api/config", HTTP_POST, [this]() { handleSaveConfig(); });
    server_.on("/api/pair/start", HTTP_POST, [this]() { handlePairStart(); });
    server_.on("/api/pair/status", HTTP_GET, [this]() { handlePairStatus(); });
    server_.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
    server_.onNotFound([this]() { redirectHome(); });
  }

  void serveIndex() {
    const char *html = role_ == DeviceRole::Cab ? kConfigCabHtml : kConfigCanHtml;
    server_.send(200, "text/html", html);
  }

  void redirectHome() {
    server_.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server_.send(302, "text/plain", "");
  }

  void handleGetConfig() {
    server_.send(200, "application/json", configJson());
  }

  void handleSaveConfig() {
    if (!settings_) {
      server_.send(500, "application/json", "{\"ok\":false}");
      return;
    }
    if (server_.hasArg("deviceName")) {
      settings_->setDeviceName(server_.arg("deviceName"));
    }
    if (role_ == DeviceRole::Canopy) {
      for (uint8_t i = 0; i < 6; ++i) {
        const String key = String("door") + String(i + 1);
        settings_->setDoorEnabled(i, server_.hasArg(key));
      }
    }
    server_.send(200, "application/json", configJson());
  }

  void handlePairStart() {
    if (espNow_) {
      espNow_->startPairing();
    }
    server_.send(200, "application/json", pairStatusJson());
  }

  void handlePairStatus() {
    server_.send(200, "application/json", pairStatusJson());
  }

  void handleReboot() {
    server_.send(200, "application/json", "{\"ok\":true,\"message\":\"Rebooting\"}");
    delay(250);
    ESP.restart();
  }

  String configJson() const {
    String json = "{";
    json += "\"role\":\"" + String(roleName(role_)) + "\",";
    json += "\"deviceName\":\"" + escapeJson(settings_->deviceName()) + "\",";
    json += "\"localMac\":\"" + (espNow_ ? espNow_->localMacString() : WiFi.macAddress()) + "\",";
    json += "\"hasPeer\":" + String(settings_->hasPeer() ? "true" : "false") + ",";
    json += "\"peerMac\":\"" + settings_->peerMacString() + "\",";
    json += "\"doorEnabledMask\":" + String(settings_->doorEnabledMask());
    json += "}";
    return json;
  }

  String pairStatusJson() const {
    String json = "{";
    json += "\"active\":" + String(espNow_ && espNow_->pairingActive() ? "true" : "false") + ",";
    json += "\"hasPeer\":" + String(settings_ && settings_->hasPeer() ? "true" : "false") + ",";
    json += "\"peerMac\":\"" + (settings_ ? settings_->peerMacString() : String("")) + "\",";
    json += "\"message\":\"" + escapeJson(espNow_ ? espNow_->lastPairMessage() : String("ESP-NOW unavailable")) + "\"";
    json += "}";
    return json;
  }

  static String escapeJson(const String &text) {
    String out;
    out.reserve(text.length() + 8);
    for (size_t i = 0; i < text.length(); ++i) {
      const char c = text[i];
      if (c == '"' || c == '\\') {
        out += '\\';
      }
      if (c == '\n' || c == '\r') {
        out += ' ';
      } else {
        out += c;
      }
    }
    return out;
  }

  DNSServer dns_;
  WebServer server_{80};
  DeviceRole role_{DeviceRole::Cab};
  SettingsStore *settings_{nullptr};
  EspNowManager *espNow_{nullptr};
};

}  // namespace shutup
