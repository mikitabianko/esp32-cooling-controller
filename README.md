# ESP32 Cooling Controller

PlatformIO firmware for an ESP32-based cooling controller with a DS18B20
temperature probe, SSD1306 OLED display, relay outputs, persistent settings,
and an embedded web dashboard.

The controller is designed for a small Peltier cooling setup: it switches the
Peltier relay according to a target temperature and hysteresis band, keeps the
fan running during cooling, and supports a configurable fan run-on time after
the Peltier element turns off.

## Prerequisites

- Python 3 for PlatformIO scripts and the local dashboard development server.
- PlatformIO Core, or VS Code with the PlatformIO extension.
- An ESP32 board compatible with the `wemos_d1_mini32` PlatformIO environment.
- A USB cable with data support and, if required by your board, the matching USB
  serial driver.
- The project hardware: DS18B20 probe, SSD1306 OLED, relay module, and the
  DS18B20 pull-up resistor.

PlatformIO downloads the Arduino framework and declared libraries during the
first build.

## Features

- DS18B20 temperature measurement with non-blocking conversion timing.
- Peltier and fan relay control, including a short startup self-test.
- Hysteresis-based cooling logic:
  - cooling starts above `target + hysteresis`;
  - cooling stops once the temperature reaches `target`;
  - the fan can continue running for a configurable run-on period.
- Safety behavior for a disconnected DS18B20 sensor (`-127 C`): Peltier output
  is disabled.
- SSD1306 OLED status display with temperature, relay state, settings, timing,
  and network pages.
- Embedded web dashboard for live status, settings, and a development/simulation
  view.
- Persistent settings stored in ESP32 Preferences/NVS.
- Native Unity tests for domain logic and generated dashboard embedding.

## Hardware

| Component | ESP32 pin |
| --- | --- |
| SSD1306 SDA | GPIO 21 |
| SSD1306 SCL | GPIO 22 |
| DS18B20 data | GPIO 26 |
| Peltier relay IN1 | GPIO 17 |
| Fan relay IN2 | GPIO 16 |

The DS18B20 data line expects the usual pull-up resistor between data and VCC.
The relay module is configured as active-low (`LOW` = on, `HIGH` = off).

## Default Settings

| Setting | Default | Allowed range |
| --- | ---: | ---: |
| Target temperature | `5.0 C` | `-20.0 C` to `40.0 C` |
| Hysteresis | `0.1 C` | `0.1 C` to `10.0 C` |
| Measurement interval | `500 ms` | `250 ms` to `60000 ms` |
| Fan run-on time | `30000 ms` | `0 ms` to `600000 ms` |

Wi-Fi defaults:

| Setting | Value |
| --- | --- |
| Access point SSID | `CoolingController` |
| Access point password | `cooling123` |
| Web server port | `80` |

The access point is always started. Station mode can be configured from the
dashboard by saving a station SSID and password.

## Project Structure

```text
include/
  app/        Application orchestration interfaces
  config/     Pins, defaults, and bounds
  domain/     Testable cooling logic and settings model
  hardware/   DS18B20 and relay adapters
  storage/    Persistent settings via Preferences
  ui/         OLED display view
  web/        Dashboard server and generated HTML page header
src/
  app/        Application orchestration implementation
  domain/     Logic without direct hardware access
  hardware/   Concrete ESP32, sensor, and relay access
  storage/    Preferences integration
  ui/         OLED rendering
  web/        HTTP routes, JSON responses, and Wi-Fi handling
web/          Editable dashboard HTML, CSS, and JavaScript sources
tools/        Build and local development helpers
test/         Unity tests
docs/         Project notes and media
```

New modules should be placed by responsibility. Code in `domain/` should stay
independent of concrete hardware libraries where possible, so it remains easy to
test locally.

## Web Dashboard

The editable dashboard sources live in `web/`. During a PlatformIO build,
`tools/embed_dashboard.py` inlines shared CSS/JavaScript assets and regenerates
`include/web/WebDashboardPage.h`.

Built-in routes:

| Route | Purpose |
| --- | --- |
| `/` or `/dashboard.html` | Main dashboard |
| `/dev` or `/dev.html` | Local/device development state view |
| `/api/status` | Live status JSON |
| `/api/settings` | Read or save controller settings |
| `/api/dev` | Read or save simulated dashboard state |

To add a dashboard page:

1. Create `web/example.html`.
2. Include shared assets such as `/assets/app.css` and `/assets/app.js`.
3. Optionally add a page-specific script, for example `/assets/example.js`.

The local route becomes `/example`, and the page is embedded automatically on
the next PlatformIO build.

For local dashboard development without an ESP32:

```sh
python3 tools/web_dev_server.py
```

The dashboard is served at `http://127.0.0.1:8080/` by default. If the port is
busy, the helper tries the next available port unless `--no-port-fallback` is
used. The local server simulates the API endpoints used by the firmware.

## Build, Upload, and Monitor

```sh
pio run
pio run --target upload
pio device monitor
```

The default PlatformIO environment is `wemos_d1_mini32`.

## Tests

Run the native test suite:

```sh
pio test -e native
```

The current tests cover the pure temperature/control helpers and verify that the
dashboard HTML assets are embedded into the generated firmware header.

## Notes

- Settings submitted through the dashboard are sanitized before use and before
  being stored.
- The OLED startup screen shows the access point IP and relay self-test state.
- If the DS18B20 reports the disconnected marker, the UI shows a sensor error
  and the Peltier relay remains off.
