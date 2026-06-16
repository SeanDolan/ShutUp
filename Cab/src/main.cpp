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

constexpr int kConfigButtonGpio = 2;
constexpr int kMuteInputGpio = 1;
constexpr int kSpeakerSignalGpio = 13;

shutup::SettingsStore settings;
shutup::EspNowManager espNow;
shutup::ConfigPortal portal;
bool configMode = false;

void setupHardwarePins() {
  pinMode(kConfigButtonGpio, INPUT_PULLUP);
  pinMode(kSpeakerSignalGpio, OUTPUT);
  digitalWrite(kSpeakerSignalGpio, LOW);
  // GPIO1 is reserved for the mute input. It supports capacitive touch if the final button uses a touch pad.
}

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
  setupHardwarePins();

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
