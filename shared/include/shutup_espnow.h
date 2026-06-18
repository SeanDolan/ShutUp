#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#include "shutup_settings.h"
#include "shutup_sounds.h"

namespace shutup {

constexpr uint8_t kEspNowChannel = 1;

enum class PacketType : uint8_t {
  PairRequest = 1,
  PairAck = 2,
  HeartbeatRequest = 3,
  HeartbeatResponse = 4,
  ConfigSnapshot = 5,
};

struct PacketHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t type;
  uint8_t role;
  uint8_t reserved;
  uint8_t mac[6];
  uint32_t sequence;
  uint32_t configRevision;
};

struct PairPacket {
  PacketHeader header;
};

struct HeartbeatRequestPacket {
  PacketHeader header;
};

struct HeartbeatResponsePacket {
  PacketHeader header;
  uint8_t doorStates[kDoorCount];
};

struct CabSoundSnapshot {
  uint8_t soundId;
  uint8_t repeat;
  uint16_t reserved;
  uint32_t delayMs;
};

struct DoorOverlaySnapshot {
  uint16_t width;
  uint16_t height;
  uint16_t x;
  uint16_t y;
  uint32_t closedColor;
  uint32_t openColor;
};

struct CabConfigSnapshotPacket {
  PacketHeader header;
  uint8_t uteColorId;
  uint8_t doorEnabledMask;
  uint16_t reserved;
  CabSoundSnapshot sounds[kSoundActionCount];
  DoorOverlaySnapshot overlays[kDoorCount];
};

static_assert(sizeof(CabConfigSnapshotPacket) <= 250, "ESP-NOW config snapshot must fit the standard payload limit");

class EspNowManager {
public:
  bool begin(DeviceRole role, SettingsStore &settings, bool configMode) {
    role_ = role;
    settings_ = &settings;
    configMode_ = configMode;
    WiFi.mode(configMode && role == DeviceRole::Canopy ? WIFI_AP_STA : WIFI_STA);
    WiFi.setSleep(false);
    localMac_ = WiFi.macAddress();
    if (esp_now_init() != ESP_OK) {
      Serial.println("ESP-NOW init failed");
      return false;
    }
    instance() = this;
    esp_now_register_recv_cb(&EspNowManager::onReceiveStatic);
    esp_now_register_send_cb(&EspNowManager::onSendStatic);
    addBroadcastPeer();
    if (!configMode_ && settings.hasPeer()) {
      addPeer(settings.peerMac());
    }
    return true;
  }

  void loop() {
    const uint32_t now = millis();
    if (pairingRebootAtMs_ != 0 && static_cast<int32_t>(now - pairingRebootAtMs_) >= 0) {
      ESP.restart();
    }
    if (pairingActive_ && now < pairingUntilMs_ && now - lastPairBroadcastMs_ >= 1000) {
      lastPairBroadcastMs_ = now;
      sendPairRequest();
    }
    if (pairingActive_ && now >= pairingUntilMs_) {
      pairingActive_ = false;
    }
    if (!configMode_ && role_ == DeviceRole::Cab && settings_ && settings_->hasPeer()) {
      if (now - lastStateRequestMs_ >= 1000) {
        lastStateRequestMs_ = now;
        sendHeartbeatRequest();
      }
    }
  }

  void startPairing() {
    pairingActive_ = true;
    pairingUntilMs_ = millis() + 60000;
    lastPairBroadcastMs_ = 0;
    sendPairRequest();
  }

