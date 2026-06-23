# Serial JSON Lines Protocol

The Zephyr firmware emits newline-delimited JSON over USB serial. Each line is a complete JSON object, so host tools can read the stream one line at a time without buffering arbitrary chunks.

## Transport

- Baud rate: `115200`
- Encoding: UTF-8
- Framing: one JSON object per line, terminated by `\n`
- First production target: USB serial
- Later optional target: Wi-Fi/WebSocket using the same message schema

## Sample Message

Sample messages use the same field names as the existing Arduino and PlatformIO firmware JSON output.

```json
{"type":"sample","heart_rate":72.40,"breath_rate":16.20,"distance":0.720,"raw_distance":0.720,"total_phase":0.120,"breath_phase":0.060,"heart_phase":0.020,"presence":true,"presence_valid":true,"presence_source":"sim","target_valid":true,"target_source":"sim","people_count":1,"target_count":1,"target_x":-0.080,"target_y":0.720,"target_distance":0.724,"target_angle":-6.30,"target_speed":0.000,"targets":[{"x":-0.080,"y":0.720,"distance":0.724,"angle":-6.30,"speed":0.000,"dop_index":0,"cluster_index":0}],"light":120.0,"light_ready":true,"console_fw":"2.1.4-zephyr","firmware_valid":false,"firmware_raw":0,"firmware_project":0,"firmware_major":0,"firmware_sub":0,"firmware_modified":0,"led_r":14,"led_g":66,"led_b":35,"frame":1,"last_radar_ms":0,"uptime_ms":1000}
```

## Host Validation

1. Open the serial port at `115200`.
2. Read one line at a time.
3. Parse each line as JSON.
4. Validate `type == "sample"`.
5. Validate the object against `protocol/sample.schema.json`.
6. Confirm `frame` and `uptime_ms` increase over time.

## Command Direction

The first Zephyr slice is output-only. Command messages will be added after real radar ingest is stable.

Planned host-to-device command shape:

```json
{"type":"command","command":"set_led","r":0,"g":80,"b":255,"brightness":0.4}
```
