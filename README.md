# CC-S3 ESP32-S3 Cruise Controller

Arduino IDE sketch for the ESP32-S3 cruise-control output unit.

The ESP32-S3 reads a 2-wire resistive cruise lever on one ADC pin, runs it
through a debounce/hysteresis/neutral-lockout state machine, and drives up
to four vehicle output lines. There is no Wi-Fi, no web UI, and no
phone-based tuning: every tunable lives in
[`Config.h`](CruiseController/Config.h) and is set before flashing.
Behavior is observed by connecting the ESP32-S3 to a PC over USB and
watching the serial log; a small set of serial commands can also learn ADC
values live and store them in NVS.

## Open

Open this folder in Arduino IDE:

```text
CruiseController
```

## Required Libraries

- ESP32 Arduino core only. `Preferences` (bundled with the core) is used for
  optional ADC learning storage -- no WiFi/LittleFS/ArduinoJson/AsyncWebServer.

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

The device logs every confirmed lever state change, every output on/off
transition, and (with `DEBUG_MODE` on) a heartbeat every 2 seconds showing
raw ADC, filtered ADC, candidate, confirmed state, hold time, and output
states:

```text
[    204ms] CC-S3 cruise controller starting (serial-only build)
[    204ms] GPIO9 role: CANCEL output
[    350ms] Calibration loaded: main=12 cancel=62 res=205 set=498 neutralMin=3800
[    350ms] Ready
[   1532ms] STATE NONE -> SET- (adc=498 filtered)
[   1532ms] OUTPUT SET- ON
[   1780ms] STATE SET- -> NONE (adc=4095 filtered)
[   2000ms] HB raw=4095 filtered=4095 candidate=NONE   confirmed=NONE   held=220ms main=0 res=0 set=0 cancel=0
```

## Serial Calibration Commands

With `ENABLE_ADC_LEARNING` on (default), type a single character into the
serial monitor and press enter:

| Key | Action |
|---|---|
| `m` | Learn MAIN center from the current filtered ADC reading |
| `c` | Learn CANCEL center |
| `r` | Learn RES+ center |
| `s` | Learn SET- center |
| `n` | Learn the NEUTRAL (released) threshold -- release the lever fully first |
| `x` | Erase learned calibration, revert to `Config.h` defaults |
| `p` | Print the currently effective ADC bands |
| `h` | Help |

Learned centers persist in NVS across reboots; the configured band *width*
in `Config.h` stays fixed, only the center shifts.

## Changing Tuning Values

Everything is in [`Config.h`](CruiseController/Config.h):

- `AdcBands::ADC_MAIN_MIN/MAX`, `ADC_CANCEL_MIN/MAX`, `ADC_RES_MIN/MAX`,
  `ADC_SET_MIN/MAX` -- per-button ADC bands. Readings between bands (or
  between `ADC_SET_MAX` and `ADC_NEUTRAL_MIN`) are `UNKNOWN` and never
  drive an output -- see "Why UNKNOWN exists" below.
- `AdcBands::ADC_NEUTRAL_MIN` -- above this, the lever is "released".
- `AdcBands::ADC_HYSTERESIS` -- widens only the *currently confirmed*
  band, so a reading sitting on an edge doesn't chatter.
- `AdcFilterConfig::SAMPLE_COUNT` / `SAMPLE_INTERVAL_MS` -- median filter
  window size and sample spacing.
- `StateTiming::CONFIRM_MS` -- debounce time a candidate must hold before
  becoming the confirmed lever state.
- `StateTiming::MIN_OUTPUT_ON_MS` -- minimum time an output stays driven
  once activated.
- `StateTiming::MAIN_LONG_PRESS_MS` -- how long MAIN must be held to report
  a long press (detection only; not wired to different behavior yet).
- `OUTPUT_ACTIVE_LEVEL` -- `ACTIVE_HIGH` or `ACTIVE_LOW` for the vehicle
  output lines, depending on the final analog-switch/PhotoMOS wiring.
- `USE_BRAKE_INPUT` -- `0` makes GPIO9 the CANCEL output (current
  assumption); `1` makes it a BRAKE input instead (CANCEL then has no
  output pin -- see GPIO table below).
- `BRAKE_ACTIVE_LEVEL`, `BrakeConfig::DEBOUNCE_MS` -- only used when
  `USE_BRAKE_INPUT` is `1`.
- `DEBUG_MODE` -- turn off to silence the heartbeat/verbose logging.
- `ENABLE_ADC_LEARNING` -- turn off to disable NVS calibration entirely and
  always use the `Config.h` constants.

## GPIO

| Name | GPIO | Direction | Notes |
|---|---:|---|---|
| ADC_CRUISE | 4 | ADC input | Lever resistor ladder, 10k pull-up to 3.3V |
| OUT_MAIN | 5 | output | -> analog switch/PhotoMOS, vehicle MAIN line |
| OUT_RES | 6 | output | -> analog switch/PhotoMOS, vehicle RES+ line |
| OUT_SET | 7 | output | -> analog switch/PhotoMOS, vehicle SET- line |
| PIN_9 | 9 | output **or** input | CANCEL output (`USE_BRAKE_INPUT=0`, default) **or** BRAKE input (`USE_BRAKE_INPUT=1`) -- see `Config.h` |

