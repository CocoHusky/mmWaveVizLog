# Protocol

The protocol folder defines the software contract between firmware, host tools, dashboard/API consumers, and release artifacts.

## Files

| File | Purpose |
| --- | --- |
| `sample.schema.json` | JSON Schema for sample payloads. |
| `examples/` | Example sample payloads that should validate against the schema. |
| `serial-jsonl.md` | Serial JSON Lines transport notes. |
| `README.md` | Protocol overview and compatibility expectations. |

## Message Types

### Sample message

Sample messages describe the latest available sensing state from the firmware.

Common fields include:

- `heart_rate`
- `breath_rate`
- `distance`
- `presence`
- `target_count`
- `targets`
- `light`
- `console_fw`
- `frame`
- `uptime_ms`

Fields may be `null` when the runtime has not received a valid value yet.

## Transports

The same logical sample model can be transported over:

- USB serial JSON Lines
- local HTTP endpoint such as `GET /api/sample`
- browser session export from the dashboard

## Versioning

Protocol compatibility follows the release tag where practical.

Recommended behavior:

- Patch versions should not remove or rename fields.
- Minor versions may add fields in a backward-compatible way.
- Major versions may rename or remove fields, but should include migration notes.
- Unknown fields should be ignored by consumers unless a strict validation mode is required.

## Units and Null Handling

- Heart rate: beats per minute.
- Breathing rate: respirations per minute.
- Distance: meters when normalized by firmware.
- Raw distance: raw radar-reported distance before normalization.
- Target speed: meters per second when available.
- Ambient light: lux.
- `null`: value unavailable, stale, not yet reported, or not supported by the active runtime path.

## Target Array Behavior

Targets are frame-based reports from the radar. `Target 1`, `Target 2`, and `Target 3` should not be treated as persistent human identities unless a future tracking layer explicitly adds identity association.

## Validation

CI should validate every file under `protocol/examples/*.json` against `protocol/sample.schema.json`.

Manual captures added to this repo should be scrubbed of private or identifying information before commit.
