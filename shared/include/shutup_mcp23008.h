#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "shutup_settings.h"

namespace shutup {

class Mcp23008DoorReader {
public:
  void begin(TwoWire &wire, uint8_t address = 0x20) {
    wire_ = &wire;
    address_ = address;
    writeRegister(0x00, 0x3F);  // GPA0-GPA5 inputs, GPA6-GPA7 unused outputs.
    writeRegister(0x06, 0x3F);  // Pull-ups for normally open switch-to-ground inputs.
  }

  void readDoorStates(uint8_t enabledMask, DoorState states[kDoorCount]) {
    uint8_t gpio = 0;
    if (!readRegister(0x09, gpio)) {
      for (uint8_t i = 0; i < kDoorCount; ++i) {
        states[i] = (enabledMask & (1U << i)) != 0 ? DoorState::Open : DoorState::Disabled;
      }
      return;
    }
    for (uint8_t i = 0; i < kDoorCount; ++i) {
      const uint8_t bit = static_cast<uint8_t>(1U << i);
      if ((enabledMask & bit) == 0) {
        states[i] = DoorState::Disabled;
      } else {
        states[i] = (gpio & bit) == 0 ? DoorState::Closed : DoorState::Open;
      }
    }
  }

private:
  bool writeRegister(uint8_t reg, uint8_t value) {
    if (!wire_) {
      return false;
    }
    wire_->beginTransmission(address_);
    wire_->write(reg);
    wire_->write(value);
    return wire_->endTransmission() == 0;
  }

  bool readRegister(uint8_t reg, uint8_t &value) {
    if (!wire_) {
      return false;
    }
    wire_->beginTransmission(address_);
    wire_->write(reg);
    if (wire_->endTransmission(false) != 0) {
      return false;
    }
    if (wire_->requestFrom(static_cast<int>(address_), 1) != 1) {
      return false;
    }
    value = wire_->read();
    return true;
  }

  TwoWire *wire_{nullptr};
  uint8_t address_{0x20};
};

}  // namespace shutup
