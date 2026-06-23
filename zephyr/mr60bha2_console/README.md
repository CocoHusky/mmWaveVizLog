# MR60BHA2 Zephyr Console

This is the production-oriented Zephyr firmware path for the XIAO ESP32-C6 and Seeed MR60BHA2 sensor stack. The current Arduino quicksetup remains the behavior reference while this implementation grows toward feature parity.

## Current Milestone

The first milestone is intentionally small:

- boot on XIAO ESP32-C6
- maintain bounded sample state
- stream schema-compatible JSON Lines over USB serial
- provide an MR60BHA2 driver/parser boundary
- include parser tests that can run before hardware decoding is complete

The current data stream is simulated. Real MR60BHA2 UART decoding is the next hardware milestone.

## Layout

```text
zephyr/mr60bha2_console/
  CMakeLists.txt
  prj.conf
  VERSION
  boards/
    seeed_xiao_esp32c6.overlay
  drivers/
    mr60bha2/
  src/
  tests/
    mr60bha2_parser/
```

## Hardware Contract

The Zephyr target preserves the current Arduino wiring contract:

| Signal | GPIO |
| --- | ---: |
| MR60BHA2 UART RX | `17` |
| MR60BHA2 UART TX | `16` |
| WS2812 RGB LED | `1` |
| BH1750 I2C SDA | `22` |
| BH1750 I2C SCL | `23` |
| BH1750 I2C address | `0x23` |

## Build

From the repository root:

```sh
west build -b seeed_xiao_esp32c6 zephyr/mr60bha2_console
```

If the board name is not available in your Zephyr checkout:

```sh
west boards | rg "xiao|esp32c6|seeed"
```

## Flash

```sh
west flash
```

## Serial Validation

Open the device serial port at `115200`.

```sh
screen /dev/cu.usbmodem14201 115200
```

Expected output is one JSON object per line:

```json
{"type":"sample","heart_rate":72.40,"breath_rate":16.20,"distance":0.720,"raw_distance":0.720,"total_phase":0.120,"breath_phase":0.060,"heart_phase":0.020,"presence":true,"presence_valid":true,"presence_source":"sim","target_valid":true,"target_source":"sim","people_count":1,"target_count":1,"target_x":-0.080,"target_y":0.720,"target_distance":0.724,"target_angle":-6.30,"target_speed":0.000,"targets":[{"x":-0.080,"y":0.720,"distance":0.724,"angle":-6.30,"speed":0.000,"dop_index":0,"cluster_index":0}],"light":120.0,"light_ready":true,"console_fw":"2.1.4-zephyr","firmware_valid":false,"firmware_raw":0,"firmware_project":0,"firmware_major":0,"firmware_sub":0,"firmware_modified":0,"led_r":14,"led_g":66,"led_b":35,"frame":1,"last_radar_ms":0,"uptime_ms":1000}
```

Validation checklist:

1. Device boots without resetting repeatedly.
2. JSON Lines appear once per second.
3. `frame` increments.
4. `uptime_ms` increments.
5. Every line parses as JSON.
6. The parsed object validates against `protocol/sample.schema.json`.

## Parser Tests

```sh
west build -b native_sim zephyr/mr60bha2_console/tests/mr60bha2_parser
west twister -T zephyr/mr60bha2_console/tests
```

The parser test currently verifies initialization, empty input handling, partial frame handling, and MR60BHA2 frame envelope detection. Real captured radar frames should be added next.

## Next Hardware Milestones

1. Configure the MR60BHA2 UART in devicetree for GPIO17/GPIO16.
2. Add raw byte ingest from the radar UART.
3. Capture real MR60BHA2 frames from the current Arduino firmware.
4. Extend `mr60bha2_parser.c` to decode heart, breath, range, presence, target, and firmware frames.
5. Feed decoded samples into `app_state.c`.
6. Add BH1750 I2C support.
7. Add WS2812 LED control and threshold rules.
