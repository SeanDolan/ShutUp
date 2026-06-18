#pragma once

#include <Arduino.h>

namespace shutup {

struct ToneStep {
  uint16_t frequencyHz;
  uint16_t durationMs;
};

struct ToneSound {
  uint8_t id;
  const char *name;
  bool loopFriendly;
  const ToneStep *steps;
  uint8_t stepCount;
};

static constexpr ToneStep kSoundNoneSteps[] = {
    {0, 1},
};

static constexpr ToneStep kSoundPowerUpSteps[] = {
    {800, 80},
    {1000, 80},
    {1300, 120},
};

static constexpr ToneStep kSoundPowerDownSteps[] = {
    {1200, 100},
    {900, 100},
    {600, 140},
};

static constexpr ToneStep kSoundPewPewSteps[] = {
    {1800, 60},
    {0, 40},
    {2000, 60},
};

static constexpr ToneStep kSoundPulseSteps[] = {
    {1200, 120},
};

static constexpr ToneStep kSoundConfirmSteps[] = {
    {1100, 70},
    {0, 40},
    {1400, 90},
};

static constexpr ToneStep kSoundErrorSteps[] = {
    {400, 160},
    {0, 60},
    {1000, 60},
};

static constexpr ToneStep kSoundSaveOkSteps[] = {
    {900, 60},
    {0, 40},
    {1200, 80},
};

static constexpr ToneStep kSoundRestartingSteps[] = {
    {1000, 120},
    {800, 120},
    {600, 180},
};

static constexpr ToneStep kSoundConnectWifiSteps[] = {
    {1400, 60},
    {1600, 60},
    {1800, 80},
};

static constexpr ToneStep kSoundLostWifiSteps[] = {
    {900, 120},
    {600, 160},
};

static constexpr ToneStep kSoundSearchingLoopSteps[] = {
    {1600, 60},
    {0, 300},
};

static constexpr ToneStep kSoundAlarmFastSteps[] = {
    {1500, 80},
    {0, 40},
    {1500, 80},
    {0, 40},
    {1500, 80},
};

static constexpr ToneStep kSoundAlarmSlowSteps[] = {
    {700, 400},
    {0, 300},
};

static constexpr ToneStep kSoundMuteOnSteps[] = {
    {500, 70},
    {0, 40},
    {500, 70},
};

static constexpr ToneStep kSoundMuteOffSteps[] = {
    {1000, 70},
    {0, 40},
    {1000, 70},
};

static constexpr ToneStep kSoundDoorOpenWarnSteps[] = {
    {800, 70},
    {1200, 90},
    {800, 70},
};

static constexpr ToneStep kSoundDoorFaultSteps[] = {
    {400, 200},
    {0, 100},
};

static constexpr ToneStep kSoundClick1Steps[] = {
    {2000, 40},
};

static constexpr ToneStep kSoundClick2Steps[] = {
    {2200, 40},
    {0, 40},
    {2200, 40},
};

static constexpr ToneStep kSoundSuccessHappySteps[] = {
    {1000, 60},
    {1400, 60},
    {1800, 100},
};

static constexpr ToneStep kSoundWarningBeepSteps[] = {
    {800, 200},
};

static constexpr ToneStep kSoundChargingUpSteps[] = {
    {700, 60},
    {900, 60},
    {1100, 60},
    {1400, 80},
};

static constexpr ToneStep kSoundChargingDownSteps[] = {
    {1400, 60},
    {1100, 60},
    {900, 60},
    {700, 80},
};

static constexpr ToneStep kSoundNotifySteps[] = {
    {1300, 120},
};

static constexpr ToneStep kSoundBossFightSteps[] = {
    {1800, 70},
    {800, 90},
    {2000, 120},
};

#define SHUTUP_STEP_COUNT(steps) static_cast<uint8_t>(sizeof(steps) / sizeof((steps)[0]))

static constexpr ToneSound kToneSounds[] = {
    {0, "None", false, kSoundNoneSteps, SHUTUP_STEP_COUNT(kSoundNoneSteps)},
    {1, "PowerUp", false, kSoundPowerUpSteps, SHUTUP_STEP_COUNT(kSoundPowerUpSteps)},
    {2, "PowerDown", false, kSoundPowerDownSteps, SHUTUP_STEP_COUNT(kSoundPowerDownSteps)},
    {3, "PewPew", false, kSoundPewPewSteps, SHUTUP_STEP_COUNT(kSoundPewPewSteps)},
    {4, "Pulse", false, kSoundPulseSteps, SHUTUP_STEP_COUNT(kSoundPulseSteps)},
    {5, "Confirm", false, kSoundConfirmSteps, SHUTUP_STEP_COUNT(kSoundConfirmSteps)},
    {6, "Error", false, kSoundErrorSteps, SHUTUP_STEP_COUNT(kSoundErrorSteps)},
    {7, "SaveOK", false, kSoundSaveOkSteps, SHUTUP_STEP_COUNT(kSoundSaveOkSteps)},
    {8, "Restarting", false, kSoundRestartingSteps, SHUTUP_STEP_COUNT(kSoundRestartingSteps)},
    {9, "ConnectWiFi", false, kSoundConnectWifiSteps, SHUTUP_STEP_COUNT(kSoundConnectWifiSteps)},
    {10, "LostWiFi", false, kSoundLostWifiSteps, SHUTUP_STEP_COUNT(kSoundLostWifiSteps)},
    {11, "SearchingLoop", true, kSoundSearchingLoopSteps, SHUTUP_STEP_COUNT(kSoundSearchingLoopSteps)},
    {12, "AlarmFast", true, kSoundAlarmFastSteps, SHUTUP_STEP_COUNT(kSoundAlarmFastSteps)},
    {13, "AlarmSlow", true, kSoundAlarmSlowSteps, SHUTUP_STEP_COUNT(kSoundAlarmSlowSteps)},
    {14, "MuteOn", false, kSoundMuteOnSteps, SHUTUP_STEP_COUNT(kSoundMuteOnSteps)},
    {15, "MuteOff", false, kSoundMuteOffSteps, SHUTUP_STEP_COUNT(kSoundMuteOffSteps)},
    {16, "DoorOpenWarn", false, kSoundDoorOpenWarnSteps, SHUTUP_STEP_COUNT(kSoundDoorOpenWarnSteps)},
    {17, "DoorFault", false, kSoundDoorFaultSteps, SHUTUP_STEP_COUNT(kSoundDoorFaultSteps)},
    {18, "Click1", false, kSoundClick1Steps, SHUTUP_STEP_COUNT(kSoundClick1Steps)},
    {19, "Click2", false, kSoundClick2Steps, SHUTUP_STEP_COUNT(kSoundClick2Steps)},
    {20, "SuccessHappy", false, kSoundSuccessHappySteps, SHUTUP_STEP_COUNT(kSoundSuccessHappySteps)},
    {21, "WarningBeep", true, kSoundWarningBeepSteps, SHUTUP_STEP_COUNT(kSoundWarningBeepSteps)},
    {22, "ChargingUp", false, kSoundChargingUpSteps, SHUTUP_STEP_COUNT(kSoundChargingUpSteps)},
    {23, "ChargingDown", false, kSoundChargingDownSteps, SHUTUP_STEP_COUNT(kSoundChargingDownSteps)},
    {24, "Notify", false, kSoundNotifySteps, SHUTUP_STEP_COUNT(kSoundNotifySteps)},
    {25, "BossFight", false, kSoundBossFightSteps, SHUTUP_STEP_COUNT(kSoundBossFightSteps)},
};

#undef SHUTUP_STEP_COUNT

static constexpr uint8_t kToneSoundCount = static_cast<uint8_t>(sizeof(kToneSounds) / sizeof(kToneSounds[0]));

inline const ToneSound *findToneSoundByName(const String &name) {
  for (uint8_t i = 0; i < kToneSoundCount; ++i) {
    if (name.equals(kToneSounds[i].name)) {
      return &kToneSounds[i];
    }
  }
  return nullptr;
}

inline const ToneSound *findToneSoundById(uint8_t id) {
  for (uint8_t i = 0; i < kToneSoundCount; ++i) {
    if (kToneSounds[i].id == id) {
      return &kToneSounds[i];
    }
  }
  return nullptr;
}

inline uint8_t toneSoundIdByName(const String &name) {
  const ToneSound *sound = findToneSoundByName(name);
  return sound ? sound->id : 0;
}

}  // namespace shutup
