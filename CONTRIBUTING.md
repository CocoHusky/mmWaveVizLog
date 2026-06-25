# Contributing

Thanks for improving mmWaveVizLog. This repository has two firmware paths:

- `zephyr/mr60bha2_console/` is the maintained product/runtime firmware path.
- `arduino/MR60BHA2_Sensor_VisLog/` is the quick-start bring-up and hardware-reference path.

## Pull Request Expectations

Before opening or merging a pull request:

1. Keep generated files out of Git.
2. Keep private credentials out of Git.
3. Update documentation when behavior, setup, or release artifacts change.
4. Keep protocol examples valid against `protocol/sample.schema.json`.
5. Let required CI pass before merging.

Required CI checks should include:

- parser build and parser executable test
- protocol JSON schema validation
- XIAO ESP32-C6 Zephyr firmware build

Twister coverage is useful, but it is currently treated as optional/non-blocking
until its harness path is hardened.

## Versioning

Release tags use `vA.B.C`:

- `A` means major version and should change for breaking behavior.
- `B` means minor version and should change for compatible new features.
- `C` means patch version and should change for fixes and small improvements.

For a release, keep these aligned when practical:

- Git tag, for example `v0.1.0`
- `zephyr/mr60bha2_console/VERSION`
- Arduino reference firmware version in `vislog_config.h`
- protocol example `console_fw` fields

## Release Checklist

1. Merge changes into `main` through a pull request.
2. Confirm required CI is green.
3. Create and push a version tag:

   ```sh
   git checkout main
   git pull
   git tag v0.1.0
   git push origin v0.1.0
   ```

4. Confirm the Release workflow publishes the signed XIAO ESP32-C6 firmware
   binary plus protocol artifacts.

## Known Follow-Up Work

These are useful next hardening steps after the current release cleanup:

- Pin Zephyr to a known-good commit once the project has a stable firmware baseline.
- Harden the Twister harness until it can become a required check again.
- Add more real captured MR60BHA2 frame fixtures for regression testing.
- Add UI/browser smoke testing for the local dashboard.
- Add a changelog once releases become regular.
