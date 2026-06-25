# Software Requirements

This is a lightweight requirements trace for the firmware and data platform. It is not a regulated-design-control file, but it shows how repository behavior is linked to verification.

| ID | Requirement | Verification |
| --- | --- | --- |
| REQ-SW-001 | Firmware shall parse MR60BHA2 UART frames into validated packets. | Native simulator parser tests with saved fixtures. |
| REQ-SW-002 | Firmware shall preserve a bounded latest-sample state for sensor outputs. | Firmware build and sample-output review. |
| REQ-SW-003 | Firmware shall expose schema-compatible sample data. | `protocol/sample.schema.json` validation against examples. |
| REQ-SW-004 | Firmware shall build for XIAO ESP32-C6. | `Build XIAO ESP32-C6 firmware` CI job. |
| REQ-SW-005 | Firmware shall produce a signed Zephyr firmware artifact for releases. | Release workflow verifies `zephyr.signed.bin`. |
| REQ-SW-006 | The repository shall keep a quick-start Arduino path for hardware bring-up. | Arduino quickstart documentation and source path. |
| REQ-SW-007 | The repository shall document the supported protocol fields and units. | Protocol README and schema. |
| REQ-SW-008 | Pull requests shall identify affected firmware/protocol/release areas. | Pull request template checklist. |
| REQ-SW-009 | Validation work shall map to a requirement, test method, and pass/fail criteria. | Validation issue template and software validation plan. |
| REQ-SW-010 | Product hardware, enclosure, battery, and thermal work shall remain out of scope for this repo unless it changes software interfaces. | Software architecture scope statement. |

## Change-control expectation

Any change that modifies a sample field, transport behavior, release artifact, or firmware module boundary should update at least one of:

- `protocol/README.md`
- `protocol/sample.schema.json`
- `docs/software-validation-plan.md`
- `docs/firmware-module-map.md`
- `CHANGELOG.md`
