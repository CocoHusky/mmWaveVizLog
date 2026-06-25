# Firmware Module Map

This document maps the main software modules to their responsibilities and verification hooks.

## Zephyr Runtime Modules

| Module | Path | Responsibility | Verification |
| --- | --- | --- | --- |
| Application entry | `zephyr/mmwavevizlog-runtime/src/main.c` | Boot sequencing, module initialization, periodic sample publication. | Firmware build, serial validation. |
| App state | `zephyr/mmwavevizlog-runtime/src/app_state.*` | Bounded latest-value model for radar, ambient, LED, firmware, and timing state. | Parser tests, JSON schema examples, dashboard/API validation. |
| Radar driver | `zephyr/mmwavevizlog-runtime/drivers/mr60bha2/mr60bha2.*` | Device-facing MR60BHA2 decode and state update logic. | Saved frame parser tests. |
| Radar parser | `zephyr/mmwavevizlog-runtime/drivers/mr60bha2/mr60bha2_parser.*` | Byte-wise UART frame parser, frame envelope detection, checksum handling. | Native parser tests and advisory Twister suite. |
| Ambient light | `zephyr/mmwavevizlog-runtime/src/ambient_light.*` | BH1750 I2C initialization and lux sampling. | Firmware build and manual sensor validation. |
| JSON stream | `zephyr/mmwavevizlog-runtime/src/json_stream.*` | Schema-compatible JSON sample serialization. | JSON schema validation and serial validation. |
| Network services | `zephyr/mmwavevizlog-runtime/src/net_services.*` | Wi-Fi AP, local HTTP dashboard, and API service layer. | Firmware build and dashboard/API validation. |
| LED control | `zephyr/mmwavevizlog-runtime/src/led_control.*` | Status LED output and local feedback control. | Firmware build and manual LED/API validation. |
| OTA control | `zephyr/mmwavevizlog-runtime/src/ota_control.*` | OTA/update control path for signed firmware images. | Release workflow artifact verification and manual OTA validation. |

## Arduino Quick-Start Modules

| Module | Path | Responsibility |
| --- | --- | --- |
| Sketch entry | `arduino/mmwavevizlog-quickstart/mmwavevizlog-quickstart.ino` | Fast first-boot firmware and hardware bring-up. |
| Radar sensor | `arduino/mmwavevizlog-quickstart/radar_sensor.*` | MR60BHA2 library integration and target/vital-sign sampling. |
| Ambient light | `arduino/mmwavevizlog-quickstart/ambient_light.*` | BH1750 bring-up and lux sampling. |
| JSON stream | `arduino/mmwavevizlog-quickstart/json_stream.*` | Browser/API sample JSON output. |
| Web server | `arduino/mmwavevizlog-quickstart/web_server.*` | Local dashboard and HTTP routes. |
| LED control | `arduino/mmwavevizlog-quickstart/led_control.*` | LED manual control and threshold rules. |
| OTA updates | `arduino/mmwavevizlog-quickstart/ota_updates.*` | Arduino OTA bring-up and update handling. |
| Config/globals/types | `arduino/mmwavevizlog-quickstart/vislog_*` | Shared configuration, type definitions, and app globals. |

## Cross-Cutting Concerns

| Concern | Current mechanism | Next improvement |
| --- | --- | --- |
| Protocol compatibility | `protocol/sample.schema.json` and examples | Add compatibility notes for future schema versions. |
| Release quality | CI firmware build and release workflow artifact checks | Pin Zephyr to a known-good commit after first release. |
| Parser quality | Saved binary frame fixtures | Add malformed/checksum-bad frame fixtures. |
| Dashboard/API quality | Manual browser validation | Add automated HTTP/API smoke tests. |
| Documentation quality | README plus docs folder | Keep docs updated through PR template checklist. |
