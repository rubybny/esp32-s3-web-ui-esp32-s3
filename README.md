# CC-S3 ESP32-S3 Cruise Controller

Arduino IDE sketch for the ESP32-S3 cruise-control output unit.

The ESP32-S3 reads the cruise switch resistor ladder on an ADC pin and
drives four PhotoMOS input LEDs accordingly. There is no Wi-Fi, no web UI,
and no runtime configuration API: all tuning (ADC thresholds, pulse
timing) is done in [`Config.h`](CruiseController/Config.h) before
flashing. Status and events are printed over USB serial for use with a PC.

## Open

Open this folder in Arduino IDE:

```text
CruiseController
```

## Required Libraries

- None beyond the ESP32 Arduino core (no WiFi/LittleFS/ArduinoJson/Preferences needed).

## Compile

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32s3 CruiseController
```

## Upload

Check the serial port:

```powershell
arduino-cli board list
```

Upload the sketch:

```powershell
arduino-cli upload -p COMx --fqbn esp32:esp32:esp32s3 CruiseController
```

Replace `COMx` with the detected port.

## PC Logging

Connect the ESP32-S3 to a PC over USB and open a serial monitor at
`115200` baud, e.g.:

```powershell
arduino-cli monitor -p COMx -c baudrate=115200
```

The device logs a line on every debounced button change, every output
on/off transition, and a heartbeat every 2 seconds with the current ADC
reading and output states:

```text
[    204ms] CC-S3 cruise controller starting
[    204ms] Ready
[   1532ms] BUTTON NONE -> SET- (adc=341)
[   1532ms] OUTPUT set ON (min 200ms)
[   1732ms] OUTPUT set OFF
[   2000ms] adc=4095 button=NONE  main=0 res=0 set=0 cancel=0
```

## Changing Tuning Values

Edit the constants in [`Config.h`](CruiseController/Config.h) and reflash:

- `CruiseConfig::ADC_MAIN/ADC_CANCEL/ADC_RES/ADC_SET` -- learned ADC
  targets for each button (must stay sorted ascending).
- `CruiseConfig::ADC_OPEN_MIN` -- readings above this are treated as
  "not pressed".
- `CruiseConfig::DEBOUNCE_MS` -- how long a reading must be stable before
  it is accepted.
- `CruiseConfig::PULSE_MAIN_MS` / `PULSE_RES_MS` / `PULSE_SET_MS` /
  `PULSE_CANCEL_MS` -- minimum time each output stays driven once
  activated.

To re-learn an ADC target, watch the serial log while pressing the
physical button and copy the reported `adc=` value into `Config.h`.

## GPIO

All outputs drive PhotoMOS input LEDs. `HIGH` is ON, `LOW` is OFF.

| Name | GPIO | Direction | Contact Side |
|---|---:|---|---|
| CRUISE_ADC | 4 | ADC input | cruise switch resistor ladder |
| MAIN_OUT | 5 | output | short to 3-drive COM |
| RES_OUT | 6 | output | short to 3-drive COM |
| SET_OUT | 7 | output | short to 3-drive COM |
| BRAKE_OUT | 9 | output | connect `BRAKE_12V_IN` to 3-drive gray wire |

`CANCEL` operation uses `BRAKE_OUT` (`GPIO9`). The dedicated CANCEL contact output is not used.

## ADC Input

Connect the cruise switch resistor ladder to `GPIO4`.

```text
3.3V
 |
10k pull-up
 |
GPIO4 ADC
 |
cruise resistor ladder
 |
GND
```

Target resistance values:

| Button | Resistance to GND | Approx ADC |
|---|---:|---:|
| MAIN | 0 ohm | 0 |
| CANCEL | 239.7 ohm | 96 |
| RES+ | 389 ohm | 153 |
| SET- | 909 ohm | 341 |
| Not pressed | OPEN | 4095 |

Removed from this dedicated version:

- ATOTO/audio outputs `GPIO35-39`
- Illumination control
- Brake input detection `GPIO10`
- PC817 input processing
- VIN/VOUT measurement

## Safety Logic

- Startup sets `GPIO5`, `GPIO6`, `GPIO7`, and `GPIO9` to `OUTPUT` and immediately drives all LOW.
- A raw ADC reading must classify to the same button for `DEBOUNCE_MS`
  before it is accepted -- a single noisy sample cannot fire an output.
- A new confirmed button clears all outputs LOW before turning the
  requested output ON.
- Only one output can be ON at a time.
- Once activated, an output stays on for at least its configured pulse
  time even if the button is released early, and turns off once the
  button reads released and that minimum time has elapsed.
