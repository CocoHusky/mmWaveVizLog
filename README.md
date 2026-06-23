# MR60BHA2 Live Console

This project is now organized as a PlatformIO Arduino project:

```text
firmware/mr60bha2_console/
├── platformio.ini
├── src/
│   └── main.cpp
└── data/
    └── index.html
```

The firmware reads the sensors, controls the RGB LED, creates the WiFi access
point, and serves the browser UI from LittleFS.

## Build And Upload

From the PlatformIO project folder:

```sh
cd /Users/username/Documents/mmWave/firmware/mr60bha2_console
pio run
pio run --target upload
pio run --target uploadfs
pio device monitor
```

The UI filesystem upload is required because `index.html` now lives in
`data/index.html`.

After upload, connect to WiFi:

```text
SSID: mmWaveVisLog-MR60BHA2
Password: wirelessphysiology
URL: http://192.168.4.1/

OTA Hostname: mmWaveVisLog-MR60BHA2-OTA
OTA password: wp-ota
```

## Device Capability Reference

This section documents what the MR60BHA2 module and this XIAO carrier setup can
provide to this project, and how the bytes are shaped on the wire. Module-level
capabilities and constraints come from Seeed's MR60BHA2 technical specification:

```text
https://files.seeedstudio.com/wiki/mmwave-for-xiao/mr60/datasheet/MR60BHA2_Breathing_and_Heartbeat_Module.pdf
```

The public PDF confirms the physical/electrical behavior and the high-level
serial outputs, but it does not include the full raw frame table. The byte-level
frame layout below is therefore taken from the installed `Seeed_Arduino_mmWave`
library parser in:

```text
/Users/username/Documents/Arduino/libraries/Seeed_Arduino_mmWave/src
```

### Module And Board Hardware

| Part | Interface | Project pins | What it provides |
| --- | --- | --- | --- |
| MR60BHA2 60 GHz radar module | UART0 to host MCU | RX `GPIO17`, TX `GPIO16` | Heart rate, breathing rate, range/distance, and raw phase diagnostics confirmed by the datasheet; presence and 3D target data are exposed by the installed Seeed library/firmware path |
| XIAO ESP32-C6 | WiFi AP + USB serial | Board MCU | Hosts this UI at `192.168.4.1`, reads the radar, exposes JSON, and controls local peripherals |
| BH1750 light sensor | I2C | SDA `GPIO22`, SCL `GPIO23`, address `0x23` | Ambient light in lux |
| WS2812 RGB LED | Single-wire RGB | `GPIO1` | Full-color indicator LED, solid/pulse/threshold rule modes |
| Grove GPIO port | GPIO | Kit exposes D0/D10 | Expansion for other sensors/modules; not used by this sketch |

### Datasheet-Confirmed Module Facts

| Category | Value / detail | Notes |
| --- | --- | --- |
| Radar chip | ADT6101P | The PDF says MR60BHA2 is based on the ADT6101P chip. |
| Radar method | FMCW radar | Detects human-body surface echo and estimates breathing/heart frequency from radar signal processing. |
| RF system | 57 to 64 GHz transceiver integration | Product introduction range. RF table lists working frequency as `58` to `62 GHz`. |
| Antenna | 2T2R PCB microstrip antenna | Integrated on the module. |
| MCU/core | ARM Cortex-M3 with radar processing unit | Module performs radar processing before UART output. |
| Flash | 1 MB | Integrated module flash. |
| Size | `25 mm x 31.5 mm` | Module size from product characteristics. |
| Breathing/heartbeat distance | `0.4 m` minimum, `1.5 m` maximum chest distance | The datasheet repeatedly recommends keeping chest distance within `1.5 m`. |
| Breath accuracy | `90%` | Datasheet function parameter. |
| Heartbeat accuracy | `90%` | Datasheet function parameter. |
| Establish detection time | `1 min` | Weak vital-sign reflections require accumulation time. |
| Supply voltage | `3.1` to `3.5 V`, typical `3.3 V` | Datasheet electrical table. |
| Supply current | `600 mA` operating, power supply capacity `> 1 A` recommended | The note section requires enough current headroom. |
| Power quality | `3.2` to `3.4 V`, ripple `<= 50 mV`, current `>= 1 A` | Datasheet precaution for reliable operation. |
| Operating temperature | `-20` to `85 C` | Datasheet electrical table. |
| Storage temperature | `-40` to `85 C` | Datasheet electrical table. |
| Transmit power | `12 dBm` | RF table. |
| Antenna gain | `4 dBi` | RF table. |
| Horizontal beam | `-60` to `+60 deg` at -3 dB | RF table. |
| Vertical beam | `-60` to `+60 deg` at -3 dB | RF table. |
| Default module IO voltage | `3.3 V` | Host MCU interface should be 3.3 V logic. |
| UART configuration note | PDF text says default UART baud is `1382400` with no parity; GUI section says set baud to `115200` | This project and the Seeed Arduino library use `115200`, which matches the GUI section and current working setup. |
| Boot modes | `BOOT1/BOOT0 = 0/1` for flash startup | Datasheet startup table. `0/0` is UART1 serial-port startup, `1/1` is debug. |

