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
    writeRegister(0x06, 0x3F);  // Pull-ups for normally closed switches to ground.
  }

  uint8_t readOpenMask(uint8_t enabledMask) {
    uint8_t gpio = 0;
    if (!readRegister(0x09, gpio)) {
      return enabledMask;
    }
    return static_cast<uint8_t>(gpio & enabledMask & 0x3F);
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