  bool pairingActive() const { return pairingActive_; }
  String localMacString() const { return localMac_; }
  String lastPairMessage() const { return lastPairMessage_; }
  bool normalLinkReady() const { return settings_ && settings_->hasPeer(); }
  DoorState lastDoorState(uint8_t index) const {
    if (index >= kDoorCount) {
      return DoorState::Disabled;
    }
    return lastDoorStates_[index];
  }
  uint8_t lastDoorEnabledMask() const { return doorEnabledMaskFrom(lastDoorStates_); }
  uint8_t lastDoorOpenMask() const { return doorOpenMaskFrom(lastDoorStates_); }
  uint32_t lastStateMessageMs() const { return lastStateMessageMs_; }
  uint32_t lastCabSeenMs() const { return lastCabSeenMs_; }
  bool cabLinkActive(uint32_t now = millis()) const { return lastCabSeenMs_ != 0 && now - lastCabSeenMs_ <= 5000; }
  bool cabLinkStale(uint32_t now = millis()) const { return lastCabSeenMs_ != 0 && now - lastCabSeenMs_ > 5000 && now - lastCabSeenMs_ <= 15000; }
  bool cabStateFresh(uint32_t now = millis()) const { return lastStateMessageMs_ != 0 && now - lastStateMessageMs_ <= 5000; }
  bool cabConfigSyncComplete() const { return cabConfigRevision_ != 0; }
  uint8_t heartbeatSuccessPercent() const {
    if (heartbeatResultCount_ == 0) {
      return 0;
    }
    uint8_t successful = 0;
    for (uint8_t i = 0; i < heartbeatResultCount_; ++i) {
      successful += heartbeatResults_[i] != 0 ? 1 : 0;
    }
    return static_cast<uint8_t>((successful * 100U) / heartbeatResultCount_);
  }
  uint16_t averageHeartbeatMs() const { return averageHeartbeatMs_; }
  void setCanopyDoorStates(const DoorState states[kDoorCount]) {
    memcpy(doorStates_, states, sizeof(doorStates_));
  }

private:
  static constexpr uint32_t kMagic = 0x53545550UL;
  static constexpr uint8_t kVersion = 2;

  static void onReceiveStatic(const uint8_t *mac, const uint8_t *data, int len) {
    if (instance()) {
      instance()->onReceive(mac, data, len);
    }
  }

  static void onSendStatic(const uint8_t *, esp_now_send_status_t) {}

  void onReceive(const uint8_t *senderMac, const uint8_t *data, int len) {
    if (len < static_cast<int>(sizeof(PacketHeader))) {
      return;
    }
    PacketHeader header{};
    memcpy(&header, data, sizeof(header));
    if (header.magic != kMagic || header.version != kVersion) {
      return;
    }
    if (header.role == static_cast<uint8_t>(role_)) {
      return;
    }

    const auto type = static_cast<PacketType>(header.type);
    if (type == PacketType::PairRequest && configMode_) {
      settings_->setPeerMac(senderMac);
      addPeer(senderMac);
      lastPairMessage_ = String("Paired with ") + roleName(static_cast<DeviceRole>(header.role)) +
                         " " + macToString(senderMac);
      sendPairAck(senderMac);
      schedulePairingReboot();
      return;
    }
    if (type == PacketType::PairAck && configMode_) {
      settings_->setPeerMac(senderMac);
      addPeer(senderMac);
      pairingActive_ = false;
      lastPairMessage_ = String("Pairing complete with ") + roleName(static_cast<DeviceRole>(header.role)) +
                         " " + macToString(senderMac);
      schedulePairingReboot();
      return;
    }
    if (type == PacketType::HeartbeatRequest && role_ == DeviceRole::Canopy &&
        len == static_cast<int>(sizeof(HeartbeatRequestPacket))) {
      lastCabSeenMs_ = millis();
      sendHeartbeatResponse(senderMac, header.sequence);
      if (settings_ && header.configRevision != settings_->configRevision()) {
        sendConfigSnapshot(senderMac);
      }
      return;
    }
    if (type == PacketType::HeartbeatResponse && role_ == DeviceRole::Cab &&
        len == static_cast<int>(sizeof(HeartbeatResponsePacket))) {
      HeartbeatResponsePacket packet{};
      memcpy(&packet, data, sizeof(packet));
      const uint32_t now = millis();
      memcpy(lastDoorStates_, packet.doorStates, sizeof(lastDoorStates_));
      lastStateMessageMs_ = now;
      if (heartbeatPending_ && packet.header.sequence == lastHeartbeatSequence_) {
        heartbeatPending_ = false;
        recordHeartbeatResult(true);
        const uint32_t elapsed = now - lastHeartbeatSentMs_;
        recordHeartbeatLatency(static_cast<uint16_t>(elapsed > 65535 ? 65535 : elapsed));
      }
      return;
    }
    if (type == PacketType::ConfigSnapshot && role_ == DeviceRole::Cab &&
        len == static_cast<int>(sizeof(CabConfigSnapshotPacket)) && settings_) {
      CabConfigSnapshotPacket packet{};
      memcpy(&packet, data, sizeof(packet));
      settings_->setUteColor(uteColorName(packet.uteColorId));
      settings_->setDoorEnabledMask(packet.doorEnabledMask);
      for (uint8_t i = 0; i < kSoundActionCount; ++i) {
        const ToneSound *sound = findToneSoundById(packet.sounds[i].soundId);
        const String canopySound = settings_->soundAction(i).canopySound;
        settings_->setSoundAction(i, sound ? sound->name : kNoSoundName, canopySound,
                                  packet.sounds[i].repeat != 0, packet.sounds[i].delayMs);
      }
      for (uint8_t i = 0; i < kDoorCount; ++i) {
        const String name = settings_->doorOverlay(i).name;
        settings_->setDoorOverlay(i, name, packet.overlays[i].width, packet.overlays[i].height,
                                  packet.overlays[i].x, packet.overlays[i].y,
                                  packet.overlays[i].closedColor, packet.overlays[i].openColor);
      }
      cabConfigRevision_ = packet.header.configRevision;
      return;
    }
  }

