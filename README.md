# ShutUp

ShutUp contains two separate PlatformIO firmware projects:

- `Cab`: vehicle-cab device using a LilyGO T-Display S3.
- `Canopy`: canopy device using a TENSTAR ROBOT ESP32-C3 SuperMini Plus with external antenna connection and onboard RGB LED.

## Project Rules

- Hardware choices, GPIO assignments, and functionality are not assumed.
- Features are added only after discussion and explicit approval.
- Pin assignments must be checked against the exact physical hardware before any PCB or wiring decisions.
- Prior project attempts or chats are not a source of truth for this repository.

## PlatformIO Projects

### Cab

- Board: LilyGO T-Display S3
- PlatformIO board ID: `lilygo-t-display-s3`
- Framework: Arduino

### Canopy

- Physical board: TENSTAR ROBOT ESP32-C3 SuperMini Plus
- Board details: red ESP32-C3 SuperMini Plus style board with external antenna connector and onboard RGB LED
- PlatformIO board ID: `tenstar-esp32-c3-supermini-plus`
- PlatformIO board manifest: `Canopy/boards/tenstar-esp32-c3-supermini-plus.json`
- Framework: Arduino
- Confirmed onboard RGB LED pin: GPIO8

No application GPIO assignments have been made. The onboard RGB LED pin is documented as a board fact, not selected for project functionality.

## Hardware Requirements

These requirements are the current agreed target behavior and hardware list. GPIO assignments are not made in this section.

### Cab Requirements

Hardware:

- LilyGO T-Display S3.
- Mute button, possibly implemented as a capacitive touch button.
- Config button, physically small.
- Speaker or alarm sounder: Jaycar `XC3744`.
  - Powered from 5 V.
  - Connections: signal, 5 V input, ground.
  - Includes 23 mm speaker, 2 W amplifier, and volume trimpot.
  - Jaycar notes the volume should be adjusted to minimum before use to avoid speaker damage.
  - Jaycar notes this module has an onboard digital-to-analogue controller and is driven from a digital pin/library.

Config behavior:

- If the config button is detected as held during startup, the Cab device starts an open Wi-Fi configuration access point.
- Cab config SSID: `SHUTUP-CABCONF`
- Cab config AP password: none.
- The config AP presents a captive portal for Cab device configuration.

### Canopy Requirements

Hardware:

- TENSTAR ROBOT ESP32-C3 SuperMini Plus.
- RGB LED board: Jaycar `XC4380` RGB LED 8 pixel strip/board.
  - Connections: DIN, VCC, GND.
  - VCC range: 4-7 VDC.
  - Data input pin is reconfigurable in firmware.
  - Jaycar references the Adafruit NeoPixel library for control.
- Mute button.
- Config button.
- Speaker or alarm sounder: Jaycar `XC3744`.
  - Powered from 5 V.
  - Connections: signal, 5 V input, ground.
  - Includes 23 mm speaker, 2 W amplifier, and volume trimpot.
  - Jaycar notes the volume should be adjusted to minimum before use to avoid speaker damage.
  - Jaycar notes this module has an onboard digital-to-analogue controller and is driven from a digital pin/library.
- MCP23008 GPIO expander for normally closed reed-switch inputs.
- Up to 6 normally closed reed switches for monitored doors.

Config behavior:

- If the config button is detected as held during startup, the Canopy device starts an open Wi-Fi configuration access point.
- Canopy config SSID: `SHUTUP-CANCONF`
- Canopy config AP password: none.
- The config AP presents a captive portal for Canopy device configuration.

Canopy LED behavior:

- LED 1: power/status.
- LED 2: heartbeat/connection state when the Cab device is connected.
- LEDs 3-8: six monitored door states.

### Communication Model

- ESP-NOW is the intended main communication path after the Cab and Canopy devices are paired through their configuration pages.
- The Canopy device is always powered and acts as the persistent state holder.
- The Cab device queries or receives the current Canopy state over ESP-NOW, conceptually similar to requesting `/state` in an HTTP system, but without using Wi-Fi/HTTP for normal operation.

## Pin Reference

Legend: `✓` means the pin has that confirmed property. A blank cell means that property is not confirmed or does not apply from the current sources.

