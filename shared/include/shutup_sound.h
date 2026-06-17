#pragma once

#include <Arduino.h>

#include "shutup_settings.h"
#include "shutup_tones.h"

namespace shutup {

class SoundPlayer {
public:
  void begin(int speakerPin, DeviceRole role, SettingsStore &settings) {
    speakerPin_ = speakerPin;
    role_ = role;
    settings_ = &settings;
    pinMode(speakerPin_, OUTPUT);
    stopTone();
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
    repeat_ = config.repeat;
    repeatDelayMs_ = config.delayMs;
    nextPlayMs_ = 0;
    active_ = true;
    if (!startSound(sound)) {
      active_ = false;
    }
  }

  void playNow(const String &sound) {
    stop();
    if (sound == kNoSoundName) {
      return;
    }
    startSound(sound);
  }

  void stop() {
    active_ = false;
    activeSound_ = kNoSoundName;
    stopTone();
    currentSound_ = nullptr;
    stepIndex_ = 0;
    stepEndMs_ = 0;
  }

  void loop() {
    updateSequence();
    if (!active_) {
      return;
    }
    if (currentSound_) {
      return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextPlayMs_) < 0) {
      return;
    }
    if (!startSound(activeSound_)) {
      active_ = false;
      return;
    }
    if (!repeat_) {
      nextPlayMs_ = 0;
      return;
    }
    nextPlayMs_ = 0;
  }

private:
  bool startSound(const String &sound) {
    if (sound == kNoSoundName) {
      return false;
    }
    const ToneSound *toneSound = findToneSoundByName(sound);
    if (!toneSound || toneSound->stepCount == 0) {
      stopTone();
      return false;
    }
    currentSound_ = toneSound;
    stepIndex_ = 0;
    playCurrentStep();
    return true;
  }

  void updateSequence() {
    if (!currentSound_) {
      return;
    }
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - stepEndMs_) < 0) {
      return;
    }
    ++stepIndex_;
    if (stepIndex_ < currentSound_->stepCount) {
      playCurrentStep();
      return;
    }
    stopTone();
    currentSound_ = nullptr;
    stepIndex_ = 0;
    stepEndMs_ = 0;
    if (active_ && repeat_) {
      nextPlayMs_ = now + repeatDelayMs_;
    } else {
      active_ = false;
    }
  }

  void playCurrentStep() {
    if (!currentSound_ || stepIndex_ >= currentSound_->stepCount) {
      stopTone();
      return;
    }
    const ToneStep &step = currentSound_->steps[stepIndex_];
    if (step.frequencyHz == 0) {
      stopTone();
    } else {
      tone(speakerPin_, step.frequencyHz);
    }
    stepEndMs_ = millis() + step.durationMs;
  }

  void stopTone() {
    if (speakerPin_ < 0) {
      return;
    }
    noTone(speakerPin_);
    digitalWrite(speakerPin_, LOW);
  }

  int speakerPin_{-1};
  DeviceRole role_{DeviceRole::Cab};
  SettingsStore *settings_{nullptr};
  bool active_{false};
  String activeSound_{kNoSoundName};
  bool repeat_{false};
  uint32_t repeatDelayMs_{0};
  uint32_t nextPlayMs_{0};
  const ToneSound *currentSound_{nullptr};
  uint8_t stepIndex_{0};
  uint32_t stepEndMs_{0};
};

}  // namespace shutup
