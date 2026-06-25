# Dashboard and API Validation

This document defines the software smoke checks for the local dashboard and API. It is intentionally separate from wearable product validation.

## API sample endpoint

Endpoint:

```text
GET /api/sample
```

Expected checks:

1. HTTP response succeeds.
2. Body parses as JSON.
3. `type` is `sample`.
4. `uptime_ms` is present and numeric.
5. `frame` is present and numeric.
6. Optional sensor values use `null` when unavailable.
7. The response matches `protocol/sample.schema.json`.

## LED endpoint

Endpoint:

```text
POST /api/led
```

Solid-color payload:

```json
{"mode":"solid","brightness":0.35,"r":40,"g":190,"b":100}
```

Threshold-rule payload:

```json
{"mode":"rule","metric":"target_speed","low":-0.05,"high":0.05,"low_r":255,"low_g":0,"low_b":0,"ok_r":0,"ok_g":0,"ok_b":0,"high_r":0,"high_g":80,"high_b":255}
```

Expected checks:

1. Valid payload is accepted.
2. Firmware remains responsive after update.
3. `GET /api/sample` still returns valid JSON after LED update.
4. Invalid payloads should not crash the firmware.

## Dashboard smoke checks

1. Page loads at `http://192.168.4.1/`.
2. Radar target tracking panel renders.
3. Multi-target panel renders.
4. Plot controls render.
5. Session logger can start and export JSON.
6. LED controls send a request without freezing the page.

## Future automation

A future host-side smoke test can read `GET /api/sample`, validate the JSON schema, post a benign LED payload, and record the result as a CI/manual validation artifact.