### Datasheet-Confirmed Serial Outputs

The technical specification says UART0 can output these detection results to a
host MCU:

| Output | Meaning | Used in this UI |
| --- | --- | --- |
| Total phase | Overall radar phase signal | Yes, graph diagnostic |
| Respiratory phase | Breathing component phase signal | Yes, graph diagnostic |
| Heartbeat phase | Heartbeat component phase signal | Yes, graph diagnostic |
| Respiratory rate | Breathing rate result | Yes, current value and graph |
| Heartbeat rate | Heart rate result | Yes, current value and graph |

The same PDF does not explicitly list distance, presence, or 3D target reports
in its text. Those are available through the installed Seeed Arduino library and
the firmware currently loaded on some MR60BHA2 kits. Treat them as
firmware/library-exposed features rather than datasheet-guaranteed outputs.

### Collectable Data

| Data | Source frame/API | Unit | Payload data | Notes |
| --- | --- | --- | --- | --- |
| Heart rate | `0x0A15`, `getHeartRate()` | bpm | one `float32` | Radar-estimated beats per minute. Accuracy depends heavily on placement and low body motion. |
| Breathing rate | `0x0A14`, `getBreathRate()` | rpm | one `float32` | Radar-estimated breaths per minute. Intended for sleep/rest scenarios. |
| Subject distance/range | `0x0A16`, `getDistance()` | m, but firmware may appear cm | `uint32 range_flag` + `float32 range` | Library only returns distance when `range_flag != 0`. This project normalizes suspicious cm-like values to metres and rejects values beyond `6 m`. |
| Total phase | `0x0A13`, `getHeartBreathPhases()` | raw phase | first `float32` | Diagnostic signal. Absolute value is not a physical range/rate. Trend over time is useful. |
| Breathing phase | `0x0A13`, `getHeartBreathPhases()` | raw phase | second `float32` | Diagnostic breathing-filtered phase. |
| Heart phase | `0x0A13`, `getHeartBreathPhases()` | raw phase | third `float32` | Diagnostic heartbeat-filtered phase. |
| Human presence | `0x0F09`, `readPresence()` / `isHumanDetected()` | boolean | one byte | `0` means no human reported; nonzero means detected. Requires firmware that emits this report. |
| 3D target info | `0x0A04`, `getPeopleCountingTargetInfo()` | m, index | count + target records | Up to `MAX_TARGET_NUM = 3` targets in the library. Used for radar-map position, angle, and speed. Firmware dependent. |
| 3D point cloud detection | `0x0A08`, `getPeopleCountingPointCloud()` | m, index | count + target records | Same record layout as target info. Library notes it may normally have no data on this module/firmware. |
| Target X | `0x0A04` or `0x0A08` target record | m | `float32 x_point` | Lateral position relative to radar. |
| Target Y | `0x0A04` or `0x0A08` target record | m | `float32 y_point` | Forward distance relative to radar. |
| Target speed | derived from `dop_index` | cm/s or m/s | `int32 dop_index` | Seeed example uses `dop_index * RANGE_STEP`; `RANGE_STEP = 17.28`, so speed is `dop_index * 17.28 cm/s`. This sketch displays m/s. |
| Target cluster ID | `0x0A04` or `0x0A08` target record | id | `int32 cluster_index` | Identifier/grouping for tracked target records. |
| Firmware info | `0xFFFF`, `readFirmwareInfo()` | packed version | `uint32` | Parsed as project/major/sub/modified bytes by the library. Not currently shown in the UI. |
| Ambient light | BH1750 I2C | lux | two-byte BH1750 measurement | Not part of MR60BHA2 UART. This sketch reads raw BH1750 high-resolution mode and converts using `raw / 1.2`. |
| RGB LED state/control | WS2812 output | RGB 0-255 | local GPIO control | Not radar data. Controlled by the ESP32 using solid, pulse, or threshold-rule mode. |

