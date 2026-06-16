#pragma once

#include <Arduino.h>
#include <driver/rmt.h>

#include "shutup_settings.h"

namespace shutup {

enum class LinkLedState : uint8_t {
  Disconnected,
  Weak,
  Connected,
};

class CanopyStatusLeds {
public:
  void begin(int dataPin) {
    pin_ = dataPin;
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(static_cast<gpio_num_t>(pin_), kChannel);
    config.clk_div = 2;  // 40 MHz RMT clock, 25 ns ticks.
    config.tx_config.carrier_en = false;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config.tx_config.idle_output_en = true;
    rmt_config(&config);
    rmt_driver_install(kChannel, 0, 0);
    clear();
  }

  void show(uint8_t enabledMask, uint8_t openMask, LinkLedState linkState) {
    setPixel(0, 0, 48, 0);
    switch (linkState) {
      case LinkLedState::Connected:
        setPixel(1, 0, 0, 48);
        break;
      case LinkLedState::Weak:
        setPixel(1, 48, 20, 0);
        break;
      case LinkLedState::Disconnected:
        setPixel(1, 48, 0, 0);
        break;
    }
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      const uint8_t led = static_cast<uint8_t>(i + 2);
      const uint8_t bit = static_cast<uint8_t>(1U << i);
      if ((enabledMask & bit) == 0) {
        setPixel(led, 0, 0, 0);
      } else if ((openMask & bit) != 0) {
        setPixel(led, 48, 0, 0);
      } else {
        setPixel(led, 0, 48, 0);
      }
    }
    write();
  }

private:
  static constexpr rmt_channel_t kChannel = RMT_CHANNEL_0;
  static constexpr uint8_t kLedCount = 8;
  static constexpr uint16_t kT0h = 14;
  static constexpr uint16_t kT0l = 32;
  static constexpr uint16_t kT1h = 28;
  static constexpr uint16_t kT1l = 24;

  struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  void clear() {
    for (auto &pixel : pixels_) {
      pixel = {0, 0, 0};
    }
    write();
  }

  void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (index >= kLedCount) {
      return;
    }
    pixels_[index] = {r, g, b};
  }

  void write() {
    rmt_item32_t items[kLedCount * 24]{};
    size_t itemIndex = 0;
    for (const auto &pixel : pixels_) {
      writeByte(pixel.g, items, itemIndex);
      writeByte(pixel.r, items, itemIndex);
      writeByte(pixel.b, items, itemIndex);
    }
    rmt_write_items(kChannel, items, itemIndex, true);
    rmt_wait_tx_done(kChannel, pdMS_TO_TICKS(20));
    delayMicroseconds(80);
  }

  void writeByte(uint8_t value, rmt_item32_t *items, size_t &itemIndex) {
    for (int8_t bit = 7; bit >= 0; --bit) {
      const bool one = (value & (1U << bit)) != 0;
      items[itemIndex].level0 = 1;
      items[itemIndex].duration0 = one ? kT1h : kT0h;
      items[itemIndex].level1 = 0;
      items[itemIndex].duration1 = one ? kT1l : kT0l;
      ++itemIndex;
    }
  }

  int pin_{-1};
  Rgb pixels_[kLedCount]{};
};

}  // namespace shutup
