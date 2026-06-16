#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <Wire.h>

#include "shutup_canopy_leds.h"
#include "shutup_config_portal.h"
#include "shutup_espnow.h"
#include "shutup_mcp23008.h"
#include "shutup_pins.h"
#include "shutup_settings.h"
#include "shutup_sound.h"

namespace {

constexpr const char *kDeviceName = "ShutUp Canopy";
constexpr const char *kConfigSsid = "SHUTUP-CANCONF";

constexpr int kConfigButtonGpio = 0;
constexpr int kMuteButtonGpio = 1;
constexpr int kLedStripDataGpio = 3;
constexpr int kMcp23008SdaGpio = 20;
constexpr int kMcp23008SclGpio = 21;
constexpr int kSpeakerSignalGpio = 10;
constexpr uint8_t kMcp23008Address = 0x20;

shutup::SettingsStore settings;
shutup::EspNowManager espNow;
shutup::ConfigPortal portal;
shutup::Mcp23008DoorReader doorReader;
shutup::CanopyStatusLeds statusLeds;
shutup::SoundPlayer soundPlayer;
bool configMode = false;
bool muted = false;
bool alarmActive = false;
bool lastCabActive = false;
uint8_t lastEnabledMask = 0;
uint8_t lastOpenMask = 0;
uint32_t lastDoorReadMs = 0;
uint8_t currentOpenMask = 0;

void setupHardwarePins() {
  pinMode(kConfigButtonGpio, INPUT_PULLUP);
  pinMode(kMuteButtonGpio, INPUT_PULLUP);
  pinMode(kSpeakerSignalGpio, OUTPUT);
  digitalWrite(kSpeakerSignalGpio, LOW);
  Wire.begin(kMcp23008SdaGpio, kMcp23008SclGpio);
  doorReader.begin(Wire, kMcp23008Address);
  statusLeds.begin(kLedStripDataGpio);
  statusLeds.show(0, 0, shutup::LinkLedState::Disconnected);
}

uint8_t readDoorOpenMask() {
  return doorReader.readOpenMask(settings.doorEnabledMask());
}

void startConfigMode() {
  configMode = true;
  Serial.println("Starting Canopy config mode");
  settings.begin(shutup::DeviceRole::Canopy, kDeviceName);
  soundPlayer.begin(kSpeakerSignalGpio, shutup::DeviceRole::Canopy, settings);
  espNow.begin(shutup::DeviceRole::Canopy, settings, true);
  portal.begin(shutup::DeviceRole::Canopy, settings, espNow, kConfigSsid, &soundPlayer);
}

void startNormalMode() {
  configMode = false;
  Serial.println("Starting Canopy ESP-NOW state-holder mode");
  settings.begin(shutup::DeviceRole::Canopy, kDeviceName);
  soundPlayer.begin(kSpeakerSignalGpio, shutup::DeviceRole::Canopy, settings);
  soundPlayer.trigger(shutup::SoundAction::Startup);
  espNow.begin(shutup::DeviceRole::Canopy, settings, false);
  if (!settings.hasPeer()) {
    Serial.println("No paired Cab device stored. Boot with config button held to pair.");
  }
}

shutup::LinkLedState canopyLinkState(uint32_t now) {
  if (espNow.cabLinkActive(now)) {
    return shutup::LinkLedState::Connected;
  }
  if (espNow.cabLinkStale(now)) {
    return shutup::LinkLedState::Weak;
  }
  return shutup::LinkLedState::Disconnected;
}

void updateCanopyOperation() {
  const uint32_t now = millis();
  const bool cabActive = espNow.cabLinkActive(now);
  const uint8_t enabledMask = settings.doorEnabledMask();

  if (now - lastDoorReadMs >= 50) {
    lastDoorReadMs = now;
    currentOpenMask = readDoorOpenMask();
  }

  if (digitalRead(kMuteButtonGpio) == LOW) {
    muted = true;
    soundPlayer.stop();
  }

  if (cabActive && !lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivitySuccess);
  } else if (!cabActive && lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivityError);
  }

  const bool anyDoorOpen = currentOpenMask != 0;
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

  if (enabledMask != lastEnabledMask || currentOpenMask != lastOpenMask || cabActive != lastCabActive ||
      now - lastDoorReadMs < 5) {
    statusLeds.show(enabledMask, currentOpenMask, canopyLinkState(now));
    lastEnabledMask = enabledMask;
    lastOpenMask = currentOpenMask;
    lastCabActive = cabActive;
  }
  espNow.setCanopyDoorState(enabledMask, currentOpenMask);
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
    statusLeds.show(settings.doorEnabledMask(), readDoorOpenMask(), shutup::LinkLedState::Disconnected);
  } else {
    updateCanopyOperation();
  }
  soundPlayer.loop();
  espNow.loop();
}
