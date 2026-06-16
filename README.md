# ShutUp

ShutUp contains two separate PlatformIO firmware projects:

- `Cab`: vehicle-cab device using a LilyGO T-Display S3.
- `Canopy`: canopy device using an ESP32-C3 Supermini V2 with external antenna connection and onboard RGB LED.

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

- Physical board: ESP32-C3 Supermini V2 with external antenna connection and onboard RGB LED
- PlatformIO board ID: `esp32-c3-devkitm-1`
- Framework: Arduino

The Canopy PlatformIO board ID is a compatible ESP32-C3 build target, not a confirmed pin map for the Supermini V2 board. GPIO use must be confirmed separately from exact board documentation before any firmware features or PCB design work.

## Build

From the repository root:

```powershell
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Cab
& 'C:\Users\seanl\.platformio\penv\Scripts\platformio.exe' run -d Canopy
```

