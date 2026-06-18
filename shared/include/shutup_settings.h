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
constexpr const char *kDefaultUteColor = "black";

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
  bool repeat{false};
  uint32_t delayMs{0};
};

struct DoorOverlayConfig {
  String name;
  uint16_t width{0};
  uint16_t height{0};
  uint16_t x{0};
  uint16_t y{0};
  uint32_t closedColor{0x00FF00};
  uint32_t openColor{0xFF0000};
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
    deviceName_ = defaultName;
    hasPeer_ = prefs_.getBool("hasPeer", false);
    prefs_.getBytes("peer", peerMac_, sizeof(peerMac_));
    doorEnabledMask_ = prefs_.getUChar("doorEn", 0);
    uteColor_ = cleanUteColor(prefs_.getString("uteColor", kDefaultUteColor));
    configRevision_ = prefs_.getUInt("cfgRev", 1);
    if (configRevision_ == 0) {
      configRevision_ = 1;
    }
    for (uint8_t i = 0; i < kSoundActionCount; ++i) {
      soundActions_[i].cabSound = prefs_.getString(soundKey(i, "cab").c_str(), kNoSoundName);
      soundActions_[i].canopySound = prefs_.getString(soundKey(i, "can").c_str(), kNoSoundName);
      soundActions_[i].repeat = prefs_.getBool(soundKey(i, "rep").c_str(), false);
      soundActions_[i].delayMs = prefs_.getUInt(soundKey(i, "del").c_str(), 0);
    }
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      doorOverlays_[i].name = prefs_.getString(doorKey(i, "name").c_str(), defaultDoorName(i));
      doorOverlays_[i].width = prefs_.getUShort(doorKey(i, "w").c_str(), 0);
      doorOverlays_[i].height = prefs_.getUShort(doorKey(i, "h").c_str(), 0);
      doorOverlays_[i].x = prefs_.getUShort(doorKey(i, "x").c_str(), 0);
      doorOverlays_[i].y = prefs_.getUShort(doorKey(i, "y").c_str(), 0);
      doorOverlays_[i].closedColor = prefs_.getUInt(doorKey(i, "closed").c_str(), 0x00FF00);
      doorOverlays_[i].openColor = prefs_.getUInt(doorKey(i, "open").c_str(), 0xFF0000);
    }
  }

  DeviceRole role() const { return role_; }
  const String &deviceName() const { return deviceName_; }
  bool hasPeer() const { return hasPeer_; }
  const uint8_t *peerMac() const { return peerMac_; }
  uint8_t doorEnabledMask() const { return doorEnabledMask_; }
  uint32_t configRevision() const { return configRevision_; }
  const SoundActionConfig &soundAction(uint8_t index) const { return soundActions_[index]; }
  const DoorOverlayConfig &doorOverlay(uint8_t index) const { return doorOverlays_[index]; }
  const String &uteColor() const { return uteColor_; }

  String peerMacString() const {
    return hasPeer_ ? macToString(peerMac_) : String("");
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

  void setDoorEnabledMask(uint8_t mask) {
    doorEnabledMask_ = static_cast<uint8_t>(mask & ((1U << kDoorCount) - 1U));
    prefs_.putUChar("doorEn", doorEnabledMask_);
  }

  bool doorEnabled(uint8_t index) const {
    if (index >= kDoorCount) {
      return false;
    }
    return (doorEnabledMask_ & (1U << index)) != 0;
  }

  void setSoundAction(uint8_t index, const String &cabSound, const String &canopySound,
                      bool repeat, uint32_t delayMs) {
    if (index >= kSoundActionCount) {
      return;
    }
    soundActions_[index].cabSound = cleanSoundName(cabSound);
    soundActions_[index].canopySound = cleanSoundName(canopySound);
    soundActions_[index].repeat = repeat;
    soundActions_[index].delayMs = delayMs;
    prefs_.putString(soundKey(index, "cab").c_str(), soundActions_[index].cabSound);
    prefs_.putString(soundKey(index, "can").c_str(), soundActions_[index].canopySound);
    prefs_.putBool(soundKey(index, "rep").c_str(), soundActions_[index].repeat);
    prefs_.putUInt(soundKey(index, "del").c_str(), soundActions_[index].delayMs);
  }

  void setDoorOverlay(uint8_t index, const String &name, uint16_t width, uint16_t height, uint16_t x, uint16_t y,
                      uint32_t closedColor, uint32_t openColor) {
    if (index >= kDoorCount) {
      return;
    }
    doorOverlays_[index].name = cleanDoorName(index, name);
    doorOverlays_[index].width = width;
    doorOverlays_[index].height = height;
    doorOverlays_[index].x = x;
    doorOverlays_[index].y = y;
    doorOverlays_[index].closedColor = closedColor;
    doorOverlays_[index].openColor = openColor;
    prefs_.putString(doorKey(index, "name").c_str(), doorOverlays_[index].name);
    prefs_.putUShort(doorKey(index, "w").c_str(), doorOverlays_[index].width);
    prefs_.putUShort(doorKey(index, "h").c_str(), doorOverlays_[index].height);
    prefs_.putUShort(doorKey(index, "x").c_str(), doorOverlays_[index].x);
    prefs_.putUShort(doorKey(index, "y").c_str(), doorOverlays_[index].y);
    prefs_.putUInt(doorKey(index, "closed").c_str(), doorOverlays_[index].closedColor);
    prefs_.putUInt(doorKey(index, "open").c_str(), doorOverlays_[index].openColor);
  }

  void setUteColor(const String &color) {
    uteColor_ = cleanUteColor(color);
    prefs_.putString("uteColor", uteColor_);
  }

  void markConfigChanged() {
    ++configRevision_;
    if (configRevision_ == 0) {
      configRevision_ = 1;
    }
    prefs_.putUInt("cfgRev", configRevision_);
  }

private:
  static String soundKey(uint8_t index, const char *suffix) {
    return String("a") + String(index) + suffix;
  }

  static String doorKey(uint8_t index, const char *suffix) {
    return String("d") + String(index) + suffix;
  }

  static String defaultDoorName(uint8_t index) {
    return String("Door #") + String(index + 1);
  }

  static String cleanDoorName(uint8_t index, const String &name) {
    String out = name;
    out.trim();
    if (out.length() == 0) {
      return defaultDoorName(index);
    }
    if (out.length() > 31) {
      out = out.substring(0, 31);
    }
    return out;
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

  static String cleanUteColor(const String &color) {
    String out = color;
    out.trim();
    out.toLowerCase();
    if (out == "black" || out == "white" || out == "gray" || out == "red" ||
        out == "blue" || out == "green" || out == "yellow") {
      return out;
    }
    return kDefaultUteColor;
  }

  Preferences prefs_;
  DeviceRole role_{DeviceRole::Cab};
  String deviceName_;
  String uteColor_{kDefaultUteColor};
  uint8_t peerMac_[6]{};
  bool hasPeer_{false};
  uint8_t doorEnabledMask_{0};
  uint32_t configRevision_{1};
  SoundActionConfig soundActions_[kSoundActionCount]{};
  DoorOverlayConfig doorOverlays_[kDoorCount]{};
};

}  // namespace shutup