The pinout tables only list exposed pins.

Sources used for this first pin reference:

- LilyGO T-Display-S3 official pin map: https://raw.githubusercontent.com/Xinyuan-LilyGO/T-Display-S3/main/image/T-DISPLAY-S3.jpg
- LilyGO T-Display-S3 README and hardware notes: https://github.com/Xinyuan-LilyGO/T-Display-S3
- TENSTAR ROBOT ESP32-C3 SuperMini Plus public reference: https://www.espboards.dev/esp32/esp32-c3-super-mini-plus/
- Jaycar XC3744 datasheet: https://media.jaycar.com.au/product/resources/XC3744_datasheetMain_112903.pdf
- Jaycar XC4380 manual: https://media.jaycar.com.au/product/resources/XC4380_manualMain_78735.pdf
- Espressif ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- Espressif ESP32-C3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf

### Cab Pinout - LilyGO T-Display S3

| Pin | Label | GPIO | 3V3 | 5V | GND | ADC | TCH | STR | SPI | UART | CLK |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| P2-1 | 3V | - | ✓ |  |  |  |  |  |  |  |  |
| P2-2 | 1 | GPIO1 |  |  |  | ✓ | ✓ |  |  |  |  |
| P2-3 | 2 | GPIO2 |  |  |  | ✓ | ✓ |  |  |  |  |
| P2-4 | 3 | GPIO3 |  |  |  | ✓ | ✓ | ✓ |  |  |  |
| P2-5 | 10 | GPIO10 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| P2-6 | 11 | GPIO11 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| P2-7 | 12 | GPIO12 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| P2-8 | 13 | GPIO13 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| P2-9 | NC | - |  |  |  |  |  |  |  |  |  |
| P2-10 | NC | - |  |  |  |  |  |  |  |  |  |
| P2-11 | GND | - |  |  | ✓ |  |  |  |  |  |  |
| P2-12 | +5V | - |  | ✓ |  |  |  |  |  |  |  |
| P1-1 | GND | - |  |  | ✓ |  |  |  |  |  |  |
| P1-2 | GND | - |  |  | ✓ |  |  |  |  |  |  |
| P1-3 | 43 | GPIO43 |  |  |  |  |  |  |  | ✓ | ✓ |
| P1-4 | 44 | GPIO44 |  |  |  |  |  |  |  | ✓ | ✓ |
| P1-5 | 18 | GPIO18 |  |  |  | ✓ |  |  |  | ✓ |  |
| P1-6 | 17 | GPIO17 |  |  |  | ✓ |  |  |  | ✓ |  |
| P1-7 | 21 | GPIO21 |  |  |  |  |  |  |  |  |  |
| P1-8 | 16 | GPIO16 |  |  |  | ✓ |  |  |  |  |  |
| P1-9 | NC | - |  |  |  |  |  |  |  |  |  |
| P1-10 | GND | - |  |  | ✓ |  |  |  |  |  |  |
| P1-11 | GND | - |  |  | ✓ |  |  |  |  |  |  |
| P1-12 | 3V | - | ✓ |  |  |  |  |  |  |  |  |

### Cab Pin Notes

| GPIO | Pin | Notes |
| --- | --- | --- |
| - | P2-1 | 3.3 V rail. |
| GPIO1 | P2-2 | ADC1_CH0, TOUCH1. |
| GPIO2 | P2-3 | ADC1_CH1, TOUCH2. |
| GPIO3 | P2-4 | ADC1_CH2, TOUCH3, ESP32-S3 strapping pin. Avoid external circuits that force a reset-time state. |
| GPIO10 | P2-5 | ADC1_CH9, TOUCH10, SPI_CS. |
| GPIO11 | P2-6 | ADC2_CH0, TOUCH11, SPI_D. |
| GPIO12 | P2-7 | ADC2_CH1, TOUCH12, SPI_CLK. |
| GPIO13 | P2-8 | ADC2_CH2, TOUCH13, SPI_Q. |
| - | P2-9 | Not connected. |
| - | P2-10 | Not connected. |
| - | P2-11 | Ground. |
| - | P2-12 | 5 V rail. |
| - | P1-1 | Ground. |
| - | P1-2 | Ground. |
| GPIO43 | P1-3 | UART0 TX / CLK_OUT1; used for serial when USB CDC is disabled. |
| GPIO44 | P1-4 | UART0 RX / CLK_OUT2; used for serial when USB CDC is disabled. |
| GPIO18 | P1-5 | ADC2_CH7, U1_RXD. |
| GPIO17 | P1-6 | ADC2_CH6, U1_TXD. |
| GPIO21 | P1-7 | Exposed GPIO. |
| GPIO16 | P1-8 | ADC2_CH5. |
| - | P1-9 | Not connected. |
| - | P1-10 | Ground. |
| - | P1-11 | Ground. |
| - | P1-12 | 3.3 V rail. |

