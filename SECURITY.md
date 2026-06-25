# Security Policy

## Supported Use

mmWaveVizLog is a public research and prototyping repository for local sensing,
firmware bring-up, and visualization around the Seeed MR60BHA2 and XIAO
ESP32-C6. Do not treat the repository defaults as production security settings.

## Reporting Security Issues

Please do not open a public issue with active exploit details, private network
credentials, device-specific secrets, or unpublished vulnerability details.

Instead, contact the maintainer through the GitHub profile for this repository
or open a minimal issue that says a private security report is available.

## Device Credentials

The Arduino quick-start path includes development default Wi-Fi and OTA
passwords so a new device can boot quickly. For any shared, field, demo, or
longer-term device, copy:

```text
arduino/mmwavevizlog-quickstart/vislog_private_config.example.h
```

to:

```text
arduino/mmwavevizlog-quickstart/vislog_private_config.h
```

Then change the Wi-Fi and OTA passwords. The private override file is ignored by
Git.

## Public Repository Hygiene

Do not commit:

- private Wi-Fi credentials
- OTA passwords used on shared devices
- private sensor captures that identify people or locations
- unpublished medical or clinical data
- generated Zephyr workspaces or build products

## Firmware and OTA Notes

Signed firmware artifacts are produced by the Zephyr release workflow. Treat OTA
as a convenience feature for prototype devices, not as a hardened fleet update
system.
