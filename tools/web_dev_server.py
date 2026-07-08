#!/usr/bin/env python3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from time import monotonic
from urllib.parse import parse_qs
import argparse
import json
import math
import sys


PROJECT_DIR = Path(__file__).resolve().parents[1]
WEB_DIR = PROJECT_DIR / "web"

DEFAULT_SETTINGS = {
    "targetC": 5.0,
    "hysteresisC": 0.1,
    "measurementIntervalMs": 500,
    "fanRunOnMs": 30000,
    "accessPointSsid": "CoolingController",
    "accessPointIp": "192.168.4.1",
    "stationSsid": "",
    "stationPassword": "",
    "stationPasswordSet": False,
    "stationConnected": False,
    "stationStatus": "disabled",
    "stationIp": "",
    "stationRssi": 0,
}

DEMO_NETWORKS = [
    {"ssid": "Kitchen WiFi", "rssi": -48, "channel": 6, "encrypted": True},
    {"ssid": "Workshop", "rssi": -66, "channel": 11, "encrypted": True},
    {"ssid": "Guest", "rssi": -78, "channel": 1, "encrypted": False},
]


class DashboardState:
    def __init__(self):
        self.started_at = monotonic() - 8 * 60
        self.settings = dict(DEFAULT_SETTINGS)

    def cooling_temperature(self, uptime_ms):
        target_c = float(self.settings["targetC"])
        elapsed_s = max(0.0, uptime_ms / 1000.0)
        # Deterministic cooling curve: high enough above target and steadily falling
        # so the browser-side prediction can fit a useful ETA in local dev.
        cooling_curve = 0.35 + 3.2 * math.exp(-elapsed_s / 620.0)
        ripple_scale = min(1.0, max(0.18, cooling_curve / 2.4))
        thermal_wave = 0.16 * ripple_scale * math.sin(elapsed_s / 52.0)
        sensor_ripple = 0.035 * ripple_scale * math.sin(elapsed_s / 11.0 + 0.8)
        return target_c + cooling_curve + thermal_wave + sensor_ripple

    def cooling_status(self, temperature_c):
        target_c = float(self.settings["targetC"])
        hysteresis_c = float(self.settings["hysteresisC"])
        peltier_running = temperature_c > target_c + hysteresis_c
        fan_run_on_active = not peltier_running and temperature_c > target_c + 0.05
        return {
            "peltierRunning": peltier_running,
            "fanRunning": peltier_running or fan_run_on_active,
            "fanRunOnActive": fan_run_on_active,
        }

    def status(self):
        uptime_ms = int((monotonic() - self.started_at) * 1000)
        temperature_c = self.cooling_temperature(uptime_ms)
        cooling = self.cooling_status(temperature_c)
        status = {
            "temperatureC": temperature_c,
            "filteredTemperatureC": temperature_c,
            "hasTemperature": True,
            "sensorDisconnected": False,
            "updateCount": uptime_ms // 1000,
            **cooling,
            "fanRunOnRemainingMs": 12000 if cooling["fanRunOnActive"] else 0,
        }

        return {
            **status,
            **self.public_settings(),
            "uptimeMs": uptime_ms,
        }

    def public_settings(self):
        settings = dict(self.settings)
        settings.pop("stationPassword", None)
        settings["stationPasswordSet"] = bool(self.settings["stationPassword"])
        settings["stationStatus"] = (
            "connected"
            if settings["stationConnected"]
            else ("disabled" if not settings["stationSsid"] else "disconnected")
        )
        return settings

    def save_settings(self, form):
        self.settings["targetC"] = read_float(form, "targetC", self.settings["targetC"])
        self.settings["hysteresisC"] = read_float(
            form, "hysteresisC", self.settings["hysteresisC"]
        )
        self.settings["measurementIntervalMs"] = read_int(
            form, "measurementIntervalMs", self.settings["measurementIntervalMs"]
        )
        self.settings["fanRunOnMs"] = read_int(
            form, "fanRunOnMs", self.settings["fanRunOnMs"]
        )
        self.settings["stationSsid"] = read_string(form, "stationSsid", "").strip()
        clear_password = read_bool(form, "clearStationPassword", False)
        station_password = read_string(form, "stationPassword", "")
        if clear_password or not self.settings["stationSsid"]:
            self.settings["stationPassword"] = ""
        elif station_password:
            self.settings["stationPassword"] = station_password

        self.settings["stationConnected"] = bool(self.settings["stationSsid"])
        self.settings["stationIp"] = "192.168.1.88" if self.settings["stationSsid"] else ""
        self.settings["stationRssi"] = -52 if self.settings["stationSsid"] else 0
        return {"ok": True, **self.public_settings()}

    def networks(self):
        return {
            "ok": True,
            "scanResult": len(DEMO_NETWORKS),
            "scanStatus": "complete",
            "scanMode": "active",
            "networks": DEMO_NETWORKS,
            "count": len(DEMO_NETWORKS),
        }

    def history(self):
        uptime_ms = int((monotonic() - self.started_at) * 1000)
        start_ms = max(0, uptime_ms - 30 * 60 * 1000)
        step_ms = 10000
        points = []
        target_c = float(self.settings["targetC"])
        hysteresis_c = float(self.settings["hysteresisC"])
        for sample_ms in range(start_ms, uptime_ms + 1, step_ms):
            temperature_c = self.cooling_temperature(sample_ms)
            cooling = self.cooling_status(temperature_c)
            points.append([
                sample_ms,
                round(temperature_c * 10),
                round(target_c * 10),
                0,
                1,
                0,
                round(hysteresis_c * 10),
                1 if cooling["peltierRunning"] else 0,
                1 if cooling["fanRunning"] else 0,
                1 if cooling["fanRunOnActive"] else 0,
            ])
        return {
            "ok": True,
            "historyMs": 2 * 60 * 60 * 1000,
            "sampleIntervalMs": step_ms,
            "hysteresisC": hysteresis_c,
            "fields": [
                "uptimeMs",
                "temperatureCx10",
                "targetCx10",
                "flags",
                "sensorValid",
                "sensorDisconnected",
                "hysteresisCx10",
                "peltierRunning",
                "fanRunning",
                "fanRunOnActive",
            ],
            "series": [
                {
                    "id": "probe1",
                    "label": "Probe 1",
                    "unit": "C",
                    "points": points,
                }
            ],
        }


