#!/usr/bin/env python3
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from time import monotonic
from urllib.parse import parse_qs
import argparse
import json
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

DEFAULT_DEV_STATE = {
    "ok": True,
    "enabled": False,
    "temperatureC": 5.0,
    "hasTemperature": True,
    "sensorDisconnected": False,
    "updateCount": 0,
    "peltierRunning": False,
    "fanRunning": False,
    "fanRunOnActive": False,
    "fanRunOnRemainingMs": 0,
}

DEMO_NETWORKS = [
    {"ssid": "Kitchen WiFi", "rssi": -48, "channel": 6, "encrypted": True},
    {"ssid": "Workshop", "rssi": -66, "channel": 11, "encrypted": True},
    {"ssid": "Guest", "rssi": -78, "channel": 1, "encrypted": False},
]


class DashboardState:
    def __init__(self):
        self.started_at = monotonic()
        self.settings = dict(DEFAULT_SETTINGS)
        self.dev_state = dict(DEFAULT_DEV_STATE)

    def status(self):
        uptime_ms = int((monotonic() - self.started_at) * 1000)
        if self.dev_state["enabled"]:
            status = {
                "temperatureC": self.dev_state["temperatureC"],
                "hasTemperature": self.dev_state["hasTemperature"],
                "sensorDisconnected": self.dev_state["sensorDisconnected"],
                "updateCount": self.dev_state["updateCount"],
                "peltierRunning": self.dev_state["peltierRunning"],
                "fanRunning": self.dev_state["fanRunning"],
                "fanRunOnActive": self.dev_state["fanRunOnActive"],
                "fanRunOnRemainingMs": self.dev_state["fanRunOnRemainingMs"],
                "devMode": True,
            }
        else:
            cycle = uptime_ms // 7000
            peltier = cycle % 2 == 0
            run_on = not peltier and (uptime_ms // 3000) % 4 == 0
            status = {
                "temperatureC": 5.4,
                "hasTemperature": True,
                "sensorDisconnected": False,
                "updateCount": uptime_ms // 1000,
                "peltierRunning": peltier,
                "fanRunning": peltier or run_on,
                "fanRunOnActive": run_on,
                "fanRunOnRemainingMs": max(0, 12000 - (uptime_ms % 12000)),
                "devMode": False,
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

    def save_dev_state(self, form):
        self.dev_state = {
            "ok": True,
            "enabled": read_bool(form, "enabled", False),
            "temperatureC": read_float(form, "temperatureC", 0.0),
            "hasTemperature": read_bool(form, "hasTemperature", False),
            "sensorDisconnected": read_bool(form, "sensorDisconnected", False),
            "updateCount": max(0, read_int(form, "updateCount", 0)),
            "peltierRunning": read_bool(form, "peltierRunning", False),
            "fanRunning": read_bool(form, "fanRunning", False),
            "fanRunOnActive": read_bool(form, "fanRunOnActive", False),
            "fanRunOnRemainingMs": max(0, read_int(form, "fanRunOnRemainingMs", 0)),
        }
        return dict(self.dev_state)

    def networks(self):
        return {
            "ok": True,
            "scanResult": len(DEMO_NETWORKS),
            "scanStatus": "complete",
            "scanMode": "active",
            "networks": DEMO_NETWORKS,
            "count": len(DEMO_NETWORKS),
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
        if self.path == "/api/networks":
            self.send_json(self.state.networks())
            return
        if self.path == "/api/dev":
            self.send_json(self.state.dev_state)
            return
        return super().do_GET()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8")
        form = parse_qs(body, keep_blank_values=True)
        if self.path == "/api/settings":
            self.send_json(self.state.save_settings(form))
            return
        if self.path == "/api/dev":
            self.send_json(self.state.save_dev_state(form))
            return
        self.send_error(404, "Not found")

    def send_json(self, payload):
        data = json.dumps(payload).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)


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
    print("API: /api/status, /api/settings, /api/networks, /api/dev")
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