  void fillHeader(PacketHeader &header, PacketType type) {
    header.magic = kMagic;
    header.version = kVersion;
    header.type = static_cast<uint8_t>(type);
    header.role = static_cast<uint8_t>(role_);
    WiFi.macAddress(header.mac);
    header.sequence = ++sequence_;
    header.configRevision = settings_ ? settings_->configRevision() : 0;
  }

  bool addPeer(const uint8_t mac[6]) {
    if (esp_now_is_peer_exist(mac)) {
      return true;
    }
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = kEspNowChannel;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
  }

  void addBroadcastPeer() {
    const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    addPeer(broadcast);
  }

  void sendPairRequest() {
    const uint8_t broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    PairPacket packet{};
    fillHeader(packet.header, PacketType::PairRequest);
    esp_now_send(broadcast, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    lastPairMessage_ = "Pairing broadcast sent. Put the Cab in pairing mode.";
  }

  void sendPairAck(const uint8_t mac[6]) {
    PairPacket packet{};
    fillHeader(packet.header, PacketType::PairAck);
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void schedulePairingReboot() {
    if (pairingRebootAtMs_ == 0) {
      pairingRebootAtMs_ = millis() + 1000;
    }
  }

  void sendHeartbeatRequest() {
    if (!settings_ || !settings_->hasPeer()) {
      return;
    }
    if (heartbeatPending_) {
      heartbeatPending_ = false;
      recordHeartbeatResult(false);
    }
    HeartbeatRequestPacket packet{};
    fillHeader(packet.header, PacketType::HeartbeatRequest);
    packet.header.configRevision = cabConfigRevision_;
    lastHeartbeatSequence_ = packet.header.sequence;
    lastHeartbeatSentMs_ = millis();
    heartbeatPending_ = true;
    esp_now_send(settings_->peerMac(), reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void sendHeartbeatResponse(const uint8_t mac[6], uint32_t requestSequence) {
    HeartbeatResponsePacket packet{};
    fillHeader(packet.header, PacketType::HeartbeatResponse);
    packet.header.sequence = requestSequence;
    memcpy(packet.doorStates, doorStates_, sizeof(packet.doorStates));
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void sendConfigSnapshot(const uint8_t mac[6]) {
    if (!settings_) {
      return;
    }
    CabConfigSnapshotPacket packet{};
    fillHeader(packet.header, PacketType::ConfigSnapshot);
    packet.uteColorId = uteColorId(settings_->uteColor());
    packet.doorEnabledMask = settings_->doorEnabledMask();
    for (uint8_t i = 0; i < kSoundActionCount; ++i) {
      const SoundActionConfig &action = settings_->soundAction(i);
      packet.sounds[i].soundId = toneSoundIdByName(action.cabSound);
      packet.sounds[i].repeat = action.repeat ? 1 : 0;
      packet.sounds[i].delayMs = action.delayMs;
    }
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      const DoorOverlayConfig &overlay = settings_->doorOverlay(i);
      packet.overlays[i].width = overlay.width;
      packet.overlays[i].height = overlay.height;
      packet.overlays[i].x = overlay.x;
      packet.overlays[i].y = overlay.y;
      packet.overlays[i].closedColor = overlay.closedColor;
      packet.overlays[i].openColor = overlay.openColor;
    }
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  static EspNowManager *&instance() {
    static EspNowManager *active = nullptr;
    return active;
  }

  void recordHeartbeatLatency(uint16_t latencyMs) {
    heartbeatLatencies_[heartbeatLatencyIndex_] = latencyMs;
    heartbeatLatencyIndex_ = static_cast<uint8_t>((heartbeatLatencyIndex_ + 1) % kHeartbeatWindow);
    if (heartbeatLatencyCount_ < kHeartbeatWindow) {
      ++heartbeatLatencyCount_;
    }
    uint32_t total = 0;
    for (uint8_t i = 0; i < heartbeatLatencyCount_; ++i) {
      total += heartbeatLatencies_[i];
    }
    averageHeartbeatMs_ = static_cast<uint16_t>(total / heartbeatLatencyCount_);
  }

  void recordHeartbeatResult(bool successful) {
    heartbeatResults_[heartbeatResultIndex_] = successful ? 1 : 0;
    heartbeatResultIndex_ = static_cast<uint8_t>((heartbeatResultIndex_ + 1) % kHeartbeatWindow);
    if (heartbeatResultCount_ < kHeartbeatWindow) {
      ++heartbeatResultCount_;
    }
  }

  static uint8_t uteColorId(const String &color) {
    if (color == "white") return 1;
    if (color == "gray") return 2;
    if (color == "red") return 3;
    if (color == "blue") return 4;
    if (color == "green") return 5;
    if (color == "yellow") return 6;
    return 0;
  }

  static const char *uteColorName(uint8_t id) {
    static constexpr const char *kColors[] = {"black", "white", "gray", "red", "blue", "green", "yellow"};
    return id < (sizeof(kColors) / sizeof(kColors[0])) ? kColors[id] : kDefaultUteColor;
  }

  static uint8_t doorEnabledMaskFrom(const DoorState states[kDoorCount]) {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      if (states[i] != DoorState::Disabled) {
        mask |= static_cast<uint8_t>(1U << i);
      }
    }
    return mask;
  }

  static uint8_t doorOpenMaskFrom(const DoorState states[kDoorCount]) {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      if (states[i] == DoorState::Open) {
        mask |= static_cast<uint8_t>(1U << i);
      }
    }
    return mask;
  }

  DeviceRole role_{DeviceRole::Cab};
  SettingsStore *settings_{nullptr};
  bool configMode_{false};
  bool pairingActive_{false};
  uint32_t pairingUntilMs_{0};
  uint32_t pairingRebootAtMs_{0};
  uint32_t lastPairBroadcastMs_{0};
  uint32_t lastStateRequestMs_{0};
  uint32_t lastStateMessageMs_{0};
  uint32_t lastCabSeenMs_{0};
  uint32_t sequence_{0};
  static constexpr uint8_t kHeartbeatWindow = 5;
  uint8_t heartbeatResults_[kHeartbeatWindow]{};
  uint8_t heartbeatResultIndex_{0};
  uint8_t heartbeatResultCount_{0};
  bool heartbeatPending_{false};
  uint16_t heartbeatLatencies_[kHeartbeatWindow]{};
  uint8_t heartbeatLatencyIndex_{0};
  uint8_t heartbeatLatencyCount_{0};
  uint16_t averageHeartbeatMs_{0};
  uint32_t lastHeartbeatSequence_{0};
  uint32_t lastHeartbeatSentMs_{0};
  uint32_t cabConfigRevision_{0};
  DoorState doorStates_[kDoorCount]{};
  DoorState lastDoorStates_[kDoorCount]{};
  String localMac_;
  String lastPairMessage_{"Idle"};
};

}  // namespace shutup
