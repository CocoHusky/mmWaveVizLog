# Firmware Module Map

This document maps the major software modules to their responsibilities and verification strategy.

| Module | Path | Purpose | Verification |
| --- | --- | --- | --- |
| MR60BHA2 parser | `zephyr/mmwavevizlog-runtime/drivers/mr60bha2/mr60bha2_parser.*` | Byte/frame parser for radar packets, including sync, length, and checksum handling. | Native simulator parser tests with saved binary fixtures. |
| MR60BHA2 driver | `zephyr/mmwavevizlog-runtime/drivers/mr60bha2/mr60bha2.*` | Device-facing radar abstraction and snapshot extraction. | Parser fixture tests and firmware build. |
| App state | `zephyr/mmwavevizlog-runtime/src/app_state.*` | Bounded latest-sample state used by JSON, dashboard, LED rules, and stale-data handling. | Firmware build and schema-compatible sample checks. |
| JSON stream | `zephyr/mmwavevizlog-runtime/src/json_stream.*` | Converts sensor state into protocol-compatible JSON. | Protocol example validation and serial validation checklist. |
| Network services | `zephyr/mmwavevizlog-runtime/src/net_services.*` | Wi-Fi AP, local HTTP API, and dashboard service boundary. | Firmware build and dashboard/API validation plan. |
| LED control | `zephyr/mmwavevizlog-runtime/src/led_control.*` | Status LED and threshold-rule output. | Firmware build and manual LED rule validation. |
| OTA control | `zephyr/mmwavevizlog-runtime/src/ota_control.*` | Firmware update control path and signed-image workflow integration. | Release workflow verifies signed artifact. |
| Ambient light | `zephyr/mmwavevizlog-runtime/src/ambient_light.*` | BH1750 I2C integration. | Firmware build and hardware bring-up checklist. |
| Arduino quickstart | `arduino/mmwavevizlog-quickstart/` | Fast bring-up reference for wiring, AP, dashboard, LED, OTA, and sensor behavior. | Manual Arduino upload and dashboard smoke check. |

## Design rules

- Parser logic should be deterministic, byte-oriented, and testable without hardware.
- Data exported from firmware should remain compatible with `protocol/sample.schema.json`.
- Hardware-facing modules should not own protocol formatting.
- Dashboard behavior should not redefine firmware semantics.
- New fields should be added with schema updates and example updates in the same pull request.