### UART Protocol

The radar UART uses `115200` baud. The Seeed frame codec uses a fixed binary
frame:

```text
Byte offset:
0        1      2      3      4      5       6       7          8..N        last
+--------+------+------+------+------+-------+-------+----------+-----------+----------+
| SOF    | ID_H | ID_L | LEN_H| LEN_L| TYPE_H| TYPE_L| HDR_CSUM | DATA      | DATA_CSUM|
+--------+------+------+------+------+-------+-------+----------+-----------+----------+
| 0x01   | big-endian  | big-endian  | big-endian    | 1 byte   | LEN bytes | 1 byte   |
+--------+-------------+-------------+---------------+----------+-----------+----------+
```

Rules:

| Field | Size | Endian | Meaning |
| --- | ---: | --- | --- |
| `SOF` | 1 byte | fixed | Start of frame. Always `0x01`. |
| `ID` | 2 bytes | big-endian | Frame ID. Library-generated outbound frames start at `0x8000` and increment. |
| `LEN` | 2 bytes | big-endian | Number of bytes in `DATA`. Maximum accepted by library is `512`. |
| `TYPE` | 2 bytes | big-endian | Report/command type. MR60BHA2 report types are listed below. |
| `HDR_CSUM` | 1 byte | n/a | `~(SOF xor ID_H xor ID_L xor LEN_H xor LEN_L xor TYPE_H xor TYPE_L)`. |
| `DATA` | `LEN` bytes | little-endian scalars | Payload. Floats are IEEE-754 `float32` little-endian. Integers are `uint32/int32` little-endian. |
| `DATA_CSUM` | 1 byte | n/a | `~(xor of every DATA byte)`. For empty payloads this is `0xFF`. |

### MR60BHA2 Frame Types

| Type | Name in library | Direction | Payload length | Payload shape |
| --- | --- | --- | ---: | --- |
| `0x0A13` | `TypeHeartBreathPhase` | radar to MCU | 12 | `float32 total_phase`, `float32 breath_phase`, `float32 heart_phase` |
| `0x0A14` | `TypeBreathRate` | radar to MCU | 4 | `float32 breath_rate` |
| `0x0A15` | `TypeHeartRate` | radar to MCU | 4 | `float32 heart_rate` |
| `0x0A16` | `TypeHeartBreathDistance` | radar to MCU | 8 | `uint32 range_flag`, `float32 range` |
| `0x0F09` | `ReportHumanDetection` | radar to MCU | at least 1 | `uint8 present` |
| `0x0A04` | `Report3DPointCloudTargetInfo` | radar to MCU | `4 + 16*n` | `uint32 target_count`, then target records |
| `0x0A08` | `Report3DPointCloudDetection` | radar to MCU | `4 + 16*n` | `uint32 target_count`, then target records |
| `0xFFFF` | `ReportFirmware` | radar to MCU | 4 | `uint32 firmware_info` |

### Payload Byte Diagrams

All payload scalar values below are little-endian even though the frame header is
big-endian.

#### Heart/Breath Phase: `TYPE 0x0A13`, `LEN 12`

```text
DATA offset:
0      1      2      3      4      5      6      7      8      9      10     11
+------+------+------+------+------+------+------+------+------+------+------+------+
| total_phase float32 LE | breath_phase float32 LE| heart_phase float32 LE |
+------+------+------+------+------+------+------+------+------+------+------+------+
```

#### Breath Rate: `TYPE 0x0A14`, `LEN 4`

