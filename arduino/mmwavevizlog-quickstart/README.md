# mmWaveVizLog Arduino Quickstart

A quick-start Arduino bring-up reference for the Seeed Studio MR60BHA2 mmWave
radar module running on a XIAO ESP32-C6. Use this path to verify hardware
wiring, board setup, Wi-Fi, OTA, LED behavior, and sensor behavior before
working on the maintained Zephyr runtime at `zephyr/mmwavevizlog-runtime/`.

The device creates its own Wi-Fi access point and serves a local web dashboard
for live radar, bio-signal, target tracking, ambient light, status LED rules,
and session logging.

## What it does

- Reads MR60BHA2 radar data over UART.
- Displays heart rate, breathing rate, radar range, presence, raw phase, breathing phase, and heart phase.
- Reads multi-target data when available from the radar target/point-cloud APIs.
- Reads ambient light from a BH1750 sensor over I2C.
- Hosts a browser dashboard at `http://192.168.4.1/`.
- Shows live plots with optional rolling noise/stability bands.
- Draws a radar-style target view with range rings and target dots.
- Controls a WS2812/RGB LED manually or from threshold rules.
- Logs raw JSON samples from the browser and exports them as a `.json` file.
- Supports Arduino OTA updates once the sketch is running.

## Important behavior notes

The multi-target list is frame-based. `Target 1`, `Target 2`, and `Target 3`
are the first, second, and third targets reported in the current radar packet.
They are not guaranteed persistent identities, so two people can swap target
numbers between frames.

The sketch keeps a primary target summary using the first target in the packet.
For multi-person testing, use the individual target cards and multi-target plots
rather than assuming `Target 1` is always the same person.

## Repository Layout

```text
arduino/mmwavevizlog-quickstart/
  mmwavevizlog-quickstart.ino
  vislog_config.h
  vislog_types.h
  vislog_globals.cpp
  vislog_globals.h
  radar_sensor.cpp
  radar_sensor.h
  ambient_light.cpp
  ambient_light.h
  led_control.cpp
  led_control.h
  json_stream.cpp
  json_stream.h
  web_server.cpp
  web_server.h
  ota_updates.cpp
  ota_updates.h
  dashboard_page.h
  README.md
```

## Hardware

| Part | Purpose |
|---|---|
| Seeed Studio XIAO ESP32-C6 | Main microcontroller, Wi-Fi access point, dashboard server |
| Seeed MR60BHA2 mmWave module | Heart, breathing, range, presence, and target data |
| BH1750 light sensor | Ambient light measurement over I2C |
| WS2812/RGB LED | Device status and threshold feedback |
| USB-C cable | Power/programming |
| Breadboard/jumper wires or custom carrier | Wiring and prototyping |

## Pin configuration

| Signal | XIAO ESP32-C6 pin in code | Connects to |
|---|---:|---|
| Radar UART RX | `17` | MR60BHA2 TX |
| Radar UART TX | `16` | MR60BHA2 RX |
| Status RGB LED | `1` | WS2812/RGB LED data pin |
| I2C SDA | `22` | BH1750 SDA |
| I2C SCL | `23` | BH1750 SCL |
| I2C address | `0x23` | BH1750 default address |

Also connect `3V3`/power and `GND` between the XIAO, MR60BHA2, BH1750, and LED
as required by your hardware. If your MR60BHA2 breakout requires 5 V, follow the
module documentation and make sure UART logic levels are safe for the ESP32-C6.

## Software requirements

1. Arduino IDE or Arduino CLI.
2. ESP32 Arduino board package with XIAO ESP32-C6 support.
3. Seeed mmWave library that provides `Seeed_Arduino_mmWave.h`, `SEEED_MR60BHA2`, `PeopleCounting`, and `TargetN`.
4. Built-in ESP32/Arduino libraries used by the sketch: `ArduinoOTA.h`, `HardwareSerial.h`, `WebServer.h`, `WiFi.h`, `Wire.h`, and `esp32-hal-rgb-led.h`.

