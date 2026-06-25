# Software Validation Plan

This plan verifies the base sensing software platform. It does not validate a finished wearable product, enclosure, battery system, thermal design, or medical/clinical performance.

## Validation Matrix

| Test ID | Requirement(s) | Purpose | Method | Pass Criteria | Evidence |
| --- | --- | --- | --- | --- | --- |
| VAL-SW-001 | REQ-SW-001 | Confirm target firmware builds. | Run Zephyr CI firmware job. | `Build XIAO ESP32-C6 firmware` passes. | CI run link. |
| VAL-SW-002 | REQ-SW-002 | Confirm parser detects a valid frame envelope. | Run native parser executable. | Envelope test passes. | CI run link. |
| VAL-SW-003 | REQ-SW-003 to REQ-SW-007 | Confirm saved MR60BHA2 frames decode to expected snapshots. | Run native parser fixture tests. | All saved fixture tests pass. | CI run link. |
| VAL-SW-004 | REQ-SW-008, REQ-SW-009 | Confirm protocol examples match schema. | Run JSON schema validation job. | Every `protocol/examples/*.json` file validates. | CI run link. |
| VAL-SW-005 | REQ-SW-010 | Confirm sample API returns parseable JSON. | Connect to device AP and request `GET /api/sample`. | Response is valid JSON and validates against schema. | Saved response or screenshot. |
| VAL-SW-006 | REQ-SW-011 | Confirm dashboard loads and updates. | Connect browser to `http://192.168.4.1/`. | Dashboard loads, plots update, frame/uptime counters increase. | Screenshot. |
| VAL-SW-007 | REQ-SW-012 | Confirm LED control endpoint behavior. | Send valid LED payload through dashboard/API. | LED changes state and invalid payload does not crash firmware. | Screenshot/log. |
| VAL-SW-008 | REQ-SW-013, REQ-SW-014 | Confirm release artifacts are produced. | Run tag/manual release workflow. | Signed binary, ELF, map, parser executable, schema, and examples are attached. | Release URL. |
| VAL-SW-009 | REQ-SW-015 | Confirm Arduino quick-start remains usable. | Open sketch in Arduino IDE and build/upload manually. | Sketch compiles and AP/dashboard appears. | Manual test note. |
| VAL-SW-010 | REQ-SW-016 | Confirm private credentials are not tracked. | Inspect `.gitignore` and search for local override file. | `vislog_private_config.h` is ignored and not committed. | Git status/search note. |

## Automated Gate Set

Required gates for merge/release:

- Parser and schema validation
- Build XIAO ESP32-C6 firmware

Advisory gates:

- Advisory Twister parser suite

## Manual Validation Checklist

Before a public release, perform these manual checks on hardware:

1. Upload or flash firmware to XIAO ESP32-C6.
2. Connect to the device AP.
3. Open the dashboard.
4. Confirm `GET /api/sample` returns JSON.
5. Confirm `frame` and `uptime_ms` increase.
6. Confirm radar values transition from `waiting`/`null` to sensor-derived values when the MR60BHA2 reports data.
7. Confirm LED manual control works.
8. Confirm session export produces valid JSON.

## Evidence Storage

Lightweight evidence can be stored as:

- CI links in release notes
- dashboard screenshots under `images/`
- protocol example files under `protocol/examples/`
- short validation notes in future release PRs

Do not commit private human-subject captures, unpublished clinical data, Wi-Fi secrets, or device-specific private credentials.
