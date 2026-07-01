# CC-S3 ESP32-S3 Cruise Controller

Arduino IDE sketch for the ESP32-S3 Wi-Fi AP, REST API, WebSocket status stream, and LittleFS-hosted frontend.

## Open

Open this folder in Arduino IDE:

```text
CruiseController
```

## AP

- SSID: `CC-S3`
- PASS: `ccs3setup`
- IP: `192.168.4.1`

Open from a phone after flashing:

```text
http://192.168.4.1/
```

## Required libraries

- WiFi
- LittleFS
- ESP Async WebServer `3.11.2`
- Async TCP `3.4.10`
- ArduinoJson
- Preferences

If older `ESPAsyncWebServer` or `AsyncTCP` forks are installed, Arduino may choose the wrong one. Keep these active:

```text
ESP Async WebServer
Async TCP
```

## Compile

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 CruiseController
```

## Upload Sketch

Check the serial port:

```powershell
arduino-cli board list
```

Upload the sketch:

```powershell
arduino-cli upload -p COMx --fqbn esp32:esp32:esp32s3 CruiseController
```

Replace `COMx` with the detected port.

## Upload LittleFS

The frontend is stored here:

```text
CruiseController/data/index.html
```

Build the LittleFS image:

```powershell
& "$env:LOCALAPPDATA\Arduino15\packages\esp32\tools\mklittlefs\4.0.2-db0513a\mklittlefs.exe" -c CruiseController\data -b 4096 -p 256 -s 0x160000 build\littlefs.bin
```

Upload it to the default ESP32-S3 data partition:

```powershell
.\tools\upload_littlefs.ps1 -Port COMx
```

Replace `COMx` with the port shown by `arduino-cli board list`.

## Verify

1. Connect a phone to Wi-Fi `CC-S3`.
2. Enter password `ccs3setup`.
3. Open `http://192.168.4.1/`.
4. Confirm Monitor values update.
5. Confirm `Main` outputs for the configured hold time.
6. Confirm `UpSet`, `DownRes`, `Cancel`, and `BrakeOut` output only while pressed from the UI.
7. Confirm only one output can be ON at a time.

## API

- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/output`
- `POST /api/learn`
- `POST /api/reset`
- `WebSocket /ws`

## GPIO

| Name | GPIO | Direction |
|---|---:|---|
| ADC_STEER | 4 | input ADC |
| ADC_VIN | 11 | input ADC |
| ADC_VOUT | 12 | input ADC |
| BRAKE_IN | 10 | input, active HIGH |
| OUT_MAIN | 5 | output |
| OUT_UPSET | 6 | output |
| OUT_DOWNRES | 7 | output |
| OUT_CANCEL | 8 | output |
| OUT_BRAKE | 9 | output |
| OUT_NAV_VOLUP | 35 | output to PhotoMOS |
| OUT_NAV_VOLDOWN | 36 | output to PhotoMOS |
| OUT_NAV_SEEKPLUS | 37 | output to PhotoMOS |
| OUT_NAV_SEEKMINUS | 38 | output to PhotoMOS |
| OUT_NAV_MODE | 39 | output to PhotoMOS |

L/R steering buttons share `ADC_STEER`. R-side buttons drive cruise outputs. L-side buttons drive ATOTO KEY1 resistor selection through hardware resistors and PhotoMOS switches.

ATOTO uses two wires: `KEY1` and `GND`. The firmware only turns one `OUT_NAV_*` GPIO ON at a time; the resistor value is selected by the external resistor + PhotoMOS hardware.
The default pin map targets the Freenove ESP32-S3 Board Lite shown by the user. Avoid `GPIO0`, `GPIO19`, `GPIO20`, `GPIO45`, `GPIO46`, and `GPIO48` unless the board design intentionally uses them.

## Safety Logic

- Output GPIOs are mutually exclusive. Turning one output ON turns all other outputs OFF.
- Steering ADC detection is debounced for 60 ms before changing outputs.
- `UpSet`, `DownRes`, and `Cancel` are momentary from steering ADC: output stays ON only while the debounced button is held.
- `Main` starts one hold pulse using `timing.mainHoldMs`; it does not retrigger until the button is released.
- When `BrakeIn` is active, automatic steering outputs are cleared and all output GPIOs are turned OFF.
- ATOTO Navi outputs are mutually exclusive. `NONE` or any R-side button turns all Navi outputs OFF.
- L-side Navi output turns cruise outputs OFF before selecting the KEY1 resistor.