def read_string(form, name, fallback):
    values = form.get(name)
    return values[0] if values else fallback


def read_bool(form, name, fallback):
    text = read_string(form, name, None)
    if text is None:
        return fallback
    return text.lower() in ("1", "true", "on")


def read_int(form, name, fallback):
    try:
        return int(read_string(form, name, fallback))
    except (TypeError, ValueError):
        return fallback


def read_float(form, name, fallback):
    try:
        return float(read_string(form, name, fallback))
    except (TypeError, ValueError):
        return fallback


class WebDevHandler(SimpleHTTPRequestHandler):
    state = DashboardState()

    def translate_path(self, path):
        clean_path = path.split("?", 1)[0].split("#", 1)[0]
        page = page_for_route(clean_path)
        if page is not None:
            return str(page)
        return str(WEB_DIR / clean_path.lstrip("/"))

    def do_GET(self):
        if self.path == "/api/status":
            self.send_json(self.state.status())
            return
        if self.path == "/api/settings":
            self.send_json({"ok": True, **self.state.public_settings()})
            return
        if self.path == "/api/history":
            self.send_json(self.state.history())
            return
        if self.path == "/api/networks":
            self.send_json(self.state.networks())
            return
        return super().do_GET()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8")
        form = parse_qs(body, keep_blank_values=True)
        if self.path == "/api/settings":
            self.send_json(self.state.save_settings(form))
            return
        self.send_error(404, "Not found")

    def send_json(self, payload):
        data = json.dumps(payload).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description="Local dashboard dev server")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument(
        "--no-port-fallback",
        action="store_true",
        help="fail instead of trying the next port when the requested one is busy",
    )
    args = parser.parse_args()

    server = bind_server(args.host, args.port, not args.no_port_fallback)
    host, port = server.server_address
    url = f"http://{host}:{port}/"
    pages = ", ".join(route for route in page_routes())
    print(f"Serving dashboard at {url}")
    print(f"Pages: {pages}")
    print("API: /api/status, /api/history, /api/settings, /api/networks")
    print("Press Ctrl+C to stop.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")


def bind_server(host, preferred_port, allow_fallback):
    last_error = None
    max_attempts = 20 if allow_fallback else 1
    for offset in range(max_attempts):
        port = preferred_port + offset
        try:
            return ThreadingHTTPServer((host, port), WebDevHandler)
        except OSError as error:
            last_error = error
            if error.errno not in (48, 98):
                break
            if not allow_fallback:
                break
            print(f"Port {port} is busy, trying {port + 1}...")

    print(
        f"Could not start web dev server on {host}:{preferred_port}: {last_error}",
        file=sys.stderr,
    )
    print(
        "Try another port, for example: python3 tools/web_dev_server.py --port 8090",
        file=sys.stderr,
    )
    raise SystemExit(1)


def page_for_route(path):
    if path in ("/", "/dashboard.html"):
        return WEB_DIR / "dashboard.html"
    if path.endswith(".html"):
        page = WEB_DIR / path.lstrip("/")
    else:
        page = WEB_DIR / f"{path.lstrip('/')}.html"
    try:
        page.resolve().relative_to(WEB_DIR)
    except ValueError:
        return None
    return page if page.is_file() else None


def page_routes():
    routes = ["/"]
    for page in sorted(WEB_DIR.glob("*.html")):
        if page.name == "dashboard.html":
            continue
        routes.append("/" + page.stem)
    return routes


if __name__ == "__main__":
    main()