Output polarity (`HIGH`-active vs `LOW`-active) is set once via
`OUTPUT_ACTIVE_LEVEL` in `Config.h`, since the final analog-switch part
(74LVC2G66 / SN74HC4066 / AQY210EHA) isn't finalized.

## ADC Input

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

Approximate measured centers (adjust in `Config.h` once retested on the
real hardware):

| Button | Resistance to GND (example) | Approx ADC |
|---|---:|---:|
| MAIN | ~0 ohm | ~0 |
| CANCEL | ~250-300 ohm | ~60 |
| RES+ | ~1.44 kohm | ~200 |
| SET- | ~8.25 kohm range | ~500 |
| Not pressed | OPEN | ~4095 |

## Lever State Machine

Modules, matching the flow below:

- `AdcFilter` -- `readAdc()` / `filterAdc()`: paced raw sampling + median filter.
- `Calibration` -- `loadAdcCalibration()` / `saveAdcCalibration()` / learn/reset: NVS-backed band centers.
- `LeverState` -- `detectLeverState()` / `updateLeverState()`: candidate -> confirm -> neutral-lockout state machine.
- `BrakeInput` -- optional debounced brake input, only active when `USE_BRAKE_INPUT=1`.
- `Outputs` -- `setMainOutput()` / `setResOutput()` / `setSetOutput()` / `setCancelOutput()` / `allOutputsOff()`: mutually-exclusive, polarity-aware pin writes.
- `ActionHandler` -- `handleLeverAction()`: combines lever state, brake override, and minimum-on-time into actual output changes.
- `SerialCommands` / `Logger` -- calibration commands and debug/heartbeat output.

### Why `UNKNOWN` exists, and the neutral lockout

The lever is one ADC line shared by four buttons at different resistances.
Releasing any button sweeps the ADC reading from that button's value back up
toward the open-circuit reading -- numerically passing through the *other*
buttons' ranges on the way, even though only one button was ever physically
pressed. The previous firmware classified every reading as the nearest
button with no gap between ranges, so that release sweep could misread as a
brand-new press (e.g. MAIN release momentarily reading as CANCEL or SET-).

This rewrite fixes it with two things working together:

1. Bands are narrower than the gaps between them (`ADC_xxx_MIN/MAX`), so a
   sweep spends most of its transit in `UNKNOWN`, not in a neighboring
   button's band.
2. Once the lever state machine confirms a real button, it is **locked**:
   no other value (including a genuinely different button) can become the
   new confirmed state until the lever is confirmed back to `NONE`
   (released). A sweep through another button's numeric band during release
   is therefore invisible to the rest of the firmware -- see the state
   diagram and comment in [`LeverState.cpp`](CruiseController/LeverState.cpp).

This also fixes the old "second MAIN press needs a longer hold" bug: state
resets to a clean, identical `NONE` every time, so every press is detected
under exactly the same conditions regardless of what happened before it.

## Safety Logic

- `setupOutputPins()` runs first in `setup()`, before serial/ADC/anything
  else, and forces every vehicle line to its inactive level before it is
  ever switched to `OUTPUT` mode -- no floating-pin glitch on boot.
- Raw ADC must classify to the same band for `StateTiming::CONFIRM_MS`
  before it is accepted as a candidate, and the candidate must further
  clear the neutral lockout described above before it becomes the
  confirmed state that actually drives outputs.
- `UNKNOWN` and `NONE` both mean "drive nothing"; only a confirmed
  MAIN/CANCEL/RES/SET turns an output on, and only one output is ever
  energized at a time (`Outputs.cpp` clears all state before setting the
  new one).
- Once activated, an output stays on for at least `MIN_OUTPUT_ON_MS` so a
  quick tap still produces a pulse long enough for the vehicle ECU to
  register, then turns off as soon as the lever is confirmed released.
- If `USE_BRAKE_INPUT` is enabled, a detected brake forces every output off
  immediately and blocks new operations until both the brake clears *and*
  the lever is independently confirmed back to `NONE`.

## Not Yet Finalized

Per the current hardware spec, these are placeholders in `Config.h` pending
real measurements and circuit decisions:

- Exact ADC ranges for each button (`AdcBands::*`).
- `StateTiming::MAIN_LONG_PRESS_MS` and `CONFIRM_MS`.
- GPIO9's final role (`USE_BRAKE_INPUT`) and, if used, brake polarity.
- `OUTPUT_ACTIVE_LEVEL` (depends on the final analog-switch part).
- Final analog-switch/PhotoMOS part numbers (74LVC2G66 / SN74HC4066 /
  AQY210EHA under consideration).
