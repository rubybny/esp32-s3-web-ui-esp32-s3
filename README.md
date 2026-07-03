# CC-S3 ESP32-S3 Cruise Controller

Arduino IDE sketch for the ESP32-S3 cruise-control output unit.

The ESP32-S3 runs as a Wi-Fi access point, serves the Web UI from LittleFS, accepts REST/WebSocket commands, and drives four PhotoMOS input LEDs.

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

## Required Libraries

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

Upload it to the default ESP32-S3 data partition:

```powershell
.\tools\upload_littlefs.ps1 -Port COMx
```

Replace `COMx` with the port shown by `arduino-cli board list`.

## GPIO

All outputs drive PhotoMOS input LEDs. `HIGH` is ON, `LOW` is OFF.

| Name | GPIO | Contact Side |
|---|---:|---|
| MAIN_OUT | 5 | short to 3-drive COM |
| RES_OUT | 6 | short to 3-drive COM |
| SET_OUT | 7 | short to 3-drive COM |
| BRAKE_OUT | 9 | connect `BRAKE_12V_IN` to 3-drive gray wire |

`CANCEL` operation uses `BRAKE_OUT` (`GPIO9`). The dedicated CANCEL contact output is not used.

Removed from this dedicated version:

- Steering ADC input `GPIO4`
- ATOTO/audio outputs `GPIO35-39`
- Illumination control
- Brake input detection `GPIO10`
- PC817 input processing
- VIN/VOUT measurement

## API

- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/output`
- `POST /api/reset`
- `WebSocket /ws`

`POST /api/output` pulse command:

```json
{
  "name": "main",
  "durationMs": 200
}
```

Valid names:

```text
main
res
set
cancel
```

`POST /api/config`:

```json
{
  "pulseMs": 200
}
```

`GET /api/status` and WebSocket status:

```json
{
  "pulseMs": 200,
  "outputs": {
    "main": false,
    "res": false,
    "set": false,
    "brake": false
  }
}
```

## Safety Logic

- Startup sets `GPIO5`, `GPIO6`, `GPIO7`, and `GPIO9` to `OUTPUT` and immediately drives all LOW.
- A new operation clears all outputs LOW before turning the requested output ON.
- Only one output can be ON at a time.
- Pulse output defaults to `200 ms`.
- WebSocket broadcasts status every `500 ms`.
