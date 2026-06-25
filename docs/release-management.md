# Release Management

This document defines how this repository moves from reviewed software changes to tagged firmware/protocol releases.

## Release Scope

A release from this repository may include:

- signed XIAO ESP32-C6 Zephyr firmware binary
- Zephyr ELF and map files
- native simulator parser executable
- protocol JSON schema
- protocol JSON examples
- release notes

A release from this repository does not claim a finished wearable product, enclosure, battery system, thermal design, or clinical/medical validation.

## Version Format

Release tags use:

```text
vA.B.C
```

Meaning:

- `A`: major version, for breaking protocol, runtime, or workflow behavior
- `B`: minor version, for compatible features, major documentation/process improvements, or new artifact classes
- `C`: patch version, for fixes, small documentation updates, and CI maintenance

## Current Release Target

The next prepared software-platform release target is:

```text
v0.2.0
```

This version reflects the renamed runtime paths plus the added software architecture, requirements, validation, protocol, release-management, and project-process documentation.

## Required Checks Before Merge

Required branch/PR checks:

- `Parser and schema validation`
- `Build XIAO ESP32-C6 firmware`

Advisory checks:

- `Advisory Twister parser suite`

Do not require the advisory Twister check until it is hardened and consistently useful as a blocking gate.

## Release Procedure

1. Merge the reviewed PR into `main`.
2. Pull the updated `main` locally.
3. Create the version tag.
4. Push the tag.
5. Confirm the Release workflow publishes artifacts.

```sh
git checkout main
git pull
git tag v0.2.0
git push origin v0.2.0
```

The release workflow can also be run manually with the same tag value if manual dispatch is preferred.

## Release Verification Checklist

After the release workflow completes, verify:

- GitHub Release exists for the tag.
- Signed firmware binary is attached.
- ELF and map files are attached.
- Native simulator parser executable is attached.
- Protocol schema is attached.
- Protocol examples archive is attached.
- Release notes mention the commit/tag.

## Rollback / Recovery

If a release artifact is wrong:

1. Do not reuse the same tag for a different commit unless the release has not been consumed.
2. Prefer creating a patch release such as `v0.2.1` with the fix.
3. Document what changed in `CHANGELOG.md`.

## Release Responsibility

A release should point at reviewed code on `main`, not at an unmerged review branch. This keeps releases aligned with branch protection, review history, and CI evidence.
