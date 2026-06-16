#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#include "shutup_settings.h"

namespace shutup {

constexpr uint8_t kEspNowChannel = 1;

enum class PacketType : uint8_t {
  PairRequest = 1,
  PairAck = 2,
  StateRequest = 3,
  StateResponse = 4,
  Heartbeat = 5,
  ConfigSync = 6,
};

struct ShutupPacket {
  uint32_t magic;
  uint8_t version;
  uint8_t type;
  uint8_t role;
  uint8_t mac[6];
  uint32_t sequence;
  uint8_t doorEnabledMask;
  uint8_t doorOpenMask;
  char name[24];
  uint8_t configIndex;
  uint8_t reserved[3];
  uint32_t repeatMs;
  uint32_t delayMs;
  char cabSound[24];
  char canopySound[24];
};

class EspNowManager {
public:
  bool begin(DeviceRole role, SettingsStore &settings, bool configMode) {
    role_ = role;
    settings_ = &settings;
    configMode_ = configMode;
    WiFi.mode(configMode ? WIFI_AP_STA : WIFI_STA);
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
        sendStateRequest();
      }
    }
    if (!configMode_ && role_ == DeviceRole::Canopy && settings_ && settings_->hasPeer()) {
      if (now - lastConfigSyncMs_ >= 1000) {
        lastConfigSyncMs_ = now;
        sendConfigSync(settings_->peerMac(), nextConfigSyncIndex_);
        nextConfigSyncIndex_ = static_cast<uint8_t>((nextConfigSyncIndex_ + 1) % kSoundActionCount);
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
  uint8_t lastDoorEnabledMask() const { return lastDoorEnabledMask_; }
  uint8_t lastDoorOpenMask() const { return lastDoorOpenMask_; }
  uint32_t lastStateMessageMs() const { return lastStateMessageMs_; }
  uint32_t lastCabSeenMs() const { return lastCabSeenMs_; }
  bool cabLinkActive(uint32_t now = millis()) const { return lastCabSeenMs_ != 0 && now - lastCabSeenMs_ <= 5000; }
  bool cabLinkStale(uint32_t now = millis()) const { return lastCabSeenMs_ != 0 && now - lastCabSeenMs_ > 5000 && now - lastCabSeenMs_ <= 15000; }
  bool cabStateFresh(uint32_t now = millis()) const { return lastStateMessageMs_ != 0 && now - lastStateMessageMs_ <= 5000; }
  uint8_t heartbeatSuccessPercent() const {
    if (heartbeatSent_ == 0) {
      return 0;
    }
    return static_cast<uint8_t>((heartbeatOk_ * 100U) / heartbeatSent_);
  }

  void setCanopyDoorState(uint8_t enabledMask, uint8_t openMask) {
    doorEnabledMask_ = enabledMask;
    doorOpenMask_ = openMask;
  }

private:
  static constexpr uint32_t kMagic = 0x53545550UL;
  static constexpr uint8_t kVersion = 1;

  static void onReceiveStatic(const uint8_t *mac, const uint8_t *data, int len) {
    if (instance()) {
      instance()->onReceive(mac, data, len);
    }
  }

  static void onSendStatic(const uint8_t *, esp_now_send_status_t) {}

  void onReceive(const uint8_t *senderMac, const uint8_t *data, int len) {
    if (len != static_cast<int>(sizeof(ShutupPacket))) {
      return;
    }
    ShutupPacket packet{};
    memcpy(&packet, data, sizeof(packet));
    if (packet.magic != kMagic || packet.version != kVersion) {
      return;
    }
    if (packet.role == static_cast<uint8_t>(role_)) {
      return;
    }

    const auto type = static_cast<PacketType>(packet.type);
    if (type == PacketType::PairRequest && configMode_) {
      settings_->setPeerMac(senderMac);
      addPeer(senderMac);
      lastPairMessage_ = String("Paired with ") + roleName(static_cast<DeviceRole>(packet.role)) +
                         " " + macToString(senderMac);
      sendPairAck(senderMac);
      return;
    }
    if (type == PacketType::PairAck && configMode_) {
      settings_->setPeerMac(senderMac);
      addPeer(senderMac);
      pairingActive_ = false;
      lastPairMessage_ = String("Pairing complete with ") + roleName(static_cast<DeviceRole>(packet.role)) +
                         " " + macToString(senderMac);
      return;
    }
    if (type == PacketType::StateRequest && role_ == DeviceRole::Canopy) {
      lastCabSeenMs_ = millis();
      sendStateResponse(senderMac);
      return;
    }
    if (type == PacketType::StateResponse && role_ == DeviceRole::Cab) {
      lastDoorEnabledMask_ = packet.doorEnabledMask;
      lastDoorOpenMask_ = packet.doorOpenMask;
      lastStateMessageMs_ = millis();
      if (heartbeatOk_ < 250) {
        ++heartbeatOk_;
      }
      return;
    }
    if (type == PacketType::ConfigSync && role_ == DeviceRole::Cab && packet.configIndex < kSoundActionCount) {
      settings_->setSoundAction(packet.configIndex, packet.cabSound, packet.canopySound, packet.repeatMs, packet.delayMs);
      return;
    }
  }

  void fillPacket(ShutupPacket &packet, PacketType type) {
    packet.magic = kMagic;
    packet.version = kVersion;
    packet.type = static_cast<uint8_t>(type);
    packet.role = static_cast<uint8_t>(role_);
    WiFi.macAddress(packet.mac);
    packet.sequence = ++sequence_;
    packet.doorEnabledMask = doorEnabledMask_;
    packet.doorOpenMask = doorOpenMask_;
    if (settings_) {
      strlcpy(packet.name, settings_->deviceName().c_str(), sizeof(packet.name));
    }
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
    ShutupPacket packet{};
    fillPacket(packet, PacketType::PairRequest);
    esp_now_send(broadcast, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
    lastPairMessage_ = "Pairing broadcast sent. Put the other device in config mode.";
  }

  void sendPairAck(const uint8_t mac[6]) {
    ShutupPacket packet{};
    fillPacket(packet, PacketType::PairAck);
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void sendStateRequest() {
    if (!settings_ || !settings_->hasPeer()) {
      return;
    }
    ShutupPacket packet{};
    fillPacket(packet, PacketType::StateRequest);
    if (heartbeatSent_ < 250) {
      ++heartbeatSent_;
    } else {
      heartbeatSent_ = static_cast<uint16_t>((heartbeatSent_ / 2U) + 1U);
      heartbeatOk_ = static_cast<uint16_t>(heartbeatOk_ / 2U);
    }
    esp_now_send(settings_->peerMac(), reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void sendStateResponse(const uint8_t mac[6]) {
    ShutupPacket packet{};
    fillPacket(packet, PacketType::StateResponse);
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  void sendConfigSync(const uint8_t mac[6], uint8_t index) {
    if (!settings_ || index >= kSoundActionCount) {
      return;
    }
    ShutupPacket packet{};
    fillPacket(packet, PacketType::ConfigSync);
    const SoundActionConfig &action = settings_->soundAction(index);
    packet.configIndex = index;
    packet.repeatMs = action.repeatMs;
    packet.delayMs = action.delayMs;
    strlcpy(packet.cabSound, action.cabSound.c_str(), sizeof(packet.cabSound));
    strlcpy(packet.canopySound, action.canopySound.c_str(), sizeof(packet.canopySound));
    esp_now_send(mac, reinterpret_cast<uint8_t *>(&packet), sizeof(packet));
  }

  static EspNowManager *&instance() {
    static EspNowManager *active = nullptr;
    return active;
  }

  DeviceRole role_{DeviceRole::Cab};
  SettingsStore *settings_{nullptr};
  bool configMode_{false};
  bool pairingActive_{false};
  uint32_t pairingUntilMs_{0};
  uint32_t lastPairBroadcastMs_{0};
  uint32_t lastStateRequestMs_{0};
  uint32_t lastStateMessageMs_{0};
  uint32_t lastCabSeenMs_{0};
  uint32_t lastConfigSyncMs_{0};
  uint32_t sequence_{0};
  uint16_t heartbeatSent_{0};
  uint16_t heartbeatOk_{0};
  uint8_t nextConfigSyncIndex_{0};
  uint8_t doorEnabledMask_{0};
  uint8_t doorOpenMask_{0};
  uint8_t lastDoorEnabledMask_{0};
  uint8_t lastDoorOpenMask_{0};
  String localMac_;
  String lastPairMessage_{"Idle"};
};

}  // namespace shutup
