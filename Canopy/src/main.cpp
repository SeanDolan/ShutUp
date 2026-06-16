#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>

#include "shutup_config_portal.h"
#include "shutup_espnow.h"
#include "shutup_pins.h"
#include "shutup_settings.h"

namespace {

constexpr const char *kDeviceName = "ShutUp Canopy";
constexpr const char *kConfigSsid = "SHUTUP-CANCONF";

// Physical config-button GPIO has not been selected yet.
// Set this to the approved Canopy config-button GPIO once the wiring is decided.
constexpr int kConfigButtonGpio = shutup::kUnassignedPin;

shutup::SettingsStore settings;
shutup::EspNowManager espNow;
shutup::ConfigPortal portal;
bool configMode = false;

uint8_t readDoorOpenMask() {
  // MCP23008 wiring and address are not assigned yet.
  // Until that hardware decision is made, report no open doors.
  return 0;
}

void startConfigMode() {
  configMode = true;
  Serial.println("Starting Canopy config mode");
  settings.begin(shutup::DeviceRole::Canopy, kDeviceName);
  espNow.begin(shutup::DeviceRole::Canopy, settings, true);
  portal.begin(shutup::DeviceRole::Canopy, settings, espNow, kConfigSsid);
}

void startNormalMode() {
  configMode = false;
  Serial.println("Starting Canopy ESP-NOW state-holder mode");
  settings.begin(shutup::DeviceRole::Canopy, kDeviceName);
  espNow.begin(shutup::DeviceRole::Canopy, settings, false);
  if (!settings.hasPeer()) {
    Serial.println("No paired Cab device stored. Boot with config button held to pair.");
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  if (kConfigButtonGpio < 0) {
    Serial.println("Canopy config-button GPIO is not assigned; config mode cannot be entered by button yet.");
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
  espNow.setCanopyDoorState(settings.doorEnabledMask(), readDoorOpenMask());
  espNow.loop();
}
