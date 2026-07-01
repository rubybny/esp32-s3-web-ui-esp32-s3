# CC-S3 ESP32-S3 Arduino backend

Arduino IDE sketch for the ESP32-S3 Wi-Fi AP, REST API, WebSocket status stream, and LittleFS-hosted frontend.

## Sketch

Open this folder in Arduino IDE:

```text
CruiseController
```

## AP

- SSID: `CC-S3`
- PASS: `ccs3setup`
- IP: `192.168.4.1`

Open from a phone:

```text
http://192.168.4.1/
```

## Required libraries

- WiFi
- LittleFS
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- Preferences

## Filesystem

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

## API

- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/output`
- `POST /api/learn`
- `POST /api/reset`
- `WebSocket /ws`
