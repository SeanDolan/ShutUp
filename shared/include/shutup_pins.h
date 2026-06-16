#pragma once

namespace shutup {

constexpr int kUnassignedPin = -1;

inline bool bootButtonHeld(int gpio, bool activeLow = true) {
  if (gpio < 0) {
    return false;
  }
  pinMode(gpio, activeLow ? INPUT_PULLUP : INPUT_PULLDOWN);
  delay(50);
  const int state = digitalRead(gpio);
  return activeLow ? state == LOW : state == HIGH;
}

}  // namespace shutup
