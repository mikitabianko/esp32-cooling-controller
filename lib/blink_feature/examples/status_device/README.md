# Status device example

This product composition uses `blink_feature` with the shared ESP32 clock,
Preferences settings backend, AP/STA network manager, web host, OTA services,
firmware upload service, and telemetry registry. It has no cooling dependency.

Build it from the repository root:

```sh
pio run -d lib/blink_feature/examples/status_device
```

The device starts the `StatusDevice` access point with password `status123` and
serves its dashboard at `/`, `/dashboard.html`, and `/apps/status/dashboard`.
