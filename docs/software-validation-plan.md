# Software Validation Plan

This plan focuses on software, firmware, protocol, and release behavior. It intentionally excludes wearable enclosure, power, charging, battery, thermal, and mechanical validation.

| ID | Purpose | Method | Pass criteria | Evidence |
| --- | --- | --- | --- | --- |
| VAL-001 | Parser accepts valid MR60BHA2 fixtures. | Run native simulator parser tests. | All fixture tests pass. | CI log. |
| VAL-002 | Parser handles partial frames. | Run native simulator parser tests. | Partial-frame test passes without false frame-ready result. | CI log. |
| VAL-003 | JSON examples match protocol schema. | Run schema validator in CI. | All files under `protocol/examples/` validate. | CI log. |
| VAL-004 | XIAO ESP32-C6 firmware builds. | Run Zephyr CI firmware job. | `zephyr.signed.bin` exists. | CI artifact/check log. |
| VAL-005 | Release workflow packages artifacts. | Run release workflow from tag or dispatch. | Signed binary, ELF, map, parser executable, schema, and examples are uploaded. | GitHub Release assets. |
| VAL-006 | Dashboard API exposes latest sample. | Connect to device AP and call `GET /api/sample`. | Response parses as JSON and matches schema. | Manual test note or future smoke test. |
| VAL-007 | LED rule endpoint accepts valid payload. | POST valid LED rule JSON. | LED state changes or endpoint reports success without crash. | Manual test note or future smoke test. |
| VAL-008 | Invalid/missing data is represented safely. | Review stale/null sample behavior. | Missing readings remain `null` or flagged invalid; zero is not used as missing data. | Code review and sample examples. |
| VAL-009 | OTA path is release-ready. | Verify release artifact and OTA docs. | Signed firmware artifact exists and OTA setup is documented. | Release workflow log. |
| VAL-010 | Advisory checks do not block required gates. | Review CI configuration. | Twister can warn while parser/schema and firmware gates remain required. | CI job status. |

## Required CI gates

- Parser and schema validation.
- XIAO ESP32-C6 firmware build.

## Advisory checks

- Twister parser suite until the harness is stable enough to require.

## Manual smoke checklist

1. Flash or upload firmware.
2. Connect to the device AP.
3. Open `http://192.168.4.1/`.
4. Confirm `GET /api/sample` returns JSON.
5. Confirm `frame` or `uptime_ms` advances.
6. Toggle LED solid color.
7. Enable one threshold rule and confirm no dashboard crash.
8. Export one session logger capture.
