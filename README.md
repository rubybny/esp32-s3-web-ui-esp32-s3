# CC-S3 ESP32-S3 Arduino backend

Arduino IDE sketch for the ESP32-S3 Wi-Fi AP, REST API, WebSocket status stream, and LittleFS-hosted frontend.

## Sketch

Open this folder in Arduino IDE:

```text
arduino/CC_S3_Backend
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
arduino/CC_S3_Backend/data/index.html
```

Upload the LittleFS data folder after flashing the sketch.

## API

- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/output`
- `POST /api/learn`
- `POST /api/reset`
- `WebSocket /ws`