```text
0      1      2      3
+------+------+------+------+
| breath_rate float32 LE |
+------+------+------+------+
```

#### Heart Rate: `TYPE 0x0A15`, `LEN 4`

```text
0      1      2      3
+------+------+------+------+
| heart_rate float32 LE  |
+------+------+------+------+
```

#### Distance/Range: `TYPE 0x0A16`, `LEN 8`

```text
0      1      2      3      4      5      6      7
+------+------+------+------+------+------+------+------+
| range_flag uint32 LE   | range float32 LE             |
+------+------+------+------+------+------+------+------+
```

`range_flag == 0` means the Seeed library treats the range as unavailable.

#### Human Presence: `TYPE 0x0F09`

```text
0
+---------+
| present |
+---------+
```

`present == 0` means clear/no human. Any nonzero value is treated as present.

#### 3D Target Info / Point Cloud: `TYPE 0x0A04` or `0x0A08`

```text
0      1      2      3      4..19                 20..35
+------+------+------+------+---------------------+---------------------+
| target_count uint32 LE | target record 0      | target record 1 ... |
+------+------+------+------+---------------------+---------------------+
```

Each target record is 16 bytes:

```text
record offset:
0      1      2      3      4      5      6      7
+------+------+------+------+------+------+------+------+
| x_point float32 LE     | y_point float32 LE          |
+------+------+------+------+------+------+------+------+
8      9      10     11     12     13     14     15
+------+------+------+------+------+------+------+------+
| dop_index int32 LE     | cluster_index int32 LE      |
+------+------+------+------+------+------+------+------+
```

Derived values used by this UI:

```text
target_distance_m = sqrt(x_point^2 + y_point^2)
target_angle_deg  = atan2(x_point, y_point) * 180 / pi
target_speed_mps  = dop_index * 17.28 / 100
```

### Current JSON Exposed By This Sketch

The browser UI does not consume raw UART bytes directly. The ESP32 parses the
radar frames and serves normalized JSON at:

```text
GET http://192.168.4.1/api/sample
```

| JSON field | Meaning |
| --- | --- |
| `heart_rate` | Heart rate in bpm, or `null`. |
| `breath_rate` | Breathing rate in rpm, or `null`. |
| `distance` | Normalized metres, or `null`. |
| `raw_distance` | Raw value reported by the library before project normalization. |
| `total_phase` | Raw total phase diagnostic. |
| `breath_phase` | Raw breathing phase diagnostic. |
| `heart_phase` | Raw heartbeat phase diagnostic. |
| `presence` | Boolean presence state. |
| `presence_valid` | Whether the presence value has a current source. |
| `presence_source` | `human frame`, `3D target`, `range/vitals`, `no recent signal`, or `waiting`. |
| `target_valid` | Whether a fresh 3D target is available. |
| `target_source` | Source of the target records, currently `target info`, `point cloud`, `stale`, or `waiting`. |
| `people_count` / `target_count` | Number of target records in the newest 3D frame. The Seeed library supports up to `3`. |
| `target_x` | Target X position in metres, or `null`. |
| `target_y` | Target Y position in metres, or `null`. |
| `target_distance` | Derived target range in metres, or `null`. |
| `target_angle` | Derived target angle in degrees, or `null`. |
| `target_speed` | Derived target speed in m/s, or `null`. |
| `targets[]` | Array of all target records exposed to the UI. Each record has `x`, `y`, `distance`, `angle`, `speed`, `dop_index`, and `cluster_index`. |
| `light` | Ambient light in lux, or `null`. |
| `light_ready` | Whether the BH1750 was found at boot. |
| `firmware_valid` | Whether a firmware info frame has been received. |
| `firmware_raw` | Raw packed firmware value from the radar module. |
| `firmware_project` | Firmware project byte. |
| `firmware_major` | Firmware major version byte. |
| `firmware_sub` | Firmware sub-version byte. |
| `firmware_modified` | Firmware modified-version byte. |
| `led_r`, `led_g`, `led_b` | Current brightness-scaled RGB LED output values. |
| `frame` | Incrementing parsed-data frame counter. |
| `last_radar_ms` | Last radar update time from ESP32 `millis()`. |
| `uptime_ms` | Current ESP32 uptime from `millis()`. |

