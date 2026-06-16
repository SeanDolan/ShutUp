#pragma once

#include <Arduino.h>

#include "shutup_settings.h"

namespace shutup {

class SoundPlayer {
public:
  void begin(int speakerPin, DeviceRole role, SettingsStore &settings) {
    speakerPin_ = speakerPin;
    role_ = role;
    settings_ = &settings;
    pinMode(speakerPin_, OUTPUT);
    digitalWrite(speakerPin_, LOW);
  }

  void trigger(SoundAction action) {
    if (!settings_) {
      return;
    }
    const SoundActionConfig &config = settings_->soundAction(static_cast<uint8_t>(action));
    const String &sound = role_ == DeviceRole::Cab ? config.cabSound : config.canopySound;
    if (sound == kNoSoundName) {
      stop();
      return;
    }
    activeSound_ = sound;
    repeatMs_ = config.repeatMs;
    delayMs_ = config.delayMs;
    nextPlayMs_ = millis() + delayMs_;
    active_ = true;
  }

  void playNow(const String &sound) {
    stop();
    if (sound == kNoSoundName) {
      return;
    }
    playConfiguredSound(sound);
  }

  void stop() {
    active_ = false;
    activeSound_ = kNoSoundName;
    noTone(speakerPin_);
    digitalWrite(speakerPin_, LOW);
  }

  void loop() {
    if (!active_) {
      return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextPlayMs_) < 0) {
      return;
    }
    playConfiguredSound(activeSound_);
    if (repeatMs_ == 0) {
      active_ = false;
      return;
    }
    nextPlayMs_ = now + repeatMs_;
  }

private:
  void playConfiguredSound(const String &sound) {
    if (sound == kNoSoundName) {
      return;
    }
    // File-backed sounds are intentionally not decoded yet. The name is kept so
    // the config contract is ready once the sound file format is agreed.
    tone(speakerPin_, 1200, 180);
  }

  int speakerPin_{-1};
  DeviceRole role_{DeviceRole::Cab};
  SettingsStore *settings_{nullptr};
  bool active_{false};
  String activeSound_{kNoSoundName};
  uint32_t repeatMs_{0};
  uint32_t delayMs_{0};
  uint32_t nextPlayMs_{0};
};

}  // namespace shutup
