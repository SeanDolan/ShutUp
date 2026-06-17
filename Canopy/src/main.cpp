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

  if (digitalRead(kMuteButtonGpio) == LOW) {
    muted = true;
    soundPlayer.stop();
  }

  if (cabActive && !lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivitySuccess);
  } else if (!cabActive && lastCabActive) {
    soundPlayer.trigger(shutup::SoundAction::ConnectivityError);
  }

  bool anyDoorOpen = false;
  for (uint8_t i = 0; i < shutup::kDoorCount; ++i) {
    anyDoorOpen = anyDoorOpen || currentDoorStates[i] == shutup::DoorState::Open;
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
