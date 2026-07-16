#!/usr/bin/env python3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from time import monotonic
from urllib.parse import parse_qs
import argparse
import copy
import json
import math
import sys


PROJECT_DIR = Path(__file__).resolve().parents[1]
WEB_DIR = PROJECT_DIR / "web"
FIXTURE_DIR = PROJECT_DIR / "test" / "fixtures" / "api"


def load_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


STATUS_FIXTURE = load_json(FIXTURE_DIR / "status.json")
SETTINGS_FIXTURE = load_json(FIXTURE_DIR / "settings.json")
HISTORY_FIXTURE = load_json(FIXTURE_DIR / "history.json")
BLINK_STATUS_FIXTURE = load_json(FIXTURE_DIR / "blink_status.json")
BLINK_SETTINGS_FIXTURE = load_json(FIXTURE_DIR / "blink_settings.json")


def load_applications():
    applications = []
    for manifest_path in sorted(WEB_DIR.glob("apps/*/manifest.json")):
        manifest = load_json(manifest_path)
        missing = [field for field in ("id", "title", "entry") if not manifest.get(field)]
        if missing:
            raise ValueError(f"{manifest_path}: missing fields: {', '.join(missing)}")
        entry = manifest["entry"]
        if not entry.startswith("/") or entry.endswith(".html"):
            raise ValueError(
                f"{manifest_path}: entry must be an absolute extensionless route"
            )
        page = web_path(entry + ".html")
        if page is None:
            raise ValueError(f"{manifest_path}: entry page does not exist: {entry}.html")
        applications.append({**manifest, "page": page})
    if not applications:
        raise ValueError("No web application manifests found under web/apps/*")
    return applications


def web_path(url_path):
    path = (WEB_DIR / url_path.lstrip("/")).resolve()
    try:
        path.relative_to(WEB_DIR)
    except ValueError:
        return None
    return path if path.is_file() else None


APPLICATIONS = load_applications()

DEMO_NETWORKS = [
    {"ssid": "Kitchen WiFi", "rssi": -48, "channel": 6, "encrypted": True},
    {"ssid": "Workshop", "rssi": -66, "channel": 11, "encrypted": True},
    {"ssid": "Guest", "rssi": -78, "channel": 1, "encrypted": False},
]


class DashboardState:
    def __init__(self):
        self.started_at = monotonic() - 8 * 60
        self.settings = copy.deepcopy(SETTINGS_FIXTURE)
        self.settings["stationPassword"] = ""
        self.blink_started_at = monotonic()
        self.blink_settings = copy.deepcopy(BLINK_SETTINGS_FIXTURE)
        self.blink_settings["stationPassword"] = ""

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
        status = copy.deepcopy(STATUS_FIXTURE)
        public_settings = self.public_settings()
        public_settings.pop("ok", None)
        status.update(public_settings)
        status.update({
            "temperatureC": temperature_c,
            "filteredTemperatureC": temperature_c,
            "hasTemperature": True,
            "sensorDisconnected": False,
            "updateCount": uptime_ms // 1000,
            **cooling,
            "fanRunOnRemainingMs": 12000 if cooling["fanRunOnActive"] else 0,
            "uptimeMs": uptime_ms,
        })
        return status

    def public_settings(self):
        settings = copy.deepcopy(self.settings)
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
        history = copy.deepcopy(HISTORY_FIXTURE)
        history["hysteresisC"] = hysteresis_c
        history["series"][0]["points"] = points
        return history

    def public_blink_settings(self):
        settings = copy.deepcopy(self.blink_settings)
        settings.pop("stationPassword", None)
        settings["stationPasswordSet"] = bool(
            self.blink_settings["stationPassword"]
        )
        settings["stationStatus"] = (
            "connected"
            if settings["stationConnected"]
            else ("disabled" if not settings["stationSsid"] else "disconnected")
        )
        return settings

    def blink_status(self):
        uptime_ms = int((monotonic() - self.blink_started_at) * 1000)
        interval_ms = int(self.blink_settings["blinkIntervalMs"])
        toggle_count = uptime_ms // max(1, interval_ms)
        status = copy.deepcopy(BLINK_STATUS_FIXTURE)
        public_settings = self.public_blink_settings()
        public_settings.pop("ok", None)
        status.update(public_settings)
        status.update(
            {
                "ledOn": toggle_count % 2 == 1,
                "toggleCount": toggle_count,
                "uptimeMs": uptime_ms,
            }
        )
        return status

    def save_blink_settings(self, form):
        self.blink_settings["deviceName"] = read_string(
            form, "deviceName", self.blink_settings["deviceName"]
        ).strip()[:32] or "Status Device"
        self.blink_settings["blinkIntervalMs"] = max(
            100,
            min(
                60000,
                read_int(
                    form,
                    "blinkIntervalMs",
                    self.blink_settings["blinkIntervalMs"],
                ),
            ),
        )
        self.blink_settings["stationSsid"] = read_string(
            form, "stationSsid", ""
        ).strip()
        forget_network = read_bool(form, "forgetStationNetwork", False)
        station_password = read_string(form, "stationPassword", "")
        if forget_network or not self.blink_settings["stationSsid"]:
            self.blink_settings["stationSsid"] = ""
            self.blink_settings["stationPassword"] = ""
        elif station_password:
            self.blink_settings["stationPassword"] = station_password
        connected = bool(self.blink_settings["stationSsid"])
        self.blink_settings["stationConnected"] = connected
        self.blink_settings["stationIp"] = "192.168.1.89" if connected else ""
        self.blink_settings["stationRssi"] = -54 if connected else 0
        return self.public_blink_settings()


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
        asset = web_path(clean_path)
        return str(asset if asset is not None else WEB_DIR / "__not_found__")

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
        if self.path == "/api/blink/status":
            self.send_json(self.state.blink_status())
            return
        if self.path == "/api/blink/settings":
            self.send_json(self.state.public_blink_settings())
            return
        return super().do_GET()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8")
        form = parse_qs(body, keep_blank_values=True)
        if self.path == "/api/settings":
            self.send_json(self.state.save_settings(form))
            return
        if self.path == "/api/blink/settings":
            self.send_json(self.state.save_blink_settings(form))
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
    print(
        "API: /api/status, /api/history, /api/settings, /api/networks, "
        "/api/blink/status, /api/blink/settings"
    )
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
    for index, application in enumerate(APPLICATIONS):
        routes = {application["entry"], application["entry"] + ".html"}
        if index == 0:
            routes.update(("/", "/dashboard.html"))
        if path in routes:
            return application["page"]
    return None


def page_routes():
    routes = ["/", "/dashboard.html"]
    for application in APPLICATIONS:
        routes.append(application["entry"])
    return routes


if __name__ == "__main__":
    main()