### Canopy Pinout - TENSTAR ROBOT ESP32-C3 SuperMini Plus

| Pin | Label | GPIO | 3V3 | 5V | GND | ADC | STR | LED | SPI | I2C | UART | JTAG |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 5V | - |  | ✓ |  |  |  |  |  |  |  |  |
| 2 | GND | - |  |  | ✓ |  |  |  |  |  |  |  |
| 3 | 3V3 | - | ✓ |  |  |  |  |  |  |  |  |  |
| 4 | IO0/A0 | GPIO0 |  |  |  | ✓ |  |  |  |  |  |  |
| 5 | IO1/A1 | GPIO1 |  |  |  | ✓ |  |  |  |  |  |  |
| 6 | IO2/A2 | GPIO2 |  |  |  | ✓ | ✓ |  |  |  |  |  |
| 7 | IO3/A3 | GPIO3 |  |  |  | ✓ |  |  |  |  |  |  |
| 8 | IO4/SCK | GPIO4 |  |  |  | ✓ |  |  | ✓ |  |  | ✓ |
| 9 | IO5/MISO | GPIO5 |  |  |  | ✓ |  |  | ✓ |  |  | ✓ |
| 10 | IO6/MOSI | GPIO6 |  |  |  |  |  |  | ✓ |  |  | ✓ |
| 11 | IO7/SS | GPIO7 |  |  |  |  |  |  | ✓ |  |  | ✓ |
| 12 | IO8/SDA | GPIO8 |  |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| 13 | IO9/SCL | GPIO9 |  |  |  |  | ✓ |  |  | ✓ |  |  |
| 14 | IO10/RX | GPIO10 |  |  |  |  |  |  |  |  | ✓ |  |
| 15 | IO21/TX | GPIO21 |  |  |  |  |  |  |  |  | ✓ |  |
| 16 | IO20/RX | GPIO20 |  |  |  |  |  |  |  |  | ✓ |  |

### Canopy Pin Notes

| GPIO | Pin | Notes |
| --- | --- | --- |
| - | 1 | 5 V input. |
| - | 2 | Ground. |
| - | 3 | 3.3 V output. |
| GPIO0 | 4 | ADC-capable GPIO. |
| GPIO1 | 5 | ADC-capable GPIO. |
| GPIO2 | 6 | ADC-capable GPIO; ESP32-C3 strapping pin. Avoid external circuits that force a reset-time state. |
| GPIO3 | 7 | ADC-capable GPIO. |
| GPIO4 | 8 | ADC-capable, SPI SCK, JTAG MTMS caution. |
| GPIO5 | 9 | ADC-capable, SPI MISO, JTAG MTDI caution. |
| GPIO6 | 10 | SPI MOSI, JTAG MTCK caution. |
| GPIO7 | 11 | SPI SS, JTAG MTDO caution. |
| GPIO8 | 12 | Onboard RGB LED / `RGB_BUILTIN`, I2C SDA label, ESP32-C3 strapping pin. Do not treat as ordinary I2C/data if using the onboard RGB LED. |
| GPIO9 | 13 | I2C SCL label, ESP32-C3 boot-mode strapping pin. Download/boot mode is controlled by GPIO9 being LOW at reset. |
| GPIO10 | 14 | Board-labeled RX. |
| GPIO21 | 15 | UART0 TX. |
| GPIO20 | 16 | UART0 RX. |

## Build

From the repository root:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Cab
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Canopy
```
