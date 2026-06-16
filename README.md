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

Sources used for this first pin reference:

- LilyGO T-Display-S3 official pin map: https://raw.githubusercontent.com/Xinyuan-LilyGO/T-Display-S3/main/image/T-DISPLAY-S3.jpg
- LilyGO T-Display-S3 README and hardware notes: https://github.com/Xinyuan-LilyGO/T-Display-S3
- TENSTAR ROBOT ESP32-C3 SuperMini Plus public reference: https://www.espboards.dev/esp32/esp32-c3-super-mini-plus/
- Jaycar XC3744 datasheet: https://media.jaycar.com.au/product/resources/XC3744_datasheetMain_112903.pdf
- Jaycar XC4380 manual: https://media.jaycar.com.au/product/resources/XC4380_manualMain_78735.pdf
- Espressif ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- Espressif ESP32-C3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf

### Cab External Headers - LilyGO T-Display S3

These are the exposed header pins only. Display, battery, button, and power-control GPIOs are listed separately below.

| Physical pin | Board label | GPIO | 3V3 | 5V | GND | ADC | Touch | Strapping | SPI | UART | Clock out | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| P2-1 left top | 3V | - | ✓ |  |  |  |  |  |  |  |  | 3.3 V rail |
| P2-2 | 1 | GPIO1 |  |  |  | ✓ | ✓ |  |  |  |  | ADC1_CH0, TOUCH1 |
| P2-3 | 2 | GPIO2 |  |  |  | ✓ | ✓ |  |  |  |  | ADC1_CH1, TOUCH2 |
| P2-4 | 3 | GPIO3 |  |  |  | ✓ | ✓ | ✓ |  |  |  | ADC1_CH2, TOUCH3; ESP32-S3 strapping pin |
| P2-5 | 10 | GPIO10 |  |  |  | ✓ | ✓ |  | ✓ |  |  | ADC1_CH9, TOUCH10, SPI_CS |
| P2-6 | 11 | GPIO11 |  |  |  | ✓ | ✓ |  | ✓ |  |  | ADC2_CH0, TOUCH11, SPI_D |
| P2-7 | 12 | GPIO12 |  |  |  | ✓ | ✓ |  | ✓ |  |  | ADC2_CH1, TOUCH12, SPI_CLK |
| P2-8 | 13 | GPIO13 |  |  |  | ✓ | ✓ |  | ✓ |  |  | ADC2_CH2, TOUCH13, SPI_Q |
| P2-9 | NC | - |  |  |  |  |  |  |  |  |  | Not connected |
| P2-10 | NC | - |  |  |  |  |  |  |  |  |  | Not connected |
| P2-11 | GND | - |  |  | ✓ |  |  |  |  |  |  | Ground |
| P2-12 left bottom | +5V | - |  | ✓ |  |  |  |  |  |  |  | 5 V rail |
| P1-1 right top | GND | - |  |  | ✓ |  |  |  |  |  |  | Ground |
| P1-2 | GND | - |  |  | ✓ |  |  |  |  |  |  | Ground |
| P1-3 | 43 | GPIO43 |  |  |  |  |  |  |  | ✓ | ✓ | UART0 TX / CLK_OUT1; used for serial when USB CDC is disabled |
| P1-4 | 44 | GPIO44 |  |  |  |  |  |  |  | ✓ | ✓ | UART0 RX / CLK_OUT2; used for serial when USB CDC is disabled |
| P1-5 | 18 | GPIO18 |  |  |  | ✓ |  |  |  | ✓ |  | ADC2_CH7, U1_RXD |
| P1-6 | 17 | GPIO17 |  |  |  | ✓ |  |  |  | ✓ |  | ADC2_CH6, U1_TXD |
| P1-7 | 21 | GPIO21 |  |  |  |  |  |  |  |  |  | Exposed GPIO |
| P1-8 | 16 | GPIO16 |  |  |  | ✓ |  |  |  |  |  | ADC2_CH5 |
| P1-9 | NC | - |  |  |  |  |  |  |  |  |  | Not connected |
| P1-10 | GND | - |  |  | ✓ |  |  |  |  |  |  | Ground |
| P1-11 | GND | - |  |  | ✓ |  |  |  |  |  |  | Ground |
| P1-12 right bottom | 3V | - | ✓ |  |  |  |  |  |  |  |  | 3.3 V rail |

### Cab Onboard Connections - LilyGO T-Display S3

These GPIOs are already connected to board hardware. They should not be treated as free external GPIOs.

