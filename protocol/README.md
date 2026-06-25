# mmWaveVizLog Protocol

The protocol folder owns the sample data contract used by firmware, dashboard tooling, host scripts, and release artifacts.

## Files

| File | Purpose |
| --- | --- |
| `sample.schema.json` | JSON Schema for one sample message. |
| `examples/sample.json` | Canonical example sample payload. |
| `serial-jsonl.md` | Serial JSON Lines transport notes. |

## Message type

Current message type:

```json
{"type":"sample"}
```

A sample represents the latest bounded firmware state. It is not a raw radar packet dump.

## Transport

Supported transports use the same sample fields:

- USB serial JSON Lines at `115200` baud.
- Local HTTP endpoint `GET /api/sample`.
- Dashboard session logger export.

## Versioning

The protocol follows the repository release version. Current release candidate:

```text
0.2.0
```

Compatibility rules:

- Adding optional nullable fields is a minor-version change.
- Removing fields is a breaking change.
- Changing units is a breaking change unless a new field name is introduced.
- Example payloads and schema should change in the same pull request.

## Null and validity rules

- Unknown or currently unavailable readings should be `null`.
- Boolean validity fields should explain whether a value is trustworthy.
- A numeric zero should mean a real zero, not missing data.
- Source labels such as `waiting`, `sensor`, or `stale` help host tools interpret state.

## Units

| Field group | Unit |
| --- | --- |
| `heart_rate` | beats per minute |
| `breath_rate` | respirations per minute |
| `distance`, `target_distance`, `target_x`, `target_y` | metres |
| `target_angle` | degrees |
| `target_speed` | metres per second |
| `light` | lux |
| `uptime_ms`, `last_radar_ms` | milliseconds |

## Target array behavior

The `targets` array is frame-based. Target index is not a persistent identity. A person or object may switch from target 1 to target 2 between frames.
