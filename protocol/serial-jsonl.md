# Serial JSON Lines Protocol

The Zephyr firmware emits newline-delimited JSON over USB serial. Each line is a complete JSON object, so host tools can read the stream one line at a time without buffering arbitrary chunks.

## Transport

- Baud rate: `115200`
- Encoding: UTF-8
- Framing: one JSON object per line, terminated by `\n`
- Primary transport: USB serial
- Optional transport: Wi-Fi using the same message schema

## Sample Message

Sample messages use the same field names as the existing Arduino and Zephyr firmware JSON output.

```json
{"type":"sample","heart_rate":null,"breath_rate":null,"distance":null,"raw_distance":null,"total_phase":null,"breath_phase":null,"heart_phase":null,"presence":false,"presence_valid":false,"presence_source":"waiting","target_valid":false,"target_source":"waiting","people_count":0,"target_count":0,"target_x":null,"target_y":null,"target_distance":null,"target_angle":null,"target_speed":null,"targets":[],"light":null,"light_ready":false,"console_fw":"Zephyr 2.2.1","firmware_valid":false,"firmware_raw":0,"firmware_project":0,"firmware_major":0,"firmware_sub":0,"firmware_modified":0,"led_r":0,"led_g":0,"led_b":0,"frame":0,"last_radar_ms":0,"uptime_ms":0}
```

## Host Validation

1. Open the serial port at `115200`.
2. Read one line at a time.
3. Parse each line as JSON.
4. Validate `type == "sample"`.
5. Validate the object against `protocol/sample.schema.json`.
6. Confirm `frame` and `uptime_ms` increase over time.

## Command Direction

Host-to-device commands use the same JSON framing and remain reserved for future control paths.

Command message shape:

```json
{"type":"command","command":"set_led","r":0,"g":80,"b":255,"brightness":0.4}
```
