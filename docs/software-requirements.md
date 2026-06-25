# Software Requirements

These requirements describe the base software platform in this repository. They are intentionally limited to firmware, protocol, validation, CI, release management, and developer workflow.

## Requirement Table

| ID | Requirement | Verification |
| --- | --- | --- |
| REQ-SW-001 | The Zephyr runtime shall build for `xiao_esp32c6/esp32c6/hpcore`. | GitHub Actions `Build XIAO ESP32-C6 firmware`. |
| REQ-SW-002 | The firmware shall parse MR60BHA2 UART frame envelopes. | Native parser executable test. |
| REQ-SW-003 | The parser shall decode saved heart-rate radar frames. | Parser fixture test: `heart_rate_frame.bin`. |
| REQ-SW-004 | The parser shall decode saved breathing-rate radar frames. | Parser fixture test: `breath_rate_frame.bin`. |
| REQ-SW-005 | The parser shall decode saved distance radar frames and normalize distance. | Parser fixture test: `distance_frame.bin`. |
| REQ-SW-006 | The parser shall decode saved presence frames. | Parser fixture test: `presence_frame.bin`. |
| REQ-SW-007 | The parser shall decode saved target-tracking frames. | Parser fixture test: `target_one_frame.bin`. |
| REQ-SW-008 | The runtime sample output shall use a documented JSON schema. | `protocol/sample.schema.json` plus CI schema validation. |
| REQ-SW-009 | Protocol examples shall validate against the schema. | GitHub Actions `Validate JSON schema examples`. |
| REQ-SW-010 | The runtime shall expose the latest sample through a local API endpoint. | Manual/API validation plan for `GET /api/sample`. |
| REQ-SW-011 | The dashboard shall provide local visualization and session export for engineering captures. | Manual dashboard validation plan. |
| REQ-SW-012 | The firmware shall support local status LED control and threshold-rule feedback. | Manual LED/API validation plan. |
| REQ-SW-013 | The release workflow shall verify a signed Zephyr firmware artifact exists. | GitHub Actions release workflow `Verify signed firmware artifact`. |
| REQ-SW-014 | The release workflow shall package firmware and protocol artifacts. | GitHub Release artifact inspection. |
| REQ-SW-015 | Arduino quick-start firmware shall remain available for first-boot hardware validation. | Documentation check and Arduino quick-start README. |
| REQ-SW-016 | Local private device credentials shall not be committed. | `.gitignore` and security documentation. |
| REQ-SW-017 | Twister coverage shall remain visible but advisory until the harness is hardened. | `Advisory Twister parser suite` CI job. |
| REQ-SW-018 | Mechanical, battery, thermal, enclosure, and wearable integration work shall remain out of scope for this software repository. | Software architecture scope statement. |

## Traceability Notes

- Requirements with automated verification should map to a CI job, test executable, schema check, or release artifact check.
- Requirements with manual verification should map to a named validation procedure in `docs/software-validation-plan.md` or `docs/dashboard-api-validation.md`.
- Hardware-product requirements should be tracked in a separate product/hardware repository, not in this software platform repository.

## Change Control

When behavior changes:

1. Update the requirement row if the intent changes.
2. Update the validation plan if the verification method changes.
3. Update protocol docs or examples if the JSON output changes.
4. Update release notes and changelog before tagging a release.
