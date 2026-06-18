# ShutUp

ShutUp contains two separate PlatformIO firmware projects:

- `Cab`: vehicle-cab device using a LilyGO T-Display S3.
- `Canopy`: canopy device using a TENSTAR ROBOT ESP32-C3 SuperMini Plus with external antenna connection and onboard RGB LED.

## AI Development Limitations

- Do not assume hardware choices, GPIO assignments, project functionality, or future expansion needs.
- Do not add features, functionality, project behavior, libraries, hardware capacity, or "just in case" options without discussion and explicit approval.
- Do not use generic device information when exact hardware details are required.
- If required information is unknown and blocks progress, stop and ask.
- Pin assignments must be checked against the exact physical hardware before any PCB or wiring decisions.
- Prior project attempts or chats are tainted and must not be used, scanned, summarized, or treated as a source of truth for this repository.
- Keep the project scope under user control; document requirements as requirements, not as implemented features or implied commitments.
- When the user specifically asks for a code fix, bulk update, feature add/modify, or documentation update, complete the full local change, commit, and push workflow.
- Do not commit or push speculative work, exploratory edits, or unapproved scope changes.

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

Project GPIO assignments are listed in the selected pin assignments section below.

Detailed wiring notes are in [docs/wiring.md](docs/wiring.md).

## Current Firmware Status

- Both devices have startup branching for config/pairing mode vs normal ESP-NOW mode.
- Config/pairing mode is entered when the relevant device button is held at boot.
- The physical config-button GPIOs are assigned in each device `main.cpp`.
- Cab pairing-button boot mode does not start a Wi-Fi access point, a web server, or a captive portal.
- Cab pairing-button boot mode displays `pairing.png` and waits for pairing to be initiated from the Canopy config page.
- Canopy config AP SSID: `SHUTUP-CONFIG`, open network.
- Canopy config mode starts an HTTP config page and DNS captive-portal responder.
- ESP-NOW pairing scaffolding is implemented:
  - Put the Cab into pairing-listen mode by booting it with its pairing button held.
  - Put the Canopy into config mode by booting it with its config button held.
  - Connect to the Canopy config AP.
  - Press `Pair Devices` on the Canopy config page.
  - The Canopy broadcasts a pairing request.
  - The Cab responds without running a Wi-Fi server or config page.
  - The peer MAC is stored in non-volatile preferences.
  - After pairing completes, both devices reboot to leave config/pairing mode cleanly.
- Normal mode initializes ESP-NOW:
  - Canopy acts as the always-powered state holder.
  - Cab acts as the state client and periodically requests Canopy state.
- Cab blacks out the T-Display-S3 screen immediately at startup, then shows config mode or normal status screens.
- Cab display assets are treated as portrait `170x320`; overlay `X` and `Y` positions are measured from the top-left of that portrait image as `(0,0)`.
- In normal startup, the Cab shows `startup.png` while ESP-NOW initializes, then shows `connecting.png` until it has received both its first heartbeat response and the Canopy config snapshot.
- Cab sends a non-blocking ESP-NOW heartbeat request every 1 second; Canopy replies with a six-entry physical door-state array.
- Cab connection health uses the success rate of the most recent 5 heartbeat attempts and average round-trip time from the last 5 successful heartbeats.
- The Cab normal display is region-updated: the ute background is drawn once, and LCD pixels are written only when a door state, mute state, heartbeat bar count, ute colour, or overlay configuration changes.
- Door changes redraw only that configured door rectangle. Mute and heartbeat changes redraw only their black indicator regions.
- The first heartbeat requests one compact Cab config snapshot containing Cab sound settings, door enabled states, all six overlay geometry/colour settings, and ute colour.
- Cab acknowledges the applied config revision in later heartbeat requests. Canopy sends no further config while the revisions match.
- Saving Canopy config increments its persistent config revision; Canopy sends one updated snapshot on the next heartbeat and retries only until the Cab acknowledges that revision.
- Canopy reads physical door state through MCP23008 GPA0-GPA5 and reports enabled door states over ESP-NOW.
- Canopy drives the 8 WS2812S LEDs as power, connectivity, and six door state indicators.
- Canopy config includes the action sound table for Startup, Connectivity success, Connectivity error, Doors ok, and Door alarm.
- In the action sound table, `Repeat` is an on/off checkbox and `Delay` is the delay between repeats.
- Sound dropdowns are generated from the shared tone library in `shared/include/shutup_sounds.h`.
- Cab images are stored in `Cab/data/images/` as `startup.png`, `connecting.png`, `pairing.png`, and `ute_<colour>.png` for black, white, gray, red, blue, green, and yellow.
- Canopy config owns the Cab door overlay rectangle settings and syncs them to the Cab over ESP-NOW.

