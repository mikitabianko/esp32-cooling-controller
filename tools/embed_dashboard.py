from html.parser import HTMLParser
from pathlib import Path
import json
import re

Import("env")

PROJECT_DIR = Path(env.subst("$PROJECT_DIR")).resolve()
root_option = env.GetProjectOption("custom_web_root", "")
ROOT_DIR = Path(root_option) if root_option else PROJECT_DIR
if not ROOT_DIR.is_absolute():
    ROOT_DIR = (PROJECT_DIR / ROOT_DIR).resolve()
WEB_DIR = ROOT_DIR / "web"
APPLICATION_ID = env.GetProjectOption("custom_web_app", "cooling")
target_option = env.GetProjectOption("custom_web_header", "")
TARGET = Path(target_option) if target_option else (
    ROOT_DIR / "lib" / "cooling_feature" / "src" / "web" / "WebDashboardPage.h"
)
if not TARGET.is_absolute():
    TARGET = ROOT_DIR / TARGET


class AssetCollector(HTMLParser):
    def __init__(self):
        super().__init__()
        self.assets = []

    def handle_starttag(self, tag, attrs):
        attr_map = dict(attrs)
        if tag == "link" and "stylesheet" in attr_map.get("rel", "").split():
            self.assets.append(attr_map.get("href", ""))
        if tag == "script" and "src" in attr_map:
            self.assets.append(attr_map.get("src", ""))


def application_manifests(application_id):
    manifests = []
    for path in sorted(WEB_DIR.glob("apps/*/manifest.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        missing = [field for field in ("id", "title", "entry") if not data.get(field)]
        if missing:
            raise ValueError(f"{path}: missing fields: {', '.join(missing)}")
        if data["id"] != application_id:
            continue
        entry = data["entry"]
        if not entry.startswith("/") or entry.endswith(".html"):
            raise ValueError(f"{path}: entry must be an absolute extensionless route")
        source = local_web_path(entry + ".html")
        if source is None:
            raise ValueError(f"{path}: entry page does not exist: {entry}.html")
        manifests.append({**data, "manifest_path": path, "source": source})
    if not manifests:
        raise ValueError(f"No web application manifest found for: {application_id}")
    return manifests


def c_identifier(text):
    return re.sub(r"[^A-Za-z0-9_]+", "_", text).strip("_")


def symbol_name(application, is_default):
    if is_default:
        return "kIndexHtml"
    name = c_identifier(application["id"])
    return "k" + name[:1].upper() + name[1:] + "Html"


def delimiter_name(application):
    return c_identifier(application["id"])[:11] + "_html"


def routes_for(application, is_default):
    routes = [application["entry"], application["entry"] + ".html"]
    if is_default:
        routes = ["/", "/dashboard.html", *routes]
    return routes


def local_web_path(url):
    if not url.startswith("/"):
        return None
    clean_url = url.split("?", 1)[0].split("#", 1)[0]
    path = (WEB_DIR / clean_url.lstrip("/")).resolve()
    try:
        path.relative_to(WEB_DIR)
    except ValueError:
        return None
    return path if path.is_file() else None


def inline_assets(html, source_path):
    collector = AssetCollector()
    collector.feed(html)
    for url in collector.assets:
        asset_path = local_web_path(url)
        if asset_path is None:
            raise ValueError(f"{source_path}: local asset does not exist: {url}")
        content = asset_path.read_text(encoding="utf-8")
        if asset_path.suffix == ".css":
            html = html.replace(
                f'<link rel="stylesheet" href="{url}">',
                f"<style>\n{content}\n  </style>",
            )
        elif asset_path.suffix == ".js":
            html = html.replace(
                f'<script src="{url}"></script>',
                f"<script>\n{content}\n  </script>",
            )
        else:
            raise ValueError(f"{source_path}: unsupported page asset: {url}")
    return html


def build_page(application, is_default):
    source_path = application["source"]
    delimiter = delimiter_name(application)
    html = inline_assets(source_path.read_text(encoding="utf-8"), source_path)
    if f"){delimiter}\"" in html:
        raise ValueError(f"{source_path.name} contains the raw string delimiter")

    manifest_path = application["manifest_path"].relative_to(ROOT_DIR)
    source_name = source_path.relative_to(ROOT_DIR)
    return (
        f"// Generated from {manifest_path} and {source_name}.\n"
        f"const char {symbol_name(application, is_default)}[] PROGMEM = R\"{delimiter}(\n"
        f"{html}\n"
        f"){delimiter}\";\n"
    )


def build_header(target_path):
    applications = application_manifests(APPLICATION_ID)
    page_sources = "\n".join(
        build_page(application, index == 0)
        for index, application in enumerate(applications)
    )
    route_checks = []
    for index, application in enumerate(applications):
        route_match = " || ".join(
            f'path == "{route}"'
            for route in routes_for(application, index == 0)
        )
        route_checks.append(
            f"  if ({route_match}) {{ return {symbol_name(application, index == 0)}; }}"
        )

    header = (
        "#pragma once\n\n"
        "#include <Arduino.h>\n\n"
        f"{page_sources}"
        "inline const char *webPageForPath(const String &path)\n"
        "{\n"
        f"{chr(10).join(route_checks)}\n"
        "  return nullptr;\n"
        "}\n"
    )
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.write_text(header, encoding="utf-8")


build_header(TARGET)
