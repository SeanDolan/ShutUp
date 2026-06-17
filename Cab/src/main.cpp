#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>

#include "shutup_cab_display.h"
#include "shutup_config_portal.h"
#include "shutup_espnow.h"
#include "shutup_pins.h"
#include "shutup_settings.h"
#include "shutup_sound.h"

namespace {

constexpr const char *kDeviceName = "ShutUp Cab";
constexpr const char *kConfigSsid = "SHUTUP-CABCONF";

constexpr int kConfigButtonGpio = 2;
constexpr int kMuteInputGpio = 1;
constexpr int kSpeakerSignalGpio = 13;

shutup::SettingsStore settings;
shutup::EspNowManager espNow;
shutup::ConfigPortal portal;
shutup::CabDisplay display;
shutup::SoundPlayer soundPlayer;
bool configMode = false;
bool muted = false;
bool alarmActive = false;
bool lastLinkFresh = false;
shutup::DoorState lastDoorStates[shutup::kDoorCount]{};
uint32_t lastDisplayMs = 0;

void setupHardwarePins() {
  pinMode(kConfigButtonGpio, INPUT_PULLUP);
  pinMode(kMuteInputGpio, INPUT_PULLUP);
  pinMode(kSpeakerSignalGpio, OUTPUT);
  digitalWrite(kSpeakerSignalGpio, LOW);
}

void startConfigMode() {
  configMode = true;
  Serial.println("Starting Cab config mode");
  settings.begin(shutup::DeviceRole::Cab, kDeviceName);
  soundPlayer.begin(kSpeakerSignalGpio, shutup::DeviceRole::Cab, settings);
  display.showConfigMode();
  espNow.begin(shutup::DeviceRole::Cab, settings, true);
  portal.begin(shutup::DeviceRole::Cab, settings, espNow, kConfigSsid, &soundPlayer);
}

void startNormalMode() {
  configMode = false;
  Serial.println("Starting Cab ESP-NOW client mode");
  settings.begin(shutup::DeviceRole::Cab, kDeviceName);
  soundPlayer.begin(kSpeakerSignalGpio, shutup::DeviceRole::Cab, settings);
  display.showSplash();
  soundPlayer.trigger(shutup::SoundAction::Startup);
  espNow.begin(shutup::DeviceRole::Cab, settings, false);
  if (!settings.hasPeer()) {
    Serial.println("No paired Canopy device stored. Boot with config button held to pair.");
  }
}

void updateNormalOperation() {
  const uint32_t now = millis();
  const bool linkFresh = espNow.cabStateFresh(now);
  shutup::DoorState doorStates[shutup::kDoorCount]{};
  bool anyDoorOpen = false;
  bool doorStatesChanged = false;
  for (uint8_t i = 0; i < shutup::kDoorCount; ++i) {
    doorStates[i] = espNow.lastDoorState(i);
    anyDoorOpen = anyDoorOpen || doorStates[i] == shutup::DoorState::Open;
    doorStatesChanged = doorStatesChanged || doorStates[i] != lastDoorStates[i];
  }

  if (digitalRead(kMuteInputGpio) == LOW) {
    muted = true;
    soundPlayer.stop();
  }

  if (linkFresh && !lastLinkFresh) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivitySuccess);
  } else if (!linkFresh && lastLinkFresh) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivityError);
  }

  if (anyDoorOpen && !muted && !alarmActive) {
    alarmActive = true;
    soundPlayer.trigger(shutup::SoundAction::DoorAlarm);
  }
  if (!anyDoorOpen && alarmActive) {
    alarmActive = false;
    muted = false;
    soundPlayer.stop();
    soundPlayer.trigger(shutup::SoundAction::DoorsOk);
  }

  if (now - lastDisplayMs >= 500 || doorStatesChanged || linkFresh != lastLinkFresh) {
    display.showNormal(doorStates, settings, linkFresh, espNow.heartbeatSuccessPercent(), espNow.averageHeartbeatMs());
    lastDisplayMs = now;
    memcpy(lastDoorStates, doorStates, sizeof(lastDoorStates));
    lastLinkFresh = linkFresh;
  }
}

}  // namespace

void setup() {
  display.beginBlackout();
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
  } else {
    updateNormalOperation();
  }
  soundPlayer.loop();
  espNow.loop();
}