| Physical / board function | GPIO | ADC | Strapping | Display | Button | Power / battery | LED / indicator | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| LCD power enable | GPIO15 |  |  | ✓ |  | ✓ | ✓ | Must be driven HIGH for display/backlight power when battery powered; also controls V3V output/indicator behavior |
| LCD backlight | GPIO38 |  |  | ✓ |  |  |  | `LCD_BL` |
| LCD D0 | GPIO39 |  |  | ✓ |  |  |  | Display data line |
| LCD D1 | GPIO40 |  |  | ✓ |  |  |  | Display data line |
| LCD D2 | GPIO41 |  |  | ✓ |  |  |  | Display data line |
| LCD D3 | GPIO42 |  |  | ✓ |  |  |  | Display data line |
| LCD D4 | GPIO45 |  | ✓ | ✓ |  |  |  | Display data line; ESP32-S3 strapping pin |
| LCD D5 | GPIO46 |  | ✓ | ✓ |  |  |  | Display data line; ESP32-S3 strapping pin |
| LCD D6 | GPIO47 |  |  | ✓ |  |  |  | Display data line |
| LCD D7 | GPIO48 |  |  | ✓ |  |  |  | Display data line |
| LCD WR | GPIO8 |  |  | ✓ |  |  |  | Display write control |
| LCD RD | GPIO9 |  |  | ✓ |  |  |  | Display read control |
| LCD DC | GPIO7 |  |  | ✓ |  |  |  | Display data/command |
| LCD CS | GPIO6 |  |  | ✓ |  |  |  | Display chip select |
| LCD reset | GPIO5 |  |  | ✓ |  |  |  | Display reset |
| Battery voltage sense | GPIO4 | ✓ |  |  |  | ✓ |  | `LCD_BAT_VOLT`; ADC input through board circuitry |
| BOOT button | GPIO0 |  | ✓ |  | ✓ |  |  | Download/boot button; ESP32-S3 boot strapping pin |
| IO14 button | GPIO14 |  |  |  | ✓ |  |  | Second front button shown on LilyGO pin map as `IO14` |
| Reset button | - |  |  |  | ✓ |  |  | Hardware reset, no GPIO assignment |
| Charge LED | - |  |  |  |  |  | ✓ | Red charge indicator, not GPIO-controlled |

### Canopy External Pins - TENSTAR ROBOT ESP32-C3 SuperMini Plus

This is the red SuperMini Plus style board with external antenna connector and onboard RGB LED.

| Physical pin | Board label | GPIO | 3V3 | 5V | GND | ADC | Strapping | Onboard LED | SPI | I2C | UART | JTAG | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 5V | - |  | ✓ |  |  |  |  |  |  |  |  | 5 V input |
| 2 | GND | - |  |  | ✓ |  |  |  |  |  |  |  | Ground |
| 3 | 3V3 | - | ✓ |  |  |  |  |  |  |  |  |  | 3.3 V output |
| 4 | IO0 / A0 | GPIO0 |  |  |  | ✓ |  |  |  |  |  |  | ADC-capable GPIO |
| 5 | IO1 / A1 | GPIO1 |  |  |  | ✓ |  |  |  |  |  |  | ADC-capable GPIO |
| 6 | IO2 / A2 | GPIO2 |  |  |  | ✓ | ✓ |  |  |  |  |  | ESP32-C3 strapping pin |
| 7 | IO3 / A3 | GPIO3 |  |  |  | ✓ |  |  |  |  |  |  | ADC-capable GPIO |
| 8 | IO4 / A4 / SCK | GPIO4 |  |  |  | ✓ |  |  | ✓ |  |  | ✓ | ADC-capable; SPI SCK; JTAG MTMS caution |
| 9 | IO5 / A5 / MISO | GPIO5 |  |  |  | ✓ |  |  | ✓ |  |  | ✓ | ADC-capable; SPI MISO; JTAG MTDI caution |
| 10 | IO6 / MOSI | GPIO6 |  |  |  |  |  |  | ✓ |  |  | ✓ | SPI MOSI; JTAG MTCK caution |
| 11 | IO7 / SS | GPIO7 |  |  |  |  |  |  | ✓ |  |  | ✓ | SPI SS; JTAG MTDO caution |
| 12 | IO8 / SDA | GPIO8 |  |  |  |  | ✓ | ✓ |  | ✓ |  |  | Onboard RGB LED / `RGB_BUILTIN`; ESP32-C3 strapping pin |
| 13 | IO9 / SCL | GPIO9 |  |  |  |  | ✓ |  |  | ✓ |  |  | ESP32-C3 boot-mode strapping pin |
| 14 | IO10 / RX | GPIO10 |  |  |  |  |  |  |  |  | ✓ |  | Board-labeled RX |
| 15 | IO21 / TX | GPIO21 |  |  |  |  |  |  |  |  | ✓ |  | UART0 TX |
| 16 | IO20 / RX | GPIO20 |  |  |  |  |  |  |  |  | ✓ |  | UART0 RX |

### Canopy Onboard Notes - TENSTAR ROBOT ESP32-C3 SuperMini Plus

| Physical / board function | GPIO | Strapping | Onboard LED | Power / antenna | Notes |
| --- | --- | --- | --- | --- | --- |
| RGB LED | GPIO8 | ✓ | ✓ |  | Shared with IO8/SDA; do not also treat GPIO8 as an ordinary I2C/data pin if using the RGB LED |
| Power LED | - |  | ✓ | ✓ | Power indicator, not GPIO-controlled |
| External antenna connector | - |  |  | ✓ | External antenna connector is present; verify any onboard jumper/solder-link requirement before relying on the external antenna |
| BOOT function | GPIO9 | ✓ |  |  | ESP32-C3 download/boot mode is controlled by GPIO9 being LOW at reset |

## Build

From the repository root:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Cab
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Canopy
```
