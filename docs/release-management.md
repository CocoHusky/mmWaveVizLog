# Release Management

This repository uses pull-request review and CI gates to keep firmware, protocol, and release artifacts aligned.

## Branching

- `main` is the release branch.
- Feature and documentation changes should branch from `main`.
- Pull requests should stay small enough to review.

## Required checks

Required before merge:

- `Parser and schema validation`
- `Build XIAO ESP32-C6 firmware`

Advisory:

- `Advisory Twister parser suite`

## Version tags

Tags use `vA.B.C`.

- `A`: breaking protocol, hardware, or runtime behavior.
- `B`: compatible feature or documentation/process release.
- `C`: bug fix, CI fix, or documentation correction.

This documentation/process release candidate is `v0.2.0`.

## Release checklist

1. Merge the release PR into `main` after required CI passes.
2. Confirm these files align:
   - `zephyr/mmwavevizlog-runtime/VERSION`
   - `arduino/mmwavevizlog-quickstart/vislog_config.h`
   - `protocol/examples/sample.json`
   - `CHANGELOG.md`
3. Create the release tag:

   ```sh
   git checkout main
   git pull
   git tag v0.2.0
   git push origin v0.2.0
   ```

4. Confirm the Release workflow publishes:
   - signed XIAO ESP32-C6 Zephyr firmware binary
   - Zephyr ELF and map file
   - native simulator parser executable
   - protocol schema
   - protocol examples
5. Download the release artifact and confirm files are named with the release tag.

## Current tool limitation note

The connected GitHub tool used by the assistant can edit files, open pull requests, and inspect CI, but it does not currently expose a GitHub Release creation action or a tag-creation action. Final tag and release publication should therefore be performed through the repository UI or local Git command after PR merge.
