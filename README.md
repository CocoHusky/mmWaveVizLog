# MR60BHA2 Sensor VisLog

PlatformIO firmware and a browser dashboard for the Seeed MR60BHA2 radar on a XIAO ESP32-C6.

The project is aimed at two short-range sensing use cases:

- rear-seat or baby monitoring: presence, target count, movement toward or away, and speed
- bedside physiology monitoring: breathing rate, heart rate, and motion phase

## Hardware

1. Seeed Studio 60 GHz mmWave sensor module pack
2. USB-C to USB-C cable
3. USB-C battery pack
4. Phone stand

![Hardware overview](docs/images/08-hardware-overview.jpg)

![Mounted side view](docs/images/09-mounted-side-view.png)

## Install First

### Arduino IDE

1. Select `XIAO ESP32C6` and the correct port.
2. Use the board settings shown below if Arduino picks the wrong profile.
3. Upload the sketch with the board connected.
4. Use the open-enclosure programming setup as the physical wiring reference.

![Arduino IDE board settings](docs/images/07-arduino-board-settings.png)

![Arduino IDE upload](docs/images/06-arduino-ide-upload.png)

![Arduino programming setup](docs/images/10-open-enclosure.png)

### Firmware Values

Use these exact values in the firmware and the setup notes:

```cpp
static const char *WIFI_AP_SSID = "mmWaveVisLog-MR60BHA2";
static const char *WIFI_AP_PASSWORD = "wirelessphysiology";
static const char *OTA_HOSTNAME = "mmWaveVisLog-MR60BHA2-OTA";
static const char *OTA_PASSWORD = "wp-ota";
static const char *VisLog_FW_VERSION = "2.1.4";
```

Open the UI at `http://192.168.4.1/`.

### PlatformIO

If you are using PlatformIO instead of Arduino IDE:

```sh
cd firmware/mr60bha2_console
pio run
pio run --target upload
pio run --target uploadfs
pio device monitor
```

The images in `docs/images/` were re-saved without embedded EXIF or location metadata.

## Live Views

### Radar target tracking

![Radar target tracking](docs/images/01-radar-target-tracking-single.png)

Use this as the main live view for a single subject. It shows presence, range, angle, speed, and the breathing and heart traces.

### Multi-target tracking

![Multi-target tracking](docs/images/02-radar-target-tracking-multi.png)

Use this view when more than one person is in range. It is the best reference for rear-seat occupancy or room monitoring.

### Range, angle, and speed history

![Range, angle, and speed history](docs/images/05-range-angle-speed-traces.png)

Use this section to watch whether a target is stable, moving closer, or moving farther away.

## Control and Logging

### LED control and threshold rules

![LED control and rule mode](docs/images/03-led-control-and-rule.png)

This is the local status LED control surface. It can run manually or follow a sensor threshold rule.

### Session logger

![Session logger](docs/images/04-session-logger.png)

This records named sessions and exports JSON for later review.

## Repository Layout

```text
firmware/mr60bha2_console/
  platformio.ini
  src/main.cpp
  data/index.html
quicksetup/
  legacy Arduino sketch and notes
docs/images/
  screenshots and setup photos used above
```