LED commands are sent to:

```text
POST http://192.168.4.1/api/led
```

Supported LED JSON keys:

| Key | Type | Meaning |
| --- | --- | --- |
| `mode` | string | `off`, `solid`, `pulse`, or `rule`. |
| `brightness` | number | `0.0` to `1.0`. |
| `r`, `g`, `b` | integer | Solid/pulse RGB values, `0` to `255`. |
| `metric` | string | Threshold metric, such as `heart_rate`, `breath_rate`, `distance`, `presence`, `light`, or phase fields. |
| `low`, `high` | number | Threshold bounds. |
| `low_r`, `low_g`, `low_b` | integer | Color when metric is below `low`. |
| `ok_r`, `ok_g`, `ok_b` | integer | Color when metric is inside the threshold band. |
| `high_r`, `high_g`, `high_b` | integer | Color when metric is above `high`. |

## Upload

### First Upload Over USB

1. Open `firmware/mr60bha2_wifi_bridge/mr60bha2_wifi_bridge.ino` in Arduino IDE.
2. Select `XIAO ESP32-C6`.
3. Confirm `Seeed Arduino mmWave` version 2.0.0 is installed.
4. Connect the XIAO over USB and upload.
5. Open Serial Monitor at `115200` baud.
6. Confirm the monitor prints `MR60BHA2 console ready`.

The first USB upload is required because OTA support has to already be present
on the ESP32 before it can receive wireless firmware updates.

### Later Uploads Over Device WiFi OTA

The sketch starts Arduino OTA on the board's own access point.

1. Power the board.
2. Connect the computer to WiFi `mmWaveVisLog-MR60BHA2`.
3. Enter password `wirelessphysiology`.
4. In Arduino IDE, select the network port for hostname `mmWaveVisLog-MR60BHA2-OTA`.
5. Upload as usual.
6. When prompted for OTA password, use `wp-ota`.

If the OTA network port does not appear, reconnect to the board WiFi and wait a
few seconds. If OTA fails or a bad sketch breaks WiFi, recover by uploading over
USB again.

Bluetooth/BLE OTA is not implemented here. ESP32 Arduino WiFi OTA is the clean
path for normal Arduino IDE uploads; BLE OTA would require a custom updater app
and a separate firmware transport.

The sketch uses only the ESP32 core, ArduinoOTA, Wire, and Seeed mmWave library.
LED and light support do not require separate Arduino libraries.

## Open The UI

1. Connect the computer or phone to WiFi `mmWaveVisLog-MR60BHA2`.
2. Enter password `wirelessphysiology`.
3. Open `http://192.168.4.1/` in a browser.

This works without any cable except power, but your computer is on the
device's access point while you use it.

## Measurements

- Heart rate and breathing rate are validated MR60BHA2 outputs.
- Distance is normalized to metres. Values beyond the radar's range are rejected; firmware values that appear to be centimetres are converted automatically.
- Presence prefers the MR60BHA2 human-detection frame, then fresh 3D targets, then fresh valid range/vital frames. It clears when no detection source remains fresh.
- Ambient light is read directly from the onboard BH1750 at I2C address `0x23`.
- Total, breathing, and heart phase remain raw radar diagnostics. Their changes over time are useful; their absolute values are not physical distances or rates.
- The dashboard includes a spatial radar view when the module firmware emits 3D target frames. It plots target X/Y position and reports angle and Doppler-derived speed.
- Target speed uses Seeed's conversion `dop_index * 17.28 cm/s`. Spatial values show as unavailable when the installed radar firmware does not emit target frames.
- Every time-series graph shows numeric Y-axis values and uses the same elapsed-time X axis selected by the shared history control.
- Error bands use a trailing 12-point standard deviation calculated separately at every sample, so uncertainty moves with local signal noise.

## Hardware Pins

- MR60BHA2 UART RX: `GPIO17`
- MR60BHA2 UART TX: `GPIO16`
- WS2812 RGB LED: `GPIO1`
- BH1750 I2C SDA: `GPIO22`
- BH1750 I2C SCL: `GPIO23`