## Arduino IDE setup

1. Open the `arduino/mmwavevizlog-quickstart/` folder in Arduino IDE.
2. Open `mmwavevizlog-quickstart.ino`.
3. Select your board. Use `XIAO ESP32-C6` if available.
4. Set the serial monitor to `115200 baud`.
5. Upload the sketch over USB.
6. Open the serial monitor. You should see startup messages showing the Wi-Fi network, local IP address, OTA hostname, and whether the BH1750 was found.
7. After upload, wait a few seconds for Wi-Fi and OTA to appear before retrying or assuming the board failed.
8. Use OTA only after the sketch is already running and the board is reachable on its Wi-Fi AP.

## Wi-Fi and dashboard

After upload, the XIAO creates its own Wi-Fi access point:

```text
SSID: mmWaveVisLog-MR60BHA2-A1B2C3
Password: wirelessphysiology
Dashboard: http://192.168.4.1/
```

Each board appends the last 3 bytes of its ESP32-C6 ID to the Wi-Fi name. The
same generated name is printed at boot in the serial monitor.

Connect your laptop or phone to that Wi-Fi network, then open the dashboard URL
in a browser.

## Local credential override

For a private device or demo device, copy:

```text
arduino/mmwavevizlog-quickstart/vislog_private_config.example.h
```

to:

```text
arduino/mmwavevizlog-quickstart/vislog_private_config.h
```

Then change `VISLOG_WIFI_AP_PASSWORD` and `VISLOG_OTA_PASSWORD`. The private
override file is ignored by Git.

## API endpoints

### `GET /api/sample`

Returns the latest sensor state as JSON. Fields may be `null` when the sensor
has not reported a valid value yet.

### `POST /api/led`

Sets the LED mode or threshold rule.

Solid color example:

```json
{
  "mode": "solid",
  "brightness": 0.35,
  "r": 40,
  "g": 190,
  "b": 100
}
```

Threshold rule example:

```json
{
  "mode": "rule",
  "metric": "target_speed",
  "low": -0.05,
  "high": 0.05,
  "low_r": 255,
  "low_g": 0,
  "low_b": 0,
  "ok_r": 0,
  "ok_g": 0,
  "ok_b": 0,
  "high_r": 0,
  "high_g": 80,
  "high_b": 255
}
```

## OTA updates

OTA is enabled after the device boots.

```text
OTA hostname: mmWaveVisLog-MR60BHA2-OTA-A1B2C3
Default OTA password: wp-ota
```

After upload or reset, Wi-Fi and OTA can take a few seconds to appear. Use OTA
only on a trusted local network. Change the default credentials before using this
outside a controlled test setup.

## Data interpretation

Some firmware/library combinations report distance in centimeters even when
examples imply meters. The sketch normalizes suspiciously large range values by
dividing by 100 and rejects values outside the configured valid range.

Target `x` and `y` are normalized to meters if the raw values look like
centimeters. Range and angle are derived from `x` and `y`.

Target speed is estimated from Doppler index using the library-provided
`RANGE_STEP` value.

## Troubleshooting

### I can upload, but the dashboard does not load

- Make sure your computer/phone is connected to the device Wi-Fi.
- Open `http://192.168.4.1/` exactly.
- Check the serial monitor for the softAP IP address.

### Dashboard loads, but data says offline

- Check radar power and ground.
- Check UART wiring: XIAO RX must connect to radar TX, and XIAO TX must connect to radar RX.
- Confirm the radar UART baud rate is `115200`.
- Confirm the MR60BHA2 library version supports the functions used in this sketch.

### BH1750 says not found

- Check SDA/SCL wiring.
- Confirm the BH1750 address is `0x23`.
- Some BH1750 boards can use `0x5C` depending on the ADDR pin. Change `AMBIENT_LIGHT_I2C_ADDRESS` if needed.
