# Dashboard and API Validation

This document defines lightweight validation for the local dashboard and HTTP API. It is intended for engineering confidence in the base software platform, not finished product validation.

## Preconditions

- Firmware is running on the XIAO ESP32-C6.
- The board Wi-Fi access point is visible.
- A laptop or phone is connected to the device AP.
- Browser can open `http://192.168.4.1/`.

## API Checks

### API-001: Sample endpoint returns JSON

Request:

```sh
curl http://192.168.4.1/api/sample
```

Pass criteria:

- Response is valid JSON.
- Response has `type` set to `sample` if included by the active runtime path.
- Response includes firmware/status fields such as `console_fw`, `frame`, and `uptime_ms`.
- Response validates against `protocol/sample.schema.json` when captured as an example payload.

### API-002: Sample counters update

Procedure:

1. Request `/api/sample` twice at least one second apart.
2. Compare `frame` and `uptime_ms`.

Pass criteria:

- `uptime_ms` increases.
- `frame` increases or remains consistent with the active sample cadence.

### API-003: LED control accepts valid payload

Example payload:

```json
{
  "mode": "solid",
  "brightness": 0.35,
  "r": 40,
  "g": 190,
  "b": 100
}
```

Pass criteria:

- API accepts the request.
- LED changes to the requested state.
- Firmware continues serving `/api/sample`.

### API-004: LED rule mode accepts threshold payload

Example payload:

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

Pass criteria:

- Rule is accepted.
- LED behavior follows selected metric bands when data is available.
- Missing/stale metric data does not crash the firmware.

## Dashboard Checks

### UI-001: Dashboard loads

Pass criteria:

- Page renders without a browser error.
- Main radar, history, LED, and session logger sections are visible.

### UI-002: Plots update

Pass criteria:

- At least one graph updates over time.
- Reset plots clears graph history.
- Noise/stability band control can be changed without breaking the plot.

### UI-003: Session logger exports JSON

Pass criteria:

- Named session can start and stop.
- Export produces a JSON file.
- Exported samples are parseable JSON.

### UI-004: Stale/offline states are visible

Pass criteria:

- Dashboard does not misrepresent missing data as valid sensor output.
- `waiting`, `stale`, `null`, or equivalent status appears when sensor data is unavailable.

## Evidence

Acceptable evidence:

- screenshot of dashboard
- saved `/api/sample` response
- short notes in release PR
- exported sample file with private data removed

Do not commit private captures that identify people, locations, or unpublished human-subject data.
