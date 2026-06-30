# CC-S3 ESP32-S3 backend

PlatformIO/Arduino firmware for the ESP32-S3 Wi-Fi AP and web server.

## AP

- SSID: `CC-S3`
- PASS: `ccs3setup`
- IP: `192.168.4.1`

## Build

```powershell
pio run
pio run --target upload
pio run --target uploadfs
```

Run `uploadfs` on the first flash and after updating `data/index.html`.

## API

- `GET /api/status`
- `GET /api/config`
- `POST /api/config`
- `POST /api/output`
- `POST /api/learn`
- `POST /api/reset`
- `WebSocket /ws`
