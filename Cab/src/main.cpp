#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>

#include "shutup_config_portal.h"
#include "shutup_espnow.h"
#include "shutup_pins.h"
#include "shutup_settings.h"

namespace {

constexpr const char *kDeviceName = "ShutUp Cab";
constexpr const char *kConfigSsid = "SHUTUP-CABCONF";

// Physical config-button GPIO has not been selected yet.
// Set this to the approved Cab config-button GPIO once the wiring is decided.
constexpr int kConfigButtonGpio = shutup::kUnassignedPin;

shutup::SettingsStore settings;
shutup::EspNowManager espNow;
shutup::ConfigPortal portal;
bool configMode = false;

void startConfigMode() {
  configMode = true;
  Serial.println("Starting Cab config mode");
  settings.begin(shutup::DeviceRole::Cab, kDeviceName);
  espNow.begin(shutup::DeviceRole::Cab, settings, true);
  portal.begin(shutup::DeviceRole::Cab, settings, espNow, kConfigSsid);
}

void startNormalMode() {
  configMode = false;
  Serial.println("Starting Cab ESP-NOW client mode");
  settings.begin(shutup::DeviceRole::Cab, kDeviceName);
  espNow.begin(shutup::DeviceRole::Cab, settings, false);
  if (!settings.hasPeer()) {
    Serial.println("No paired Canopy device stored. Boot with config button held to pair.");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  if (kConfigButtonGpio < 0) {
    Serial.println("Cab config-button GPIO is not assigned; config mode cannot be entered by button yet.");
  }

  if (shutup::bootButtonHeld(kConfigButtonGpio)) {
    startConfigMode();
  } else {
    startNormalMode();
  }
}

void loop() {
  if (configMode) {
    portal.loop();
  }
  espNow.loop();
}
