# ESP32 CalDAV Calendar - E-Paper calendar display with CalDAV support

[![License](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](https://opensource.org/license/gpl-3-0/)

## Table of Contents

- [ESP32 CalDAV Calendar - E-Paper calendar display with CalDAV support](#esp32-caldav-calendar---e-paper-calendar-display-with-caldav-support)
  - [Table of Contents](#table-of-contents)
  - [About](#about)
    - [Technical features](#technical-features)
  - [Before you start](#before-you-start)
    - [Build the firmware](#build-the-firmware)
    - [Configure the settings](#configure-the-settings)
    - [Flash the firmware](#flash-the-firmware)
  - [Settings format](#settings-format)
    - [CalDAV settings](#caldav-settings)
    - [WiFi settings](#wifi-settings)
    - [System settings](#system-settings)
  - [Directory structure](#directory-structure)
  - [Resources](#resources)
  - [Maintainer](#maintainer)

## About

Open-Source ESP32-S3 based e-paper calendar display with [CalDAV](https://en.wikipedia.org/wiki/CalDAV) support. The device periodically wakes from deep sleep, connects to your WiFi network, fetches upcoming calendar events from a CalDAV server, renders them on an e-paper display using LVGL and then returns to deep sleep.

### Technical features

- ESP32-S3 MCU with OPI PSRAM
- 16 MB flash, LittleFS filesystem
- E-paper display driven by the [FastEPD](https://github.com/bitbank2/FastEPD) library
- Capacitive touch input via GT911 controller
- CalDAV calendar integration (multiple calendars supported)
- Two configurable display views
  - **List view** – Agenda-style list of upcoming events
  - **Calendar view** – Monthly calendar grid with event indicators
- WiFi (STA) with configurable retry logic
- NTP time synchronisation with configurable server and interval
- RTC backup via BM8563 (I²C) to keep time across deep sleep cycles
- Battery voltage monitoring via ADC with EMA filtering and outlier rejection
- Deep sleep with configurable wake interval to minimise power consumption
- Settings stored in NVS, loaded from a JSON file on first boot

## Before you start

Clone the repository and initialise the submodules:

```sh
git clone https://github.com/Kampi/ESP32-Calendar.git
cd ESP32-Calendar
git submodule update --init --recursive
```

### Build the firmware

The firmware is built with [PlatformIO](https://platformio.org/). To build the debug configuration run:

```sh
platformio run --environment debug
```

For a release build use:

```sh
platformio run --environment release
```

### Configure the settings

Before flashing, create your `data/settings_default.json` based on the template that is already provided in the repository. Fill in your CalDAV server URL, credentials, WiFi details and the desired system parameters. See the [Settings format](#settings-format) section for a full description of every field.

### Flash the firmware

After a successful build, flash the firmware and the LittleFS filesystem image in one step:

```sh
platformio run --environment debug --target upload
platformio run --environment debug --target uploadfs
```

> **NOTE**
> Make sure the correct serial port is selected. You can override the default port by setting the environment variable `PIO_UPLOAD_PORT` before running the commands above.

> **NOTE**
> The filesystem image must be uploaded at least once. After the first boot the device reads `settings_default.json` from LittleFS, stores the values in NVS and deletes the file. Subsequent boots load settings directly from NVS.

## Settings format

The settings are provided as a JSON file located at `data/settings_default.json`. The file is read once on first boot and the values are persisted in NVS flash. The top-level object contains three sections: `caldav`, `wifi` and `system`.

```json
{
    "caldav": {
        "url": "https://your-caldav-server/remote.php/dav/calendars/user/",
        "username": "your-username",
        "password": "your-password",
        "timeout": 30,
        "calendars": [
            "personal",
            "work"
        ]
    },
    "wifi": {
        "ssid": "your-ssid",
        "password": "your-wifi-password",
        "maxRetries": 5,
        "timeout": 5000
    },
    "system": {
        "timezone": "CET-1CEST,M3.5.0,M10.5.0/3",
        "ntp-timeout": 5,
        "ntp-server": "pool.ntp.org",
        "ntp-sync-interval": 2,
        "sleep-hours": 3
    }
}
```

### CalDAV settings

| Key | Type | Description |
| --- | ---- | ----------- |
| `url` | `string` | Full URL of the CalDAV principal or calendar home set. |
| `username` | `string` | CalDAV account username. |
| `password` | `string` | CalDAV account password. |
| `timeout` | `number` | HTTP request timeout in **seconds**. |
| `calendars` | `array<string>` | List of calendar display names to fetch events from. All named calendars must exist on the server. |

### WiFi settings

| Key | Type | Description |
| --- | ---- | ----------- |
| `ssid` | `string` | SSID of the WiFi network to connect to. |
| `password` | `string` | WPA/WPA2 passphrase. Leave empty for open networks. |
| `maxRetries` | `number` | Maximum number of connection attempts before giving up and entering deep sleep. |
| `timeout` | `number` | Connection timeout in **milliseconds** per attempt (`0` = wait indefinitely). |

### System settings

| Key | Type | Description |
| --- | ---- | ----------- |
| `timezone` | `string` | POSIX timezone string (e.g. `CET-1CEST,M3.5.0,M10.5.0/3` for Central European Time). A full list of valid strings can be found in the [ESP-IDF timezone list](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv). |
| `ntp-server` | `string` | Hostname or IP address of the NTP server. |
| `ntp-timeout` | `number` | Maximum time in **seconds** to wait for an NTP response. |
| `ntp-sync-interval` | `number` | How often the device performs a full NTP synchronisation, in **days** (`0` = sync on every wake). |
| `sleep-hours` | `number` | Duration of the deep sleep interval in **hours** between display refreshes. |

## Directory structure

- `components`: External ESP-IDF components used by the project
  - `ESP32-CalDAV`: CalDAV client library for ESP32
  - `FastEPD`: E-paper display driver
- `data`: Filesystem content uploaded to the device via LittleFS
  - `settings_default.json`: Default settings template (see [Settings format](#settings-format))
- `docs`: Project documentation
- `main`: Application source code
  - `Devices`: Low-level peripheral drivers (ADC, I²C, RTC)
  - `EventManager`: FreeRTOS event bus
  - `Export/fonts`: Custom LVGL font files
  - `Settings`: Settings manager (NVS + JSON loader)
  - `SNTP`: NTP synchronisation helper
  - `TimeManager`: Time management and RTC abstraction
  - `UI`: LVGL screen definitions (dashboard, day view, status bar)
  - `WiFi`: WiFi driver and connection management
- `scripts`: PlatformIO helper scripts (code formatting, clean)

## Resources

- [CalDAV RFC 4791](https://www.rfc-editor.org/rfc/rfc4791)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [LVGL Documentation](https://docs.lvgl.io/)
- [FastEPD Library](https://github.com/bitbank2/FastEPD)
- [POSIX Timezone Database](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)
- [PlatformIO Documentation](https://docs.platformio.org/)

## Maintainer

- [Daniel Kampert](mailto:DanielKampert@kampis-elektroecke.de)