## Selected Pin Assignments

These assignments avoid known boot strapping pins, onboard display pins, onboard RGB LED pins, and JTAG pins.

### Cab Selected Pins

| Function | GPIO | Board label | Reason |
| --- | --- | --- | --- |
| Pairing button | GPIO2 | `2` | Exposed ADC/touch-capable GPIO, not marked as strapping, display, touch-controller reset/init, battery sense, onboard button, UART, I2C, or SPI. Safe for held-at-boot input. |
| Mute input | GPIO1 | `1` | Exposed ADC/touch-capable pin, not marked as a strapping pin. Currently coded as an active-low input. |
| Speaker signal | GPIO13 | `13` | Exposed output-capable pin, not marked as strapping, display, touch-controller reset/init, battery sense, onboard button, UART, or I2C. |

### Canopy Selected Pins

| Function | GPIO | Board label | Reason |
| --- | --- | --- | --- |
| Config button | GPIO0 | `IO0/A0` | Exposed ADC-capable GPIO, not marked as ESP32-C3 strapping, onboard LED, SPI, I2C, UART, or JTAG. Safe for held-at-boot input. |
| Mute button | GPIO1 | `IO1/A1` | Exposed ADC-capable GPIO, not marked as ESP32-C3 strapping, onboard LED, SPI, I2C, UART, or JTAG. |
| WS2812S data | GPIO3 | `IO3/A3` | Exposed GPIO, not marked as strapping, onboard LED, SPI, I2C, UART, or JTAG. |
| MCP23008 SDA | GPIO20 | `IO20/RX` | Exposed GPIO selected for remapped I2C SDA. Avoids strapping pins GPIO8/GPIO9 and avoids JTAG pins GPIO4-GPIO7. Native USB CDC is enabled for serial logging so GPIO20 is not needed for normal serial. |
| MCP23008 SCL | GPIO21 | `IO21/TX` | Exposed GPIO selected for remapped I2C SCL. Avoids strapping pins GPIO8/GPIO9 and avoids JTAG pins GPIO4-GPIO7. Native USB CDC is enabled for serial logging so GPIO21 is not needed for normal serial. |
| Speaker signal | GPIO10 | `IO10/RX` | Exposed GPIO, not marked as strapping, onboard LED, SPI, I2C, or JTAG. |

MCP23008 wiring summary:

- Full wiring details are in [docs/wiring.md](docs/wiring.md).
- MCP23008 address is currently `0x20`; A0, A1, and A2 are tied low.
- Door sensor 1-6 currently map to MCP23008 GPA0-GPA5 in order.
- The current firmware records physical door state only: `open`, `closed`, or `disabled`.
- The current firmware maps LOW to physical door `closed` and HIGH to physical door `open`.
- With normally open reed switches wired from MCP23008 input to GND, a cable break reads the same as physical door `open`.

## Config Page Demos

The real Canopy config page is stored in `shared/web/` and is the source of truth:

- `shared/web/config_can.html`

The firmware header and demo file are generated from that source page:

- `shared/generated/web_assets.h`
- `demo/config_can.html`

Run this after editing files in `shared/web/`:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\python.exe' tools\generate_web_assets.py
```

PlatformIO also runs the generator before each device build.

The generator also scans `shared/include/shutup_sounds.h` and exposes the named tone sounds to the config page dropdowns.

The current speaker implementation stores and syncs selected tone names and plays them as non-blocking square-wave tone sequences on the XC3744 signal pin.

## Hardware Requirements

These requirements are the current agreed target behavior and hardware list.

### Cab Requirements

Hardware:

- LilyGO T-Display S3.
- Mute button, possibly implemented as a capacitive touch button.
- Pairing button, physically small.
- Speaker or alarm sounder: Jaycar `XC3744`.
  - Powered from 5 V.
  - Connections: signal, 5 V input, ground.
  - Includes 23 mm speaker, 2 W amplifier, and volume trimpot.
  - Jaycar notes the volume should be adjusted to minimum before use to avoid speaker damage.
  - Jaycar notes this module has an onboard digital-to-analogue controller and is driven from a digital pin/library.

Config behavior:

- If the pairing button is detected as held during startup, the Cab device enters pairing-listen mode.
- Cab pairing-listen mode displays `pairing.png`.
- Cab pairing-listen mode does not start a Wi-Fi access point, a web server, or a captive portal.
- Pairing is initiated from the Canopy config page.

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
- MCP23008 GPIO expander for normally open reed-switch inputs.
- Up to 6 normally open reed switches for monitored doors.

Config behavior:

- If the config button is detected as held during startup, the Canopy device starts an open Wi-Fi configuration access point.
- Canopy config SSID: `SHUTUP-CONFIG`
- Canopy config AP password: none.
- The config AP presents a captive portal for Canopy device configuration.

Canopy LED behavior:

- LED 1: green when powered/running.
- LED 2: red when Cab is disconnected, orange when Cab connectivity is stale/weak, blue when Cab is connected.
- LEDs 3-8: red when the matching door is open/error, green when closed, off when that door sensor is disabled.

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
- LilyGO T-Display-S3 factory LCD setup reference: https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/example/factory/factory.ino
- TENSTAR ROBOT ESP32-C3 SuperMini Plus public reference: https://www.espboards.dev/esp32/esp32-c3-super-mini-plus/
- Jaycar XC3744 datasheet: https://media.jaycar.com.au/product/resources/XC3744_datasheetMain_112903.pdf
- Jaycar XC4380 manual: https://media.jaycar.com.au/product/resources/XC4380_manualMain_78735.pdf
- Espressif ESP32-S3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- Espressif ESP32-C3 datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf

### Cab Pinout - LilyGO T-Display S3

| Pin | GPIO | 3V3 | 5V | GND | ADC | TCH | STR | SPI | UART | CLK |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 3V | - | ✓ |  |  |  |  |  |  |  |  |
| 1 | GPIO1 |  |  |  | ✓ | ✓ |  |  |  |  |
| 2 | GPIO2 |  |  |  | ✓ | ✓ |  |  |  |  |
| 3 | GPIO3 |  |  |  | ✓ | ✓ | ✓ |  |  |  |
| 10 | GPIO10 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| 11 | GPIO11 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| 12 | GPIO12 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| 13 | GPIO13 |  |  |  | ✓ | ✓ |  | ✓ |  |  |
| NC | - |  |  |  |  |  |  |  |  |  |
| NC | - |  |  |  |  |  |  |  |  |  |
| GND | - |  |  | ✓ |  |  |  |  |  |  |
| +5V | - |  | ✓ |  |  |  |  |  |  |  |
| GND | - |  |  | ✓ |  |  |  |  |  |  |
| GND | - |  |  | ✓ |  |  |  |  |  |  |
| 43 | GPIO43 |  |  |  |  |  |  |  | ✓ | ✓ |
| 44 | GPIO44 |  |  |  |  |  |  |  | ✓ | ✓ |
| 18 | GPIO18 |  |  |  | ✓ |  |  |  | ✓ |  |
| 17 | GPIO17 |  |  |  | ✓ |  |  |  | ✓ |  |
| 21 | GPIO21 |  |  |  |  |  |  |  |  |  |
| 16 | GPIO16 |  |  |  | ✓ |  |  |  |  |  |
| NC | - |  |  |  |  |  |  |  |  |  |
| GND | - |  |  | ✓ |  |  |  |  |  |  |
| GND | - |  |  | ✓ |  |  |  |  |  |  |
| 3V | - | ✓ |  |  |  |  |  |  |  |  |

### Cab Pin Notes

| Pin | GPIO | Notes |
| --- | --- | --- |
| 3V | - | 3.3 V rail. |
| 1 | GPIO1 | ADC1_CH0, TOUCH1. |
| 2 | GPIO2 | ADC1_CH1, TOUCH2. |
| 3 | GPIO3 | ADC1_CH2, TOUCH3, ESP32-S3 strapping pin. Avoid external circuits that force a reset-time state. |
| 10 | GPIO10 | ADC1_CH9, TOUCH10, SPI_CS. |
| 11 | GPIO11 | ADC2_CH0, TOUCH11, SPI_D. |
| 12 | GPIO12 | ADC2_CH1, TOUCH12, SPI_CLK. |
| 13 | GPIO13 | ADC2_CH2, TOUCH13, SPI_Q. |
| NC | - | Not connected. |
| NC | - | Not connected. |
| GND | - | Ground. |
| +5V | - | 5 V rail. |
| GND | - | Ground. |
| GND | - | Ground. |
| 43 | GPIO43 | UART0 TX / CLK_OUT1; used for serial when USB CDC is disabled. |
| 44 | GPIO44 | UART0 RX / CLK_OUT2; used for serial when USB CDC is disabled. |
| 18 | GPIO18 | ADC2_CH7, U1_RXD. |
| 17 | GPIO17 | ADC2_CH6, U1_TXD. |
| 21 | GPIO21 | Exposed GPIO. |
| 16 | GPIO16 | ADC2_CH5. |
| NC | - | Not connected. |
| GND | - | Ground. |
| GND | - | Ground. |
| 3V | - | 3.3 V rail. |

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

| Pin | GPIO | Notes |
| --- | --- | --- |
| 1 | - | 5 V input. |
| 2 | - | Ground. |
| 3 | - | 3.3 V output. |
| 4 | GPIO0 | ADC-capable GPIO. |
| 5 | GPIO1 | ADC-capable GPIO. |
| 6 | GPIO2 | ADC-capable GPIO; ESP32-C3 strapping pin. Avoid external circuits that force a reset-time state. |
| 7 | GPIO3 | ADC-capable GPIO. |
| 8 | GPIO4 | ADC-capable, SPI SCK, JTAG MTMS caution. |
| 9 | GPIO5 | ADC-capable, SPI MISO, JTAG MTDI caution. |
| 10 | GPIO6 | SPI MOSI, JTAG MTCK caution. |
| 11 | GPIO7 | SPI SS, JTAG MTDO caution. |
| 12 | GPIO8 | Onboard RGB LED / `RGB_BUILTIN`, I2C SDA label, ESP32-C3 strapping pin. Do not treat as ordinary I2C/data if using the onboard RGB LED. |
| 13 | GPIO9 | I2C SCL label, ESP32-C3 boot-mode strapping pin. Download/boot mode is controlled by GPIO9 being LOW at reset. |
| 14 | GPIO10 | Board-labeled RX. |
| 15 | GPIO21 | UART0 TX. |
| 16 | GPIO20 | UART0 RX. |

## Build

From the repository root:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Cab
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Canopy
```
