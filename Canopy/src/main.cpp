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
#include "shutup_sound_player.h"

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
constexpr uint32_t kMuteClearStableMs = 3000;

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
uint8_t mutedDoorMask = 0;
uint32_t allDoorsOkSinceMs = 0;
uint32_t lastDoorReadMs = 0;
shutup::DoorState currentDoorStates[shutup::kDoorCount]{};
shutup::DoorState lastDoorStates[shutup::kDoorCount]{};

void setupHardwarePins() {
  pinMode(kConfigButtonGpio, INPUT_PULLUP);
  pinMode(kMuteButtonGpio, INPUT_PULLUP);
  pinMode(kSpeakerSignalGpio, OUTPUT);
  digitalWrite(kSpeakerSignalGpio, LOW);
  Wire.begin(kMcp23008SdaGpio, kMcp23008SclGpio);
  doorReader.begin(Wire, kMcp23008Address);
  statusLeds.begin(kLedStripDataGpio);
  statusLeds.show(currentDoorStates, shutup::LinkLedState::Disconnected);
}

void readDoorStates(shutup::DoorState states[shutup::kDoorCount]) {
  doorReader.readDoorStates(settings.doorEnabledMask(), states);
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

uint8_t openDoorMask(const shutup::DoorState doorStates[shutup::kDoorCount]) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < shutup::kDoorCount; ++i) {
    if (doorStates[i] == shutup::DoorState::Open) {
      mask |= static_cast<uint8_t>(1U << i);
    }
  }
  return mask;
}

void updateDoorAlarm(uint8_t openMask, uint32_t now) {
  if (digitalRead(kMuteButtonGpio) == LOW) {
    soundPlayer.stop();
    alarmActive = false;
    if (openMask != 0) {
      muted = true;
      mutedDoorMask = openMask;
      allDoorsOkSinceMs = 0;
    }
  }

  if (openMask == 0) {
    if (alarmActive) {
      alarmActive = false;
      soundPlayer.stop();
      soundPlayer.trigger(shutup::SoundAction::DoorsOk);
    }
    if (muted) {
      if (allDoorsOkSinceMs == 0) {
        allDoorsOkSinceMs = now;
      } else if (now - allDoorsOkSinceMs >= kMuteClearStableMs) {
        muted = false;
        mutedDoorMask = 0;
      }
    }
    return;
  }

  allDoorsOkSinceMs = 0;
  const uint8_t unmutedOpenMask = muted ? static_cast<uint8_t>(openMask & ~mutedDoorMask) : openMask;
  if (unmutedOpenMask == 0) {
    if (alarmActive) {
      alarmActive = false;
      soundPlayer.stop();
    }
    return;
  }
  if (!alarmActive) {
    alarmActive = true;
    soundPlayer.trigger(shutup::SoundAction::DoorAlarm);
  }
}

void updateCanopyOperation() {
  const uint32_t now = millis();
  const bool cabActive = espNow.cabLinkActive(now);
  bool doorStatesChanged = false;

  if (now - lastDoorReadMs >= 50) {
    lastDoorReadMs = now;
    readDoorStates(currentDoorStates);
    for (uint8_t i = 0; i < shutup::kDoorCount; ++i) {
      doorStatesChanged = doorStatesChanged || currentDoorStates[i] != lastDoorStates[i];
    }
  }

  if (cabActive && !lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivitySuccess);
  } else if (!cabActive && lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivityError);
  }

  updateDoorAlarm(openDoorMask(currentDoorStates), now);

  if (doorStatesChanged || cabActive != lastCabActive || now - lastDoorReadMs < 5) {
    statusLeds.show(currentDoorStates, canopyLinkState(now));
    memcpy(lastDoorStates, currentDoorStates, sizeof(lastDoorStates));
    lastCabActive = cabActive;
  }
  espNow.setCanopyDoorStates(currentDoorStates);
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
    readDoorStates(currentDoorStates);
    statusLeds.show(currentDoorStates, shutup::LinkLedState::Disconnected);
  } else {
    updateCanopyOperation();
  }
  soundPlayer.loop();
  espNow.loop();
}
