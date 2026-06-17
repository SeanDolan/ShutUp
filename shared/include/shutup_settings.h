#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace shutup {

enum class DeviceRole : uint8_t {
  Cab = 1,
  Canopy = 2,
};

constexpr uint8_t kDoorCount = 6;
constexpr uint8_t kSoundActionCount = 5;
constexpr const char *kNoSoundName = "None";

enum class DoorState : uint8_t {
  Disabled = 0,
  Closed = 1,
  Open = 2,
};

inline const char *doorStateName(DoorState state) {
  switch (state) {
    case DoorState::Disabled:
      return "disabled";
    case DoorState::Closed:
      return "closed";
    case DoorState::Open:
      return "open";
  }
  return "disabled";
}

enum class SoundAction : uint8_t {
  Startup = 0,
  ConnectivitySuccess = 1,
  ConnectivityError = 2,
  DoorsOk = 3,
  DoorAlarm = 4,
};

inline const char *soundActionName(uint8_t index) {
  switch (static_cast<SoundAction>(index)) {
    case SoundAction::Startup:
      return "Startup";
    case SoundAction::ConnectivitySuccess:
      return "Connectivity success";
    case SoundAction::ConnectivityError:
      return "Connectivity error";
    case SoundAction::DoorsOk:
      return "Doors ok";
    case SoundAction::DoorAlarm:
      return "Door alarm";
  }
  return "Unknown";
}

struct SoundActionConfig {
  String cabSound{kNoSoundName};
  String canopySound{kNoSoundName};
  uint32_t repeatMs{0};
  uint32_t delayMs{0};
};

inline const char *roleName(DeviceRole role) {
  return role == DeviceRole::Cab ? "Cab" : "Canopy";
}

inline const char *roleKey(DeviceRole role) {
  return role == DeviceRole::Cab ? "cab" : "canopy";
}

inline String macToString(const uint8_t mac[6]) {
  char text[18];
  snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(text);
}

inline bool parseMacString(const String &text, uint8_t out[6]) {
  unsigned int values[6];
  if (sscanf(text.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
    return false;
  }
  for (uint8_t i = 0; i < 6; ++i) {
    if (values[i] > 0xFF) {
      return false;
    }
    out[i] = static_cast<uint8_t>(values[i]);
  }
  return true;
}

class SettingsStore {
public:
  void begin(DeviceRole role, const char *defaultName) {
    role_ = role;
    prefs_.begin(roleKey(role), false);
    deviceName_ = prefs_.getString("name", defaultName);
    hasPeer_ = prefs_.getBool("hasPeer", false);
    prefs_.getBytes("peer", peerMac_, sizeof(peerMac_));
    doorEnabledMask_ = prefs_.getUChar("doorEn", 0);
    for (uint8_t i = 0; i < kSoundActionCount; ++i) {
      soundActions_[i].cabSound = prefs_.getString(soundKey(i, "cab").c_str(), kNoSoundName);
      soundActions_[i].canopySound = prefs_.getString(soundKey(i, "can").c_str(), kNoSoundName);
      soundActions_[i].repeatMs = prefs_.getUInt(soundKey(i, "rep").c_str(), 0);
      soundActions_[i].delayMs = prefs_.getUInt(soundKey(i, "del").c_str(), 0);
    }
  }

  DeviceRole role() const { return role_; }
  const String &deviceName() const { return deviceName_; }
  bool hasPeer() const { return hasPeer_; }
  const uint8_t *peerMac() const { return peerMac_; }
  uint8_t doorEnabledMask() const { return doorEnabledMask_; }
  const SoundActionConfig &soundAction(uint8_t index) const { return soundActions_[index]; }

  String peerMacString() const {
    return hasPeer_ ? macToString(peerMac_) : String("");
  }

  void setDeviceName(const String &name) {
    String trimmed = name;
    trimmed.trim();
    if (trimmed.length() == 0) {
      return;
    }
    if (trimmed.length() > 31) {
      trimmed = trimmed.substring(0, 31);
    }
    deviceName_ = trimmed;
    prefs_.putString("name", deviceName_);
  }

  void setPeerMac(const uint8_t mac[6]) {
    memcpy(peerMac_, mac, sizeof(peerMac_));
    hasPeer_ = true;
    prefs_.putBytes("peer", peerMac_, sizeof(peerMac_));
    prefs_.putBool("hasPeer", true);
  }

  void clearPeer() {
    memset(peerMac_, 0, sizeof(peerMac_));
    hasPeer_ = false;
    prefs_.putBytes("peer", peerMac_, sizeof(peerMac_));
    prefs_.putBool("hasPeer", false);
  }

  void setDoorEnabled(uint8_t index, bool enabled) {
    if (index >= kDoorCount) {
      return;
    }
    const uint8_t bit = static_cast<uint8_t>(1U << index);
    if (enabled) {
      doorEnabledMask_ |= bit;
    } else {
      doorEnabledMask_ &= static_cast<uint8_t>(~bit);
    }
    prefs_.putUChar("doorEn", doorEnabledMask_);
  }

  bool doorEnabled(uint8_t index) const {
    if (index >= kDoorCount) {
      return false;
    }
    return (doorEnabledMask_ & (1U << index)) != 0;
  }

  void setSoundAction(uint8_t index, const String &cabSound, const String &canopySound,
                      uint32_t repeatMs, uint32_t delayMs) {
    if (index >= kSoundActionCount) {
      return;
    }
    soundActions_[index].cabSound = cleanSoundName(cabSound);
    soundActions_[index].canopySound = cleanSoundName(canopySound);
    soundActions_[index].repeatMs = repeatMs;
    soundActions_[index].delayMs = delayMs;
    prefs_.putString(soundKey(index, "cab").c_str(), soundActions_[index].cabSound);
    prefs_.putString(soundKey(index, "can").c_str(), soundActions_[index].canopySound);
    prefs_.putUInt(soundKey(index, "rep").c_str(), soundActions_[index].repeatMs);
    prefs_.putUInt(soundKey(index, "del").c_str(), soundActions_[index].delayMs);
  }

private:
  static String soundKey(uint8_t index, const char *suffix) {
    return String("a") + String(index) + suffix;
  }

  static String cleanSoundName(const String &name) {
    String out = name;
    out.trim();
    if (out.length() == 0) {
      return kNoSoundName;
    }
    if (out.length() > 31) {
      out = out.substring(0, 31);
    }
    return out;
  }

  Preferences prefs_;
  DeviceRole role_{DeviceRole::Cab};
  String deviceName_;
  uint8_t peerMac_[6]{};
  bool hasPeer_{false};
  uint8_t doorEnabledMask_{0};
  SoundActionConfig soundActions_[kSoundActionCount]{};
};

}  // namespace shutup
