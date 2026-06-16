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

## Build

From the repository root:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Cab
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Canopy
```
