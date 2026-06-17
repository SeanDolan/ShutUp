# ShutUp Wiring Specification

This document is the working wiring reference for the current firmware and hardware plan. Do not treat the MCP23008 door-input section as PCB-final until the cable-fault behavior is resolved.

## Shared Rules

- All connected modules must share a common ground with the ESP32 board they connect to.
- ESP32 GPIO signals are 3.3 V logic.
- Do not feed 5 V logic into ESP32 GPIO pins.
- Button inputs are currently active-low in firmware: the input pin is pulled up and the button shorts the input to GND when pressed.

## Cab Device

Board: LilyGO T-Display S3.

| Function | Board pin | GPIO | Wiring | Firmware behavior |
| --- | --- | --- | --- | --- |
| Pairing button | `2` | GPIO2 | Momentary button from GPIO2 to GND. | Held during power-up enters pairing-listen mode and displays `pairing.png`. |
| Mute button | `1` | GPIO1 | Momentary button from GPIO1 to GND. | Pressed state mutes the current alarm condition. |
| Speaker signal | `13` | GPIO13 | GPIO13 to XC3744 signal input. | Non-blocking tone output. |

Cab XC3744 speaker module:

| XC3744 pin | Connects to |
| --- | --- |
| Signal | Cab GPIO13 |
| 5 V | Cab 5 V rail |
| GND | Cab GND |

## Canopy Device

Board: TENSTAR ROBOT ESP32-C3 SuperMini Plus.

| Function | Board pin | GPIO | Wiring | Firmware behavior |
| --- | --- | --- | --- | --- |
| Config button | `IO0/A0` | GPIO0 | Momentary button from GPIO0 to GND. | Held during power-up starts the `SHUTUP-CONFIG` access point and config portal. |
| Mute button | `IO1/A1` | GPIO1 | Momentary button from GPIO1 to GND. | Pressed state mutes the current alarm condition. |
| WS2812S data | `IO3/A3` | GPIO3 | GPIO3 to WS2812S DIN. | Drives the 8 LED status board. |
| MCP23008 SDA | `IO20/RX` | GPIO20 | GPIO20 to MCP23008 SDA. | Remapped I2C SDA. |
| MCP23008 SCL | `IO21/TX` | GPIO21 | GPIO21 to MCP23008 SCL. | Remapped I2C SCL. |
| Speaker signal | `IO10/RX` | GPIO10 | GPIO10 to XC3744 signal input. | Non-blocking tone output. |

Canopy XC3744 speaker module:

| XC3744 pin | Connects to |
| --- | --- |
| Signal | Canopy GPIO10 |
| 5 V | Canopy 5 V rail |
| GND | Canopy GND |

Canopy XC4380 / WS2812S 8 LED board:

| LED board pin | Connects to |
| --- | --- |
| DIN | Canopy GPIO3 |
| VCC | 5 V rail |
| GND | Canopy GND |

## Canopy MCP23008

The current firmware uses one MCP23008 at I2C address `0x20`.

| MCP23008 pin/function | Connects to | Notes |
| --- | --- | --- |
| VDD | 3.3 V | Keeps I2C and GPIO signals at ESP32-safe 3.3 V levels. |
| VSS | GND | Common ground with the Canopy ESP32-C3 board. |
| SDA | Canopy GPIO20 | I2C data. |
| SCL | Canopy GPIO21 | I2C clock. |
| A0 | GND | Address `0x20`. |
| A1 | GND | Address `0x20`. |
| A2 | GND | Address `0x20`. |
| RESET | 3.3 V | Hold high for normal operation. |
| GPA0 | Door sensor 1 input | Firmware enables the internal pull-up. |
| GPA1 | Door sensor 2 input | Firmware enables the internal pull-up. |
| GPA2 | Door sensor 3 input | Firmware enables the internal pull-up. |
| GPA3 | Door sensor 4 input | Firmware enables the internal pull-up. |
| GPA4 | Door sensor 5 input | Firmware enables the internal pull-up. |
| GPA5 | Door sensor 6 input | Firmware enables the internal pull-up. |
| GPA6 | Unused | Firmware configures this as an unused output. |
| GPA7 | Unused | Firmware configures this as an unused output. |
| INT | Not used by current firmware | No interrupt input is currently assigned. |

MCP23008 PCB requirements:

- Place one `100 nF` ceramic decoupling capacitor between MCP23008 VDD and VSS, physically close to the MCP23008 power pins.
- Provide I2C pull-ups from SDA to 3.3 V and SCL to 3.3 V unless the final PCB design confirms they already exist elsewhere on the same bus.
- If pull-ups are added on the ShutUp PCB, use `4.7 kOhm` as the starting value for both SDA and SCL.
- Do not power the MCP23008 from 5 V unless I2C level shifting is also added and approved.

Current MCP23008 firmware behavior:

- GPA0-GPA5 are configured as inputs.
- Internal MCP23008 pull-ups are enabled on GPA0-GPA5.
- The firmware records only physical door state: `open`, `closed`, or `disabled`.
- A LOW read on GPA0-GPA5 is currently recorded as physical door `open`.
- A HIGH read on GPA0-GPA5 is currently recorded as physical door `closed`.
- If the MCP23008 cannot be read over I2C, every enabled door is recorded as physical door `open`.

Current passive reed-switch wiring implied by the firmware:

| Reed wire | Connects to |
| --- | --- |
| One side | MCP23008 GPA input |
| Other side | GND |

Critical PCB decision:

- With the current passive switch-to-ground wiring and current firmware mapping, an open-circuit cable fault reads the same as a physical door `closed`.
- This does not satisfy the intended safety behavior where a broken cable should alert as physical door `open`.
- Before PCB fabrication, the final reed-switch wiring and firmware mapping must be resolved together so the healthy closed-door state and the cable-fault state cannot be confused.

