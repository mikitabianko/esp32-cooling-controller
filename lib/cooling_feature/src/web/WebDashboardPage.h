#pragma once

#include <Arduino.h>

// Generated from web/apps/cooling/manifest.json and web/apps/cooling/dashboard.html.
const char kIndexHtml[] PROGMEM = R"cooling_html(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Cooling Controller</title>
  <style>
:root {
  color-scheme: light;
  font-family: system-ui, sans-serif;
  --bg: #f5f7f8;
  --text: #172126;
  --muted: #68787f;
  --card: #ffffff;
  --border: #dce4e7;
  --ok: #11773d;
  --bad: #bf3030;
  --button: #e7eef1;
  --button-text: #172126;
  --primary: #176b4d;
  --chart-grid: #d8e1e4;
  --chart-primary: #0f7a5a;
  --chart-secondary: #b15f1b;
  --chart-target: #7b4db9;
  --chart-hysteresis: rgba(123, 77, 185, 0.17);
  --chart-current: rgba(15, 122, 90, 0.54);
  --chart-prediction: #e044d0;
  --chart-disconnected: rgba(191, 48, 48, 0.16);
  --chart-selection: rgba(23, 107, 77, 0.16);
  --input: #ffffff;
  --overlay: rgba(12, 18, 20, 0.42);
  --shadow: 0 18px 60px rgba(21, 32, 36, 0.18);
}

[data-theme="dark"] {
  color-scheme: dark;
  --bg: #101417;
  --text: #eef4f2;
  --muted: #95a3a0;
  --card: #182024;
  --border: #2b3638;
  --ok: #65d38a;
  --bad: #ff7474;
  --button: #283236;
  --button-text: #eef4f2;
  --chart-grid: #2e3b3e;
  --chart-primary: #5ccca1;
  --chart-secondary: #f0a45f;
  --chart-target: #c4a2ff;
  --chart-hysteresis: rgba(196, 162, 255, 0.16);
  --chart-current: rgba(92, 204, 161, 0.58);
  --chart-prediction: #ff8df2;
  --chart-disconnected: rgba(255, 116, 116, 0.18);
  --chart-selection: rgba(92, 204, 161, 0.18);
  --input: #11181b;
  --overlay: rgba(0, 0, 0, 0.58);
  --shadow: 0 18px 60px rgba(0, 0, 0, 0.36);
}

body { margin: 0; background: var(--bg); color: var(--text); }
main { max-width: 860px; margin: 0 auto; padding: 22px; }
.narrow { max-width: 760px; }
header { display: flex; align-items: center; justify-content: space-between; gap: 12px; margin-bottom: 18px; }
h1 { margin: 0; font-size: 26px; font-weight: 700; }
h2 { margin: 0; font-size: 18px; }
button,
a.button,
.toolbar a {
  border: 0;
  border-radius: 8px;
  padding: 9px 12px;
  background: var(--button);
  color: var(--button-text);
  font: inherit;
  font-size: 14px;
  text-decoration: none;
}
button.primary { background: var(--primary); color: #ffffff; }
button:disabled { cursor: not-allowed; opacity: 0.48; }
.subtle { opacity: 0.72; }
.danger { color: var(--bad); }
.actions,
.toolbar { display: flex; align-items: center; gap: 8px; flex-wrap: wrap; justify-content: flex-end; }
.hidden { display: none; }
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(170px, 1fr)); gap: 10px; }
.grid.wide { grid-template-columns: repeat(auto-fit, minmax(190px, 1fr)); }
.card { border: 1px solid var(--border); border-radius: 8px; padding: 14px; background: var(--card); }
.label { color: var(--muted); font-size: 13px; }
.value { margin-top: 8px; font-size: 28px; font-weight: 700; }
.small { font-size: 18px; }
.ok { color: var(--ok); }
.bad { color: var(--bad); }
.chart-panel { margin-top: 14px; border: 1px solid var(--border); border-radius: 8px; padding: 14px; background: var(--card); }
.chart-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 12px; margin-bottom: 10px; }
.chart-head > div:first-child { min-width: 0; }
.chart-head h2 { margin-bottom: 4px; }
#chartWindow { display: block; min-height: 16px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; line-height: 16px; font-variant-numeric: tabular-nums; }
.chart-actions { display: flex; align-items: center; justify-content: flex-end; gap: 8px; flex-wrap: wrap; }
.chart-actions button { padding: 7px 9px; font-size: 12px; }
.chart-actions button[aria-pressed="true"] { background: var(--primary); color: #ffffff; }
.chart-legend { display: flex; align-items: center; justify-content: flex-end; gap: 12px; flex-wrap: wrap; min-height: 22px; }
.legend-item { display: inline-flex; align-items: center; gap: 6px; color: var(--muted); font-size: 12px; font-weight: 650; }
.legend-swatch { width: 22px; height: 3px; border-radius: 999px; background: currentColor; }
.chart-wrap { position: relative; width: 100%; height: 260px; }
.chart-wrap canvas { display: block; width: 100%; height: 100%; touch-action: none; }
.chart-tooltip { position: absolute; z-index: 2; display: grid; gap: 2px; min-width: 138px; border: 1px solid var(--border); border-radius: 8px; padding: 8px 9px; background: var(--card); box-shadow: var(--shadow); color: var(--text); font-size: 12px; pointer-events: none; }
.chart-tooltip.hidden { display: none; }
.chart-tooltip span { color: var(--muted); }
.chart-empty { position: absolute; inset: 0; display: grid; place-items: center; color: var(--muted); font-size: 14px; pointer-events: none; }
.chart-empty.hidden { display: none; }
.chart-touch-hint { display: none; margin-top: 8px; color: var(--muted); font-size: 12px; line-height: 1.35; }
.panel { border-top: 1px solid var(--border); padding-top: 14px; margin-top: 14px; }
form { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 10px; align-items: end; }
input,
select { box-sizing: border-box; width: 100%; margin-top: 6px; border: 1px solid var(--border); border-radius: 8px; padding: 9px 10px; background: var(--input); color: var(--text); font: inherit; }
.check { display: flex; gap: 8px; align-items: center; min-height: 38px; }
.check input { width: auto; margin: 0; }
.settings-overlay { position: fixed; inset: 0; display: none; align-items: center; justify-content: center; padding: 18px; background: var(--overlay); z-index: 10; }
.settings-overlay.open { display: flex; }
.settings-dialog { width: min(620px, 100%); max-height: calc(100vh - 36px); overflow: auto; border: 1px solid var(--border); border-radius: 8px; background: var(--card); box-shadow: var(--shadow); }
.settings-head { display: flex; align-items: center; justify-content: space-between; gap: 12px; padding: 14px 16px; border-bottom: 1px solid var(--border); }
#settingsForm { padding: 16px; gap: 16px; }
.settings-section { grid-column: 1 / -1; display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 10px; padding-top: 12px; border-top: 1px solid var(--border); }
.settings-section:first-child { padding-top: 0; border-top: 0; }
.section-title { grid-column: 1 / -1; margin: 0; color: var(--muted); font-size: 13px; font-weight: 700; text-transform: uppercase; }
.android-network { grid-column: 1 / -1; display: grid; gap: 10px; }
.internet-status { display: grid; grid-template-columns: 32px minmax(0, 1fr); gap: 14px; align-items: center; min-height: 56px; border-bottom: 1px solid var(--border); padding: 4px 2px 12px; }
.internet-main { min-width: 0; display: grid; gap: 2px; }
.internet-main strong { overflow-wrap: anywhere; font-size: 16px; font-weight: 650; }
.internet-main span { overflow-wrap: anywhere; color: var(--muted); font-size: 13px; }
.network-status { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 8px; }
.status-line { min-width: 0; padding: 8px 0; border-bottom: 1px solid var(--border); }
.status-line strong { display: block; color: var(--muted); font-size: 12px; font-weight: 650; }
.status-line span { display: block; margin-top: 3px; overflow-wrap: anywhere; font-size: 13px; }
.scan-panel { grid-column: 1 / -1; display: grid; gap: 6px; padding-top: 8px; }
.scan-head { display: flex; align-items: center; justify-content: space-between; gap: 10px; min-height: 42px; }
.scan-head span { display: grid; gap: 2px; }
.scan-head strong { font-size: 15px; font-weight: 650; }
.scan-head small { color: var(--muted); font-size: 12px; }
.network-list { display: grid; }
.network-item { display: grid; grid-template-columns: 32px minmax(0, 1fr) auto; gap: 14px; align-items: center; width: 100%; min-height: 58px; border: 0; border-bottom: 1px solid var(--border); border-radius: 0; padding: 8px 2px; background: transparent; color: var(--text); text-align: left; }
.network-item.selected { color: var(--primary); }
.network-main strong,
.network-main span { overflow-wrap: anywhere; }
.network-main { min-width: 0; display: grid; gap: 2px; }
.network-main strong { font-size: 15px; font-weight: 600; }
.network-main span { color: var(--muted); font-size: 12px; }
.network-trailing { min-width: 42px; color: var(--muted); font-size: 12px; text-align: right; }
.network-item.selected .network-trailing { color: var(--primary); font-weight: 650; }
.network-icon { position: relative; display: block; width: 26px; height: 24px; color: var(--muted); }
.network-icon i { position: absolute; bottom: 2px; width: 5px; border-radius: 999px; background: currentColor; opacity: 0.35; }
.network-icon i:nth-child(1) { left: 3px; height: 7px; }
.network-icon i:nth-child(2) { left: 10px; height: 13px; }
.network-icon i:nth-child(3) { left: 17px; height: 19px; }
.network-icon.signal-1 i:nth-child(1),
.network-icon.signal-2 i:nth-child(-n + 2),
.network-icon.signal-3 i { opacity: 1; }
.network-item.selected .network-icon { color: var(--primary); }
.lock-dot { display: inline-block; width: 7px; height: 7px; margin-left: 4px; border-radius: 999px; background: currentColor; opacity: 0.72; vertical-align: 1px; }
.network-details { grid-column: 1 / -1; display: grid; grid-template-columns: minmax(0, 1fr); gap: 12px; padding-top: 12px; }
.network-field { display: grid; gap: 6px; min-width: 0; padding: 0; color: var(--muted); font-size: 12px; font-weight: 650; }
.network-field input { margin: 0; border: 0; border-bottom: 1px solid var(--border); border-radius: 0; padding: 3px 0 8px; background: transparent; color: var(--text); font-size: 15px; font-weight: 500; outline: 0; }
.network-field input:focus { border-color: var(--primary); }
.network-field input::placeholder { color: var(--muted); opacity: 0.86; }
.network-field > span:last-child { color: var(--muted); font-size: 11px; font-weight: 400; }
.password-control { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 10px; align-items: end; }
.password-control button { min-width: 44px; padding: 4px 0 8px; border-radius: 0; color: var(--primary); background: transparent; font-size: 12px; font-weight: 650; }
.forget-network { justify-self: start; padding: 0; background: transparent; color: var(--bad); font-size: 13px; font-weight: 650; }
.firmware-update { grid-column: 1 / -1; display: grid; gap: 12px; }
.firmware-actions { display: flex; align-items: center; gap: 10px; flex-wrap: wrap; }
#firmwareUploadState { color: var(--muted); font-size: 13px; overflow-wrap: anywhere; }
.switch-row { display: flex; align-items: center; justify-content: space-between; gap: 14px; min-height: 48px; color: var(--text); }
.switch-row strong { display: block; font-size: 14px; font-weight: 600; }
.switch-row small { display: block; margin-top: 2px; color: var(--muted); font-size: 12px; }
.switch { position: relative; width: 48px; height: 28px; flex: 0 0 auto; }
.switch input { position: absolute; inset: 0; width: 100%; height: 100%; margin: 0; opacity: 0; }
.switch span { position: absolute; inset: 0; border-radius: 999px; background: var(--border); }
.switch span::after { content: ""; position: absolute; top: 4px; left: 4px; width: 20px; height: 20px; border-radius: 50%; background: var(--card); box-shadow: 0 1px 4px rgba(0, 0, 0, 0.22); transition: transform 140ms ease; }
.switch input:checked + span { background: var(--primary); }
.switch input:checked + span::after { transform: translateX(20px); }
.form-actions { display: flex; gap: 8px; align-items: center; justify-content: flex-end; grid-column: 1 / -1; margin-top: 4px; }
#saveState { color: var(--muted); font-size: 13px; }

@media (max-width: 560px) {
  main { padding: 14px; }
  header { align-items: flex-start; flex-direction: column; }
  .actions,
  .toolbar { justify-content: flex-start; }
  .chart-panel { margin-right: -8px; margin-left: -8px; padding: 12px; }
  .chart-head { flex-direction: column; }
  .chart-actions { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); width: 100%; gap: 6px; }
  .chart-actions button { min-height: 42px; padding: 8px 6px; font-size: 11px; line-height: 1.15; }
  .chart-legend { grid-column: 1 / -1; justify-content: flex-start; gap: 8px 10px; }
  .legend-item { gap: 5px; font-size: 11px; }
  .legend-swatch { width: 18px; }
  .chart-wrap { height: min(62vh, 360px); min-height: 310px; }
  .chart-tooltip { max-width: calc(100% - 24px); min-width: 132px; }
  .chart-touch-hint { display: block; }
  .settings-overlay { align-items: flex-end; padding: 0; }
  .settings-dialog { width: 100%; max-height: 88vh; border-right: 0; border-bottom: 0; border-left: 0; }
}

  </style>
  <style>
:root {
  --chart-peltier: #b15f1b;
  --chart-fan: #2d8ab8;
}

[data-theme="dark"] {
  --chart-peltier: #f0a45f;
  --chart-fan: #7fc8ef;
}

  </style>
</head>
<body>
  <main class="narrow">
    <header>
      <h1>Cooling Controller</h1>
      <div class="actions">
        <button id="settingsOpen" type="button">Settings</button>
        <button id="themeToggle" type="button">Dark theme</button>
      </div>
    </header>
    <section class="grid">
      <div class="card"><div class="label">Temperature</div><div id="temperature" class="value">--</div></div>
      <div class="card"><div class="label">Sensor</div><div id="sensor" class="value small">--</div></div>
      <div class="card"><div class="label">Peltier</div><div id="peltier" class="value small">--</div></div>
      <div class="card"><div class="label">Fan</div><div id="fan" class="value small">--</div></div>
      <div class="card"><div class="label">Fan run-on</div><div id="runon" class="value small">--</div></div>
      <div class="card"><div class="label">Updates</div><div id="count" class="value small">--</div></div>
      <div class="card"><div class="label">Target</div><div id="target" class="value small">--</div></div>
      <div class="card"><div class="label">Uptime</div><div id="uptime" class="value small">--</div></div>
      <div class="card"><div class="label">WiFi network</div><div id="wifi" class="value small">--</div></div>
      <div class="card"><div class="label">OTA</div><div id="ota" class="value small">--</div></div>
    </section>
    <section class="chart-panel" aria-labelledby="temperatureChartTitle">
      <div class="chart-head">
        <div>
          <h2 id="temperatureChartTitle">Temperature history</h2>
          <div id="chartWindow" class="label">Startup to now</div>
        </div>
        <div class="chart-actions">
          <div id="chartLegend" class="chart-legend"></div>
          <button id="chartPredictionToggle" type="button" aria-pressed="false">Prediction OFF</button>
          <button id="chartResetZoom" type="button">Reset zoom</button>
          <button id="chartDownloadPng" type="button">Download PNG</button>
          <button id="chartDownloadCsv" type="button">Download CSV</button>
        </div>
      </div>
      <div class="chart-wrap">
        <canvas id="temperatureChart" height="260" aria-label="Temperature over time"></canvas>
        <div id="chartTooltip" class="chart-tooltip hidden"></div>
        <div id="chartEmpty" class="chart-empty">Waiting for measurements</div>
      </div>
      <div class="chart-touch-hint">Tap for values. Pinch to zoom. Swipe while zoomed to move.</div>
    </section>
  </main>
  <div id="settingsOverlay" class="settings-overlay" role="dialog" aria-modal="true" aria-labelledby="settingsTitle">
    <section class="settings-dialog">
      <div class="settings-head">
        <h2 id="settingsTitle">Control settings</h2>
        <button id="settingsClose" type="button">Close</button>
      </div>
      <form id="settingsForm">
        <section class="settings-section">
          <h3 class="section-title">Cooling</h3>
          <label class="label">Target, °C<input id="targetInput" name="targetC" type="number" min="-20" max="40" step="0.1" required></label>
          <label class="label">Hysteresis, °C<input id="hysteresisInput" name="hysteresisC" type="number" min="0.1" max="10" step="0.1" required></label>
          <label class="label">Measurement, ms<input id="measurementInput" name="measurementIntervalMs" type="number" min="250" max="60000" step="50" required></label>
          <label class="label">Fan run-on, ms<input id="fanRunOnInput" name="fanRunOnMs" type="number" min="0" max="600000" step="1000" required></label>
        </section>
        <section class="settings-section">
          <h3 class="section-title">Network</h3>
          <div class="android-network">
            <div class="internet-status">
              <span class="network-icon signal-3" aria-hidden="true"><i></i><i></i><i></i></span>
              <span class="internet-main">
                <strong id="stationStatus">--</strong>
                <span id="stationPageStatus">--</span>
              </span>
            </div>
            <div class="network-status">
              <div class="status-line"><strong>Device hotspot</strong><span id="apStatus">--</span></div>
              <div class="status-line"><strong>Hotspot page</strong><span id="apPageStatus">--</span></div>
              <div class="status-line"><strong>Current page</strong><span id="currentPageStatus">--</span></div>
              <div class="status-line"><strong>OTA service</strong><span id="otaStatus">--</span></div>
              <div class="status-line"><strong>OTA hostname</strong><span id="otaHostname">--</span></div>
            </div>
          </div>
          <div class="scan-panel">
            <div class="scan-head">
              <span>
                <strong>Available networks</strong>
                <small id="networkScanState">Not scanned yet</small>
              </span>
              <button id="scanNetworks" type="button">Refresh</button>
            </div>
            <div id="networkList" class="network-list" aria-live="polite"></div>
          </div>
          <div class="network-details">
            <label class="network-field">Network name<input id="stationSsidInput" name="stationSsid" type="text" maxlength="32" autocomplete="off" placeholder="Select a network" readonly><span>Select from the list or enable hidden network</span></label>
            <label class="network-field password-field">Password<span class="password-control"><input id="stationPasswordInput" name="stationPassword" type="password" maxlength="64" autocomplete="new-password" placeholder="Leave blank to keep saved"><button id="toggleStationPassword" type="button" aria-pressed="false">Show</button></span><span>Leave blank to keep the saved password</span></label>
            <label class="switch-row">
              <span><strong>Hidden network</strong><small>Manual SSID</small></span>
              <span class="switch"><input id="hiddenNetworkInput" name="hiddenNetwork" type="checkbox" value="1"><span></span></span>
            </label>
            <input id="forgetStationNetworkInput" name="forgetStationNetwork" type="hidden" value="0">
            <button id="forgetStationNetwork" class="forget-network" type="button">Forget network</button>
          </div>
        </section>
        <section class="settings-section">
          <h3 class="section-title">Firmware</h3>
          <div class="firmware-update">
            <label class="network-field">Firmware image<input id="firmwareFileInput" type="file" accept=".bin,application/octet-stream"><span>Use the generated firmware.bin file</span></label>
            <div class="firmware-actions">
              <button id="firmwareUpload" type="button">Upload firmware</button>
              <span id="firmwareUploadState"></span>
            </div>
          </div>
        </section>
        <div class="form-actions">
          <span id="saveState"></span>
          <button class="primary" type="submit">Save</button>
        </div>
      </form>
    </section>
  </div>
  <script>
(function () {
  const setText = (id, text) => {
    const element = document.getElementById(id);
    if (element) element.textContent = text;
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    setText
  };
}());

  </script>
  <script>
(function () {
  const fetchWithTimeout = (url, options, timeoutMs) => {
    const controller = window.AbortController ? new AbortController() : null;
    let timeoutId = null;
    const timeout = new Promise((_, reject) => {
      timeoutId = window.setTimeout(() => {
        if (controller) controller.abort();
        reject(new Error('request timeout'));
      }, timeoutMs);
    });
    const requestOptions = controller
      ? { ...(options || {}), signal: controller.signal }
      : (options || {});
    return Promise.race([window.fetch(url, requestOptions), timeout])
      .finally(() => {
        if (timeoutId !== null) window.clearTimeout(timeoutId);
      });
  };

  const requestJson = async (url, options, timeoutMs) => {
    const response = await fetchWithTimeout(url, options, timeoutMs);
    if (!response.ok) throw new Error('request failed: ' + response.status);
    return response.json();
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    api: { fetchWithTimeout, requestJson }
  };
}());

  </script>
  <script>
(function () {
  const storageKeys = {
    theme: 'device-dashboard.theme',
    legacyTheme: 'theme'
  };

  const initTheme = () => {
    const themeToggle = document.getElementById('themeToggle');
    if (!themeToggle) return;
    const applyTheme = (theme) => {
      document.documentElement.dataset.theme = theme;
      themeToggle.textContent = theme === 'dark' ? 'Light theme' : 'Dark theme';
      window.localStorage.setItem(storageKeys.theme, theme);
      window.localStorage.setItem(storageKeys.legacyTheme, theme);
    };
    applyTheme(
      window.localStorage.getItem(storageKeys.theme) ||
      window.localStorage.getItem(storageKeys.legacyTheme) ||
      'light'
    );
    themeToggle.addEventListener('click', () => {
      applyTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark');
    });
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    theme: { init: initTheme, storageKeys }
  };
  document.addEventListener('DOMContentLoaded', initTheme);
}());

  </script>
  <script>
(function () {
  const init = ({ overlay, openButton, closeButton, initialFocus }) => {
    const open = () => {
      overlay.classList.add('open');
      openButton.setAttribute('aria-expanded', 'true');
      if (initialFocus) initialFocus.focus();
    };
    const close = () => {
      overlay.classList.remove('open');
      openButton.setAttribute('aria-expanded', 'false');
      openButton.focus();
    };

    openButton.setAttribute('aria-expanded', 'false');
    openButton.addEventListener('click', open);
    closeButton.addEventListener('click', close);
    overlay.addEventListener('click', (event) => {
      if (event.target === overlay) close();
    });
    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && overlay.classList.contains('open')) close();
    });
    return { open, close };
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    settingsDialog: { init }
  };
}());

  </script>
  <script>
(function () {
  const pageUrl = (ip) => ip ? 'http://' + ip + '/' : '--';
  const signalQuality = (rssi) => {
    if (rssi >= -55) return 'Excellent';
    if (rssi >= -67) return 'Good';
    if (rssi >= -75) return 'Fair';
    return 'Weak';
  };
  const signalLevel = (rssi) => {
    const value = Number(rssi);
    if (value >= -60) return 3;
    if (value >= -72) return 2;
    return 1;
  };

  const renderStatus = (data) => {
    const { setText } = window.DeviceWeb;
    setText('apStatus', data.accessPointSsid || '--');
    setText('apPageStatus', pageUrl(data.accessPointIp));
    setText('stationStatus', data.stationSsid
      ? data.stationSsid
      : (data.stationLastFailure ? 'Failed' : 'Not connected'));
    setText('stationPageStatus', data.stationConnected
      ? pageUrl(data.stationIp)
      : (data.stationSsid ? (data.stationStatus || 'disconnected') : 'Wi-Fi off'));
    setText('currentPageStatus', window.location.origin || '--');
  };

  const init = ({ onDirty }) => {
    const scanButton = document.getElementById('scanNetworks');
    const list = document.getElementById('networkList');
    const scanState = document.getElementById('networkScanState');
    const ssidInput = document.getElementById('stationSsidInput');
    const passwordInput = document.getElementById('stationPasswordInput');
    const passwordToggle = document.getElementById('toggleStationPassword');
    const hiddenInput = document.getElementById('hiddenNetworkInput');
    const forgetInput = document.getElementById('forgetStationNetworkInput');
    const forgetButton = document.getElementById('forgetStationNetwork');
    let scanStartedAt = 0;
    let savedSsid = '';
    let selectedSsid = '';
    let visibleSsids = new Set();

    const setHiddenMode = (enabled, keepValue) => {
      hiddenInput.checked = enabled;
      ssidInput.readOnly = !enabled;
      ssidInput.placeholder = enabled ? 'Enter hidden network SSID' : 'Select from nearby networks';
      if (enabled) {
        selectedSsid = '';
        if (!keepValue) ssidInput.value = '';
      } else if (selectedSsid) {
        ssidInput.value = selectedSsid;
      }
    };

    const render = (networks) => {
      list.textContent = '';
      visibleSsids = new Set();
      if (!networks.length) {
        scanState.textContent = 'No 2.4 GHz networks found';
        return;
      }
      scanState.textContent = networks.length + ' found';
      networks.forEach((network) => {
        if (network.ssid) visibleSsids.add(network.ssid);
        const item = document.createElement('button');
        item.type = 'button';
        item.className = 'network-item';
        const selected = Boolean(network.ssid) &&
          network.ssid === ssidInput.value && !hiddenInput.checked;
        item.classList.toggle('selected', selected);
        item.addEventListener('click', () => {
          forgetInput.value = '0';
          if (network.ssid) {
            selectedSsid = network.ssid;
            setHiddenMode(false, false);
          } else {
            setHiddenMode(true, false);
            ssidInput.focus();
          }
          render(networks);
          onDirty('');
        });

        const icon = document.createElement('span');
        icon.className = 'network-icon signal-' + signalLevel(network.rssi);
        icon.setAttribute('aria-hidden', 'true');
        icon.append(document.createElement('i'), document.createElement('i'), document.createElement('i'));
        const main = document.createElement('span');
        main.className = 'network-main';
        const ssid = document.createElement('strong');
        ssid.textContent = network.ssid || '(hidden network)';
        const detail = document.createElement('span');
        detail.textContent = [
          signalQuality(Number(network.rssi)), Number(network.rssi) + ' dBm',
          'ch ' + network.channel, network.encrypted ? 'secured' : 'open'
        ].join(' / ');
        main.append(ssid, detail);
        const trailing = document.createElement('span');
        trailing.className = 'network-trailing';
        trailing.textContent = selected ? 'Selected' : (network.encrypted ? 'Secured' : '');
        if (network.encrypted && !selected) {
          const lock = document.createElement('span');
          lock.className = 'lock-dot';
          trailing.append(lock);
        }
        item.append(icon, main, trailing);
        list.append(item);
      });
    };

    const load = async (polling) => {
      if (!polling) scanStartedAt = Date.now();
      scanButton.disabled = true;
      scanState.textContent = polling ? 'Still scanning...' : 'Scanning...';
      try {
        const data = await window.DeviceWeb.api.requestJson(
          '/api/networks', { cache: 'no-store' }, 3000
        );
        if (data.scanStatus === 'scanning') {
          if (Date.now() - scanStartedAt > 15000) {
            list.textContent = '';
            scanState.textContent = 'Scan timeout';
            return;
          }
          window.setTimeout(() => load(true), 600);
          return;
        }
        if (!data.ok) {
          list.textContent = '';
          scanState.textContent = 'Scan ' + (data.scanStatus || 'failed');
          return;
        }
        render(Array.isArray(data.networks) ? data.networks : []);
      } catch (error) {
        list.textContent = '';
        scanState.textContent = 'Scan unavailable';
      } finally {
        if (!['Still scanning...', 'Scanning...'].includes(scanState.textContent)) {
          scanButton.disabled = false;
        }
      }
    };

    const applySettings = (data) => {
      savedSsid = data.stationSsid || '';
      selectedSsid = savedSsid;
      ssidInput.value = savedSsid;
      passwordInput.value = '';
      passwordInput.type = 'password';
      passwordToggle.textContent = 'Show';
      passwordToggle.setAttribute('aria-pressed', 'false');
      passwordInput.placeholder = data.stationPasswordSet
        ? 'Saved; leave blank to keep'
        : 'Leave blank for open network';
      forgetInput.value = '0';
      setHiddenMode(false, true);
    };

    const validate = () => {
      ssidInput.value = ssidInput.value.trim();
      const changed = ssidInput.value !== savedSsid;
      if (!hiddenInput.checked && changed && ssidInput.value && !visibleSsids.has(ssidInput.value)) {
        return 'Scan and select a network first';
      }
      if (hiddenInput.checked && !ssidInput.value) {
        ssidInput.focus();
        return 'Enter hidden network SSID';
      }
      return '';
    };

    scanButton.addEventListener('click', () => load(false));
    passwordToggle.addEventListener('click', () => {
      const showing = passwordInput.type === 'text';
      passwordInput.type = showing ? 'password' : 'text';
      passwordToggle.textContent = showing ? 'Show' : 'Hide';
      passwordToggle.setAttribute('aria-pressed', showing ? 'false' : 'true');
      passwordInput.focus();
    });
    hiddenInput.addEventListener('change', () => {
      forgetInput.value = '0';
      setHiddenMode(hiddenInput.checked, true);
      onDirty('');
      if (hiddenInput.checked) ssidInput.focus();
    });
    forgetButton.addEventListener('click', () => {
      selectedSsid = '';
      ssidInput.value = '';
      passwordInput.value = '';
      forgetInput.value = '1';
      setHiddenMode(false, true);
      onDirty('Network will be forgotten on save');
    });

    return { applySettings, load, renderStatus, validate };
  };

  window.SystemNetwork = { init, renderStatus };
}());

  </script>
  <script>
(function () {
  const statusText = (data) => {
    if (!data.otaEnabled) return 'Disabled';
    if (data.otaUpdating) return 'Updating ' + (Number(data.otaProgress) || 0) + '%';
    if (data.otaStatus === 'failed') {
      return 'Failed' + (data.otaLastError ? ': ' + data.otaLastError : '');
    }
    if (data.otaStatus === 'completed') return 'Completed';
    if (data.otaStarted) return 'Ready';
    return data.stationConnected ? 'Starting' : 'Waiting for Wi-Fi';
  };

  const renderStatus = (data) => {
    const { setText } = window.DeviceWeb;
    setText('otaStatus', statusText(data));
    setText('otaHostname', data.otaHostname
      ? data.otaHostname + (data.otaPasswordSet ? ' / password' : ' / open')
      : '--');
  };

  const initUploader = () => {
    const fileInput = document.getElementById('firmwareFileInput');
    const uploadButton = document.getElementById('firmwareUpload');
    const state = document.getElementById('firmwareUploadState');
    uploadButton.addEventListener('click', () => {
      const file = fileInput.files && fileInput.files[0];
      if (!file) {
        state.textContent = 'Choose firmware.bin first';
        return;
      }
      const body = new FormData();
      body.append('firmware', file, file.name);
      const request = new XMLHttpRequest();
      uploadButton.disabled = true;
      state.textContent = 'Uploading...';
      request.upload.addEventListener('progress', (event) => {
        if (event.lengthComputable) {
          state.textContent = 'Uploading ' + Math.round((event.loaded * 100) / event.total) + '%';
        }
      });
      request.addEventListener('load', () => {
        uploadButton.disabled = false;
        if (request.status >= 200 && request.status < 300) {
          state.textContent = 'Uploaded; rebooting';
          return;
        }
        try {
          state.textContent = JSON.parse(request.responseText).error || 'Upload failed';
        } catch (error) {
          state.textContent = 'Upload failed';
        }
      });
      request.addEventListener('error', () => {
        uploadButton.disabled = false;
        state.textContent = 'Upload failed';
      });
      request.open('POST', '/api/firmware');
      request.send(body);
    });
  };

  window.SystemOta = { initUploader, renderStatus, statusText };
}());

  </script>
  <script>
(function () {
  const maxHorizonMs = 6 * 60 * 60 * 1000;
  const lookbackMs = 20 * 60 * 1000;
  const maxSampleGapMs = 3 * 60 * 1000;
  const minSpanMs = 90 * 1000;
  const minSamples = 4;
  const targetToleranceC = 0.05;
  const maxTargetShiftC = 0.25;
  const minClosingSlopeCPerSecond = 0.000015;

  const weightedLinearFit = (points) => {
    if (!points || points.length < 2) return null;
    let sumW = 0;
    let sumX = 0;
    let sumY = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      sumW += weight;
      sumX += point.x * weight;
      sumY += point.y * weight;
    });
    if (sumW <= 0) return null;
    const meanX = sumX / sumW;
    const meanY = sumY / sumW;
    let sxx = 0;
    let sxy = 0;
    let syy = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      const dx = point.x - meanX;
      const dy = point.y - meanY;
      sxx += weight * dx * dx;
      sxy += weight * dx * dy;
      syy += weight * dy * dy;
    });
    if (sxx <= 0) return null;
    const slope = sxy / sxx;
    const intercept = meanY - slope * meanX;
    let residualSum = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      const residual = point.y - (intercept + slope * point.x);
      residualSum += weight * residual * residual;
    });
    return {
      slope,
      intercept,
      r2: syy > 0 ? Math.max(0, Math.min(1, (sxy * sxy) / (sxx * syy))) : 0,
      rmse: Math.sqrt(residualSum / sumW)
    };
  };

  const collectSamples = (samples, latest) => {
    const result = [];
    const startMs = latest.timeMs - lookbackMs;
    const targetBandC = Math.max(
      maxTargetShiftC,
      Number.isFinite(latest.hysteresis) ? Math.abs(latest.hysteresis) * 2 : 0
    );
    let previousValid = null;
    for (let index = samples.length - 1; index >= 0; index -= 1) {
      const sample = samples[index];
      if (sample.timeMs > latest.timeMs) continue;
      if (sample.timeMs < startMs) break;
      const valid = Number.isFinite(sample.timeMs) &&
        Number.isFinite(sample.temperature) &&
        Number.isFinite(sample.target) && !sample.disconnected;
      if (!valid) {
        if (result.length && sample.disconnected) break;
        continue;
      }
      if (Math.abs(sample.target - latest.target) > targetBandC) {
        if (result.length) break;
        continue;
      }
      if (previousValid && previousValid.timeMs - sample.timeMs > maxSampleGapMs) break;
      result.push(sample);
      previousValid = sample;
    }
    return result.reverse();
  };

  const trendStats = (samples, direction) => {
    let closingWeight = 0;
    let totalWeight = 0;
    let totalClosingC = 0;
    let totalSeconds = 0;
    for (let index = 1; index < samples.length; index += 1) {
      const previous = samples[index - 1];
      const sample = samples[index];
      const dt = (sample.timeMs - previous.timeMs) / 1000;
      if (dt <= 0) continue;
      const previousDistance = direction * (previous.temperature - previous.target);
      const distance = direction * (sample.temperature - sample.target);
      if (!Number.isFinite(previousDistance) || !Number.isFinite(distance)) continue;
      const closingC = previousDistance - distance;
      const weight = Math.min(dt, 120);
      totalWeight += weight;
      totalSeconds += dt;
      totalClosingC += closingC;
      if (closingC >= -targetToleranceC * 0.3) closingWeight += weight;
    }
    return {
      ratio: totalWeight > 0 ? closingWeight / totalWeight : 0,
      netSlope: totalSeconds > 0 ? totalClosingC / totalSeconds : 0
    };
  };

  const build = (samples, latestChartTime, keepaliveMs) => {
    const latest = samples.slice().reverse().find((sample) => (
      Number.isFinite(sample.temperature) && Number.isFinite(sample.target) && !sample.disconnected
    ));
    if (!latest || latestChartTime - latest.timeMs > keepaliveMs * 2) return null;
    const latestErrorC = latest.temperature - latest.target;
    const latestDistanceC = Math.abs(latestErrorC);
    if (!Number.isFinite(latestDistanceC) || latestDistanceC <= targetToleranceC) return null;
    const direction = latestErrorC >= 0 ? 1 : -1;
    const fitSamples = collectSamples(samples, latest)
      .map((sample) => ({
        ...sample,
        errorC: sample.temperature - sample.target,
        distanceC: direction * (sample.temperature - sample.target)
      }))
      .filter((sample) => sample.distanceC > targetToleranceC * 0.5);
    if (fitSamples.length < minSamples) return null;
    const spanMs = fitSamples[fitSamples.length - 1].timeMs - fitSamples[0].timeMs;
    if (spanMs < minSpanMs) return null;
    const stats = trendStats(fitSamples, direction);
    if (stats.ratio < 0.45 && stats.netSlope <= minClosingSlopeCPerSecond) return null;
    const durationMs = Math.max(1, spanMs);
    const weightedPoints = fitSamples.map((sample) => ({
      x: (sample.timeMs - latest.timeMs) / 1000,
      errorC: sample.errorC,
      distanceC: sample.distanceC,
      weight: 0.35 + 0.65 * Math.pow((sample.timeMs - fitSamples[0].timeMs) / durationMs, 2)
    }));
    const linearFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x, y: point.errorC, weight: point.weight
    })));
    const expFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x, y: Math.log(point.distanceC), weight: point.weight
    })));
    const maxEtaSeconds = maxHorizonMs / 1000;
    const linearClosingSlope = linearFit ? -direction * linearFit.slope : 0;
    const linearEtaSeconds = linearClosingSlope > minClosingSlopeCPerSecond
      ? (latestDistanceC - targetToleranceC) / linearClosingSlope : NaN;
    const expK = expFit ? -expFit.slope : 0;
    const expEtaSeconds = expK > 0 ? Math.log(latestDistanceC / targetToleranceC) / expK : NaN;
    const linearValid = Number.isFinite(linearEtaSeconds) && linearEtaSeconds >= 5 &&
      linearEtaSeconds <= maxEtaSeconds && linearClosingSlope > minClosingSlopeCPerSecond;
    const expValid = Number.isFinite(expEtaSeconds) && expEtaSeconds >= 5 &&
      expEtaSeconds <= maxEtaSeconds && expK > 0;
    if (!linearValid && !expValid) return null;
    const linearScore = linearFit
      ? linearFit.r2 - Math.min(1, linearFit.rmse / Math.max(latestDistanceC, 0.1)) * 0.25 + stats.ratio * 0.2
      : -Infinity;
    const expScore = expFit
      ? expFit.r2 - Math.min(1, expFit.rmse) * 0.2 + stats.ratio * 0.2 +
        (latestDistanceC > 0.3 ? 0.05 : 0)
      : -Infinity;
    const useExp = expValid && (!linearValid || expScore >= linearScore - 0.05);
    const etaMs = (useExp ? expEtaSeconds : linearEtaSeconds) * 1000;
    const endMs = latest.timeMs + etaMs;
    const points = [{ timeMs: latest.timeMs, temperature: latest.temperature }];
    const stepMs = Math.max(15000, Math.min(120000, etaMs / 24));
    for (let timeMs = latest.timeMs + stepMs; timeMs < endMs; timeMs += stepMs) {
      const elapsedSeconds = (timeMs - latest.timeMs) / 1000;
      const distanceC = useExp
        ? latestDistanceC * Math.exp(-expK * elapsedSeconds)
        : Math.max(targetToleranceC, latestDistanceC - linearClosingSlope * elapsedSeconds);
      points.push({
        timeMs,
        temperature: latest.target + direction * Math.max(targetToleranceC, distanceC)
      });
    }
    points.push({
      timeMs: endMs,
      temperature: latest.target + direction * targetToleranceC
    });
    return {
      points,
      etaMs: endMs,
      target: latest.target,
      model: useExp ? 'exponential' : 'linear',
      confidence: Math.max(0, Math.min(1, useExp ? expScore : linearScore))
    };
  };

  window.CoolingPrediction = { build };
}());

  </script>
  <script>
(function () {
  const json = (path, options, timeoutMs) => (
    window.DeviceWeb.api.requestJson(path, options, timeoutMs)
  );

  window.CoolingApi = {
    history: () => json('/api/history', { cache: 'no-store' }, 1200),
    saveSettings: (form) => json('/api/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams(new FormData(form))
    }, 3000),
    status: () => json('/api/status', { cache: 'no-store' }, 900)
  };
}());

  </script>
  <script>
(function () {
  const startedAt = Date.now();
  const minChartWindowMs = 5 * 60 * 1000;
  const keepaliveMs = 30 * 1000;
  const disconnectedFlag = 1;
  const predictionStorageKey = 'cooling-chart-prediction';
  const chartSamples = [];
  let lastChartUptimeMs = 0;
  let lastChartSampleKey = '';
  let zoomRange = null;
  let zoomFollowsRight = false;
  let pointerState = null;
  let predictionEnabled = false;
  const loadPredictionPreference = () => {
    try {
      return window.localStorage && window.localStorage.getItem(predictionStorageKey) === '1';
    } catch (error) {
      return false;
    }
  };
  const setState = (id, on) => {
    const el = document.getElementById(id);
    el.textContent = on ? 'ON' : 'OFF';
    el.className = 'value small ' + (on ? 'ok' : '');
  };
  const sampleTimeMs = (data) => {
    const uptimeMs = Number(data.uptimeMs);
    return Number.isFinite(uptimeMs) ? uptimeMs : Date.now() - startedAt;
  };
  const formatTemperature = (value) => (
    Number.isFinite(value) ? value.toFixed(1) + ' °C' : '--'
  );
  const formatTime = (timeMs) => {
    const seconds = Math.max(0, Math.round(timeMs / 1000));
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = seconds % 60;
    return minutes + ':' + String(remainingSeconds).padStart(2, '0');
  };
  const latestChartTime = () => Math.max(lastChartUptimeMs, ...chartSamples.map((sample) => sample.timeMs), 0);
  const isCoarsePointer = () => (
    window.matchMedia && window.matchMedia('(pointer: coarse)').matches
  );
  const pushChartSample = (sample) => {
    const normalizedSample = normalizeChartSample(sample);
    if (!Number.isFinite(normalizedSample.timeMs)) return;
    const existing = chartSamples.findIndex((item) => item.timeMs === normalizedSample.timeMs);
    if (existing >= 0) chartSamples.splice(existing, 1);
    chartSamples.push(normalizedSample);
    chartSamples.sort((left, right) => left.timeMs - right.timeMs);
    lastChartUptimeMs = Math.max(lastChartUptimeMs, normalizedSample.timeMs);
  };
  const numericOrNull = (value) => {
    if (value === null || value === undefined || value === '') return null;
    const number = Number(value);
    return Number.isFinite(number) ? number : null;
  };
  const normalizeChartSample = (sample) => ({
    ...sample,
    timeMs: Number(sample.timeMs),
    temperature: numericOrNull(sample.temperature),
    target: numericOrNull(sample.target),
    hysteresis: numericOrNull(sample.hysteresis),
    peltierRunning: Boolean(sample.peltierRunning),
    fanRunning: Boolean(sample.fanRunning),
    fanRunOnActive: Boolean(sample.fanRunOnActive),
    disconnected: Boolean(sample.disconnected)
  });
  const addChartSample = (data) => {
    const timeMs = sampleTimeMs(data);
    if (timeMs + 1000 < lastChartUptimeMs) {
      chartSamples.splice(0, chartSamples.length);
      lastChartSampleKey = '';
      zoomRange = null;
      zoomFollowsRight = false;
    }
    lastChartUptimeMs = timeMs;
    const disconnected = Boolean(data.sensorDisconnected) || !data.hasTemperature;
    const temperature = disconnected ? null : Number(data.filteredTemperatureC ?? data.temperatureC);
    const target = Number(data.targetC);
    const hysteresis = Number(data.hysteresisC);
    const sampleKey = [
      data.updateCount,
      disconnected ? 'x' : temperature.toFixed(2),
      Number.isFinite(target) ? target.toFixed(2) : '',
      Number.isFinite(hysteresis) ? hysteresis.toFixed(2) : '',
      data.peltierRunning ? 'p1' : 'p0',
      data.fanRunning ? 'f1' : 'f0',
      data.fanRunOnActive ? 'r1' : 'r0'
    ].join(':');
    if (lastChartSampleKey === sampleKey) return;
    lastChartSampleKey = sampleKey;
    pushChartSample(normalizeChartSample({
      timeMs,
      temperature: Number.isFinite(temperature) ? temperature : null,
      target: Number.isFinite(target) ? target : null,
      hysteresis: Number.isFinite(hysteresis) ? hysteresis : null,
      peltierRunning: Boolean(data.peltierRunning),
      fanRunning: Boolean(data.fanRunning),
      fanRunOnActive: Boolean(data.fanRunOnActive),
      disconnected,
      sensorValid: !disconnected,
      status: disconnected ? 'unavailable' : 'ok',
      flags: disconnected ? disconnectedFlag : 0
    }));
    const cutoffMs = latestChartTime() - Math.max(minChartWindowMs, keepaliveMs * 4);
    while (chartSamples.length > 1200 && chartSamples[0].timeMs < cutoffMs) chartSamples.shift();
  };
  const applyHistory = (history) => {
    if (!history || !Array.isArray(history.series)) return;
    const historySamples = [];
    const historyHysteresis = numericOrNull(history.hysteresisC);
    const peltierFlag = Number(history.peltierFlag) || 2;
    const fanFlag = Number(history.fanFlag) || 4;
    const fanRunOnFlag = Number(history.fanRunOnFlag) || 8;
    history.series.forEach((remoteSeries) => {
      if (remoteSeries.id !== 'probe1' || !Array.isArray(remoteSeries.points)) return;
      remoteSeries.points.forEach((point) => {
        if (!Array.isArray(point) || point.length < 4) return;
        const flags = Number(point[3]) || 0;
        const disconnected =
          point.length > 5 ? Number(point[5]) === 1 : (flags & disconnectedFlag) !== 0;
        const sensorValid =
          point.length > 4 ? Number(point[4]) === 1 : !disconnected;
        const sample = normalizeChartSample({
          timeMs: Number(point[0]),
          temperature: disconnected ? null : Number(point[1]) / 10,
          target: Number(point[2]) / 10,
          hysteresis: point.length > 6 ? numericOrNull(Number(point[6]) / 10) : historyHysteresis,
          peltierRunning: point.length > 7 ? Number(point[7]) === 1 : (flags & peltierFlag) !== 0,
          fanRunning: point.length > 8 ? Number(point[8]) === 1 : (flags & fanFlag) !== 0,
          fanRunOnActive: point.length > 9 ? Number(point[9]) === 1 : (flags & fanRunOnFlag) !== 0,
          disconnected,
          sensorValid,
          status: sensorValid ? 'ok' : 'unavailable',
          flags
        });
        if (Number.isFinite(sample.timeMs)) historySamples.push(sample);
      });
    });
    if (!historySamples.length && chartSamples.length) return;
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    historySamples.forEach(pushChartSample);
  };
  const loadHistory = async () => {
    try {
      applyHistory(await window.CoolingApi.history());
      drawTemperatureChart();
    } catch (error) {
      // History is optional; live samples will continue filling the chart.
    }
  };
  const chartColors = () => {
    const style = getComputedStyle(document.documentElement);
    const predictionColor = document.documentElement.dataset.theme === 'dark'
      ? '#ff8df2'
      : '#e044d0';
    return {
      text: style.getPropertyValue('--muted').trim(),
      grid: style.getPropertyValue('--chart-grid').trim(),
      temperature: style.getPropertyValue('--chart-primary').trim(),
      target: style.getPropertyValue('--chart-target').trim(),
      hysteresis: style.getPropertyValue('--chart-hysteresis').trim(),
      current: style.getPropertyValue('--chart-current').trim(),
      prediction: predictionColor,
      peltier: style.getPropertyValue('--chart-peltier').trim(),
      fan: style.getPropertyValue('--chart-fan').trim(),
      disconnected: style.getPropertyValue('--chart-disconnected').trim(),
      selection: style.getPropertyValue('--chart-selection').trim(),
      background: style.getPropertyValue('--card').trim()
    };
  };
  const renderChartLegend = (legend) => {
    legend.textContent = '';
    [
      { label: 'Filtered temperature', color: '--chart-primary' },
      { label: 'Target temperature', color: '--chart-target' },
      { label: 'Hysteresis band', color: '--chart-hysteresis' },
      { label: 'Peltier', color: '--chart-peltier' },
      { label: 'Fan', color: '--chart-fan' },
      ...(predictionEnabled ? [{ label: 'Prediction', value: chartColors().prediction }] : []),
      { label: 'Sensor unavailable', color: '--chart-disconnected' }
    ].forEach((series) => {
      const item = document.createElement('span');
      item.className = 'legend-item';
      item.style.color = series.value ||
        getComputedStyle(document.documentElement).getPropertyValue(series.color).trim();
      const swatch = document.createElement('span');
      swatch.className = 'legend-swatch';
      const label = document.createElement('span');
      label.textContent = series.label;
      item.append(swatch, label);
      legend.append(item);
    });
  };
  const chartRange = (full) => {
    const fullEnd = chartDisplayEnd();
    if (full) return { startMs: 0, endMs: fullEnd, spanMs: fullEnd };
    if (zoomRange) {
      const rawStartMs = Math.min(zoomRange.startMs, zoomRange.endMs);
      const rawEndMs = Math.max(zoomRange.startMs, zoomRange.endMs);
      const spanMs = Math.max(1000, rawEndMs - rawStartMs);
      const liveEnd = zoomConstraintEnd(spanMs);
      const maxStartMs = Math.max(0, liveEnd - spanMs);
      const startMs = zoomFollowsRight ||
        isChartRightEdge(rawEndMs, liveEnd, spanMs) ||
        isChartRightEdge(rawEndMs, fullEnd, spanMs)
        ? maxStartMs
        : clamp(rawStartMs, 0, maxStartMs);
      const endMs = startMs + spanMs;
      zoomRange = { startMs, endMs };
      zoomFollowsRight = isChartRightEdge(endMs, liveEnd, spanMs);
      return { startMs, endMs, spanMs: endMs - startMs };
    }
    return { startMs: 0, endMs: fullEnd, spanMs: fullEnd };
  };
  const panZoomRange = (deltaMs) => {
    if (!zoomRange || !Number.isFinite(deltaMs)) return false;
    const startMs = Math.min(zoomRange.startMs, zoomRange.endMs);
    const endMs = Math.max(zoomRange.startMs, zoomRange.endMs);
    const spanMs = Math.max(1000, endMs - startMs);
    const liveEnd = zoomConstraintEnd(spanMs);
    const maxStartMs = Math.max(0, liveEnd - spanMs);
    const nextStartMs = clamp(startMs + deltaMs, 0, maxStartMs);
    zoomRange = { startMs: nextStartMs, endMs: nextStartMs + spanMs };
    zoomFollowsRight = isChartRightEdge(zoomRange.endMs, liveEnd, spanMs);
    return true;
  };
  const buildTemperaturePrediction = () => (
    predictionEnabled
      ? window.CoolingPrediction.build(chartSamples, latestChartTime(), keepaliveMs)
      : null
  );
  const drawChartInto = (canvas, full) => {
    const rect = full ? { width: 1200, height: 620 } : canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(1, Math.round(rect.width * (full ? 1 : dpr)));
    const height = Math.max(1, Math.round(rect.height * (full ? 1 : dpr)));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }

    const ctx = canvas.getContext('2d');
    const colors = chartColors();
    ctx.clearRect(0, 0, width, height);
    if (full) {
      ctx.fillStyle = colors.background;
      ctx.fillRect(0, 0, width, height);
    }
    if (!chartSamples.length) return null;

    const range = chartRange(full);
    const visibleSamples = chartSamples.filter((sample) => sample.timeMs >= range.startMs && sample.timeMs <= range.endMs);
    const firstAfterStart = chartSamples.findIndex((sample) => sample.timeMs >= range.startMs);
    const beforeRange = firstAfterStart > 0
      ? chartSamples[firstAfterStart - 1]
      : (firstAfterStart < 0 ? chartSamples[chartSamples.length - 1] : null);
    const firstAfterEnd = chartSamples.find((sample) => sample.timeMs > range.endMs);
    const plotSamples = [beforeRange, ...visibleSamples, firstAfterEnd]
      .filter(Boolean)
      .filter((sample, index, samples) => samples.findIndex((item) => item.timeMs === sample.timeMs) === index)
      .sort((left, right) => left.timeMs - right.timeMs);
    const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
    const predictionSamples = prediction && prediction.points
      ? prediction.points.filter((sample) => sample.timeMs >= range.startMs && sample.timeMs <= range.endMs)
      : [];
    const currentSample = chartSamples.slice().reverse().find((sample) => (
      Number.isFinite(sample.temperature) && !sample.disconnected
    ));
    const values = plotSamples
      .flatMap((sample) => [
        sample.temperature,
        sample.target,
        Number.isFinite(sample.target) && Number.isFinite(sample.hysteresis)
          ? sample.target + sample.hysteresis
          : null
      ].filter(Number.isFinite))
      .concat(predictionSamples.map((sample) => sample.temperature).filter(Number.isFinite))
      .concat(currentSample ? [currentSample.temperature] : []);
    if (!values.length && zoomRange && !full) {
      zoomRange = null;
      zoomFollowsRight = false;
      return drawChartInto(canvas, false);
    }
    if (!values.length) return null;
    const minValue = Math.min(...values);
    const maxValue = Math.max(...values);
    const valuePadding = Math.max(0.5, (maxValue - minValue) * 0.18);
    const yMin = Math.floor((minValue - valuePadding) * 2) / 2;
    const yMax = Math.ceil((maxValue + valuePadding) * 2) / 2;
    const span = Math.max(1, yMax - yMin);
    const scale = full ? 1 : dpr;
    const compact = !full && canvas.getBoundingClientRect().width < 440;
    const pad = {
      left: (compact ? 54 : 76) * scale,
      right: (compact ? 8 : 18) * scale,
      top: (full ? 54 : (compact ? 14 : 18)) * scale,
      bottom: (compact ? 42 : 50) * scale
    };
    const plotW = Math.max(1, width - pad.left - pad.right);
    const plotH = Math.max(1, height - pad.top - pad.bottom);
    const stateLaneH = Math.min((compact ? 18 : 22) * scale, Math.max(0, plotH * 0.18));
    const stateInset = 2 * scale;
    const axisY = pad.top + plotH;
    const xFor = (timeMs) => pad.left + ((timeMs - range.startMs) / range.spanMs) * plotW;
    const yFor = (value) => pad.top + (1 - ((value - yMin) / span)) * plotH;

    plotSamples.forEach((sample, index) => {
      if (!sample.disconnected) return;
      const previous = plotSamples[index - 1];
      if (previous && previous.disconnected) return;
      let lastDisconnected = sample;
      let nextValid = null;
      for (let i = index + 1; i < plotSamples.length; i += 1) {
        if (!plotSamples[i].disconnected) {
          nextValid = plotSamples[i];
          break;
        }
        lastDisconnected = plotSamples[i];
      }
      const rawZoneStart = previous
        ? previous.timeMs + (sample.timeMs - previous.timeMs) / 2
        : sample.timeMs;
      const rawZoneEnd = nextValid
        ? lastDisconnected.timeMs + (nextValid.timeMs - lastDisconnected.timeMs) / 2
        : Math.min(range.endMs, lastDisconnected.timeMs + keepaliveMs);
      const zoneStart = clamp(rawZoneStart, range.startMs, range.endMs);
      const zoneEnd = clamp(
        rawZoneEnd,
        range.startMs,
        range.endMs
      );
      if (zoneEnd <= zoneStart) return;
      ctx.fillStyle = colors.disconnected;
      ctx.fillRect(xFor(zoneStart), pad.top, Math.max(1, xFor(zoneEnd) - xFor(zoneStart)), plotH);
    });

    ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
    ctx.lineWidth = 1 * scale;
    ctx.strokeStyle = colors.grid;
    ctx.fillStyle = colors.text;
    ctx.textBaseline = 'middle';

    const yTicks = compact ? 3 : 4;
    for (let i = 0; i <= yTicks; i += 1) {
      const y = pad.top + (plotH / yTicks) * i;
      const value = yMax - (span / yTicks) * i;
      ctx.beginPath();
      ctx.moveTo(pad.left, y);
      ctx.lineTo(width - pad.right, y);
      ctx.stroke();
      ctx.textAlign = 'right';
      ctx.fillText(value.toFixed(1) + ' °C', pad.left - 8 * scale, y);
    }

    ctx.textBaseline = 'top';
    const xTicks = compact ? 2 : 3;
    for (let i = 0; i <= xTicks; i += 1) {
      const x = pad.left + (plotW / xTicks) * i;
      const timeMs = range.startMs + (range.spanMs / xTicks) * i;
      ctx.beginPath();
      ctx.moveTo(x, pad.top);
      ctx.lineTo(x, pad.top + plotH);
      ctx.stroke();
      ctx.textAlign = i === 0 ? 'left' : (i === xTicks ? 'right' : 'center');
      ctx.fillText(formatTime(timeMs), x, axisY + 8 * scale);
    }

    ctx.strokeStyle = colors.text;
    ctx.lineWidth = 1 * scale;
    ctx.beginPath();
    ctx.moveTo(pad.left, pad.top);
    ctx.lineTo(pad.left, pad.top + plotH);
    ctx.moveTo(pad.left, axisY);
    ctx.lineTo(pad.left + plotW, axisY);
    ctx.stroke();

    ctx.fillStyle = colors.text;
    ctx.font = String((compact ? 11 : 12) * scale) + 'px system-ui, sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'bottom';
    ctx.fillText('Uptime', pad.left + plotW / 2, height - 4 * scale);
    if (!compact) {
      ctx.save();
      ctx.translate(13 * scale, pad.top + plotH / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.textBaseline = 'top';
      ctx.fillText('Temperature, °C', 0, 0);
      ctx.restore();
    }

    const drawHysteresisBand = () => {
      const bandSamples = plotSamples.filter((sample) => (
        Number.isFinite(sample.target) &&
        Number.isFinite(sample.hysteresis) &&
        sample.hysteresis > 0
      ));
      if (bandSamples.length < 1) return;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.fillStyle = colors.hysteresis;
      ctx.beginPath();
      bandSamples.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.target + sample.hysteresis);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      bandSamples.slice().reverse().forEach((sample) => {
        ctx.lineTo(xFor(sample.timeMs), yFor(sample.target));
      });
      ctx.closePath();
      ctx.fill();
      ctx.strokeStyle = colors.target;
      ctx.globalAlpha = 0.42;
      ctx.setLineDash([5 * scale, 5 * scale]);
      ctx.beginPath();
      bandSamples.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.target + sample.hysteresis);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.restore();
    };

    const drawLine = (key, color, allowGaps) => {
      ctx.strokeStyle = color;
      ctx.lineWidth = 2 * scale;
      let drawing = false;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.beginPath();
      const lineSamples = plotSamples.filter((sample) => (
        Number.isFinite(sample[key]) && !(allowGaps && sample.disconnected)
      ));
      if (lineSamples.length === 1 && key === 'target') {
        const y = yFor(lineSamples[0][key]);
        ctx.moveTo(pad.left, y);
        ctx.lineTo(pad.left + plotW, y);
      }
      plotSamples.forEach((sample) => {
        const value = sample[key];
        if (!Number.isFinite(value) || (allowGaps && sample.disconnected)) {
          drawing = false;
          return;
        }
        const x = xFor(sample.timeMs);
        const y = yFor(value);
        if (!drawing) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
        drawing = true;
      });
      ctx.stroke();
      ctx.restore();
    };
    const drawStateLane = () => {
      const rowGap = 1 * scale;
      const rowH = Math.max(1.5 * scale, Math.min(2.5 * scale, (stateLaneH - rowGap) / 2));
      const laneTop = pad.top + plotH - rowH * 2 - rowGap - stateInset;
      const rows = [
        { key: 'peltierRunning', color: colors.peltier, y: laneTop },
        { key: 'fanRunning', color: colors.fan, y: laneTop + rowH + rowGap }
      ];
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, laneTop, plotW, rowH * 2 + rowGap);
      ctx.clip();
      ctx.globalAlpha = 0.16;
      ctx.fillStyle = colors.grid;
      rows.forEach((row) => ctx.fillRect(pad.left, row.y, plotW, rowH));
      ctx.globalAlpha = 1;
      rows.forEach((row) => {
        ctx.fillStyle = row.color;
        plotSamples.forEach((sample, index) => {
          if (!sample[row.key]) return;
          const next = plotSamples[index + 1];
          const start = clamp(sample.timeMs, range.startMs, range.endMs);
          const end = next ? clamp(next.timeMs, range.startMs, range.endMs) : start;
          if (end <= start) return;
          ctx.globalAlpha = row.key === 'fanRunning' && sample.fanRunOnActive ? 0.52 : 0.82;
          ctx.fillRect(xFor(start), row.y, Math.max(1, xFor(end) - xFor(start)), rowH);
        });
      });
      ctx.restore();
    };
    const drawCurrentTemperatureMarker = () => {
      const latest = currentSample;
      if (!latest) return;
      const y = yFor(latest.temperature);
      const rawX = xFor(latest.timeMs);
      const latestVisible = latest.timeMs >= range.startMs && latest.timeMs <= range.endMs;
      const x = latestVisible
        ? clamp(rawX, pad.left, pad.left + plotW)
        : pad.left + plotW;
      ctx.save();
      ctx.strokeStyle = colors.current;
      ctx.lineWidth = 1 * scale;
      ctx.setLineDash([3 * scale, 5 * scale]);
      ctx.beginPath();
      ctx.moveTo(pad.left, y);
      ctx.lineTo(x, y);
      ctx.stroke();
      ctx.setLineDash([]);
      const label = latest.temperature.toFixed(1) + ' °C';
      ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
      const labelW = ctx.measureText(label).width + 8 * scale;
      const labelH = 18 * scale;
      const labelX = Math.max(2 * scale, pad.left - labelW - 4 * scale);
      const labelY = clamp(y - labelH / 2, pad.top, pad.top + plotH - labelH);
      ctx.fillStyle = colors.background;
      ctx.fillRect(labelX, labelY, labelW, labelH);
      ctx.strokeStyle = colors.current;
      ctx.strokeRect(labelX, labelY, labelW, labelH);
      ctx.fillStyle = colors.current;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(label, labelX + labelW / 2, labelY + labelH / 2);
      ctx.restore();
    };
    const drawPrediction = () => {
      if (!prediction || !prediction.points || prediction.points.length < 2) return;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.strokeStyle = colors.prediction;
      ctx.lineWidth = 2 * scale;
      ctx.setLineDash([7 * scale, 5 * scale]);
      ctx.beginPath();
      prediction.points.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.temperature);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.setLineDash([]);
      const eta = prediction.points[prediction.points.length - 1];
      if (eta.timeMs >= range.startMs && eta.timeMs <= range.endMs) {
        const x = xFor(eta.timeMs);
        const y = yFor(eta.temperature);
        ctx.fillStyle = colors.prediction;
        ctx.beginPath();
        ctx.arc(x, y, 3.5 * scale, 0, Math.PI * 2);
        ctx.fill();
        ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'bottom';
        ctx.fillText('ETA ' + formatTime(eta.timeMs), Math.min(x + 7 * scale, pad.left + plotW - 62 * scale), y - 5 * scale);
      }
      ctx.restore();
    };
    drawHysteresisBand();
    drawLine('target', colors.target, false);
    drawPrediction();
    drawLine('temperature', colors.temperature, true);
    drawCurrentTemperatureMarker();
    drawStateLane();

    if (full) {
      ctx.textBaseline = 'middle';
      ctx.textAlign = 'left';
      ctx.font = '16px system-ui, sans-serif';
      ctx.fillStyle = colors.text;
      ctx.fillText('Temperature history', pad.left, 22);
      [
        ['Filtered temperature', colors.temperature],
        ['Target temperature', colors.target],
        ['Hysteresis band', colors.target],
        ['Peltier', colors.peltier],
        ['Fan', colors.fan],
        ...(predictionEnabled ? [['Prediction', colors.prediction]] : []),
        ['Sensor unavailable', colors.disconnected]
      ].forEach((item, index) => {
        const x = pad.left + index * 190;
        const y = 42;
        ctx.fillStyle = item[1];
        ctx.fillRect(x, y - 2, 34, 4);
        ctx.fillStyle = colors.text;
        ctx.fillText(item[0], x + 44, y);
      });
    }

    if (!full && pointerState && pointerState.dragging && pointerState.mode === 'zoom') {
      ctx.fillStyle = colors.selection;
      const left = Math.max(pad.left, Math.min(pointerState.startX, pointerState.currentX));
      const right = Math.min(pad.left + plotW, Math.max(pointerState.startX, pointerState.currentX));
      ctx.fillRect(left, pad.top, right - left, plotH);
    }
    return { pad, plotW, plotH, range, xFor, yFor, visibleSamples };
  };
  const drawTemperatureChart = () => {
    const canvas = document.getElementById('temperatureChart');
    const empty = document.getElementById('chartEmpty');
    const legend = document.getElementById('chartLegend');
    const chartWindow = document.getElementById('chartWindow');
    const chartResetZoom = document.getElementById('chartResetZoom');
    if (!canvas || !empty || !legend) return;
    empty.classList.toggle('hidden', chartSamples.length > 0);
    renderChartLegend(legend);
    const geometry = drawChartInto(canvas, false);
    if (chartWindow) {
      const range = chartRange(false);
      const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
      chartWindow.textContent = zoomRange
        ? 'Zoom ' + formatTime(range.startMs) + ' to ' + formatTime(range.endMs)
        : (prediction
            ? 'Startup to ' + formatTime(latestChartTime()) + ' / ETA ' + formatTime(prediction.etaMs)
            : 'Startup to ' + formatTime(latestChartTime()));
    }
    if (chartResetZoom) {
      chartResetZoom.disabled = !zoomRange;
      chartResetZoom.textContent = zoomRange ? 'Show full range' : 'Full range';
      chartResetZoom.setAttribute('aria-disabled', zoomRange ? 'false' : 'true');
      chartResetZoom.title = zoomRange
        ? 'Return to the full temperature history'
        : 'The full temperature history is already visible';
    }
    return geometry;
  };
  const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
  const chartDisplayEnd = () => {
    const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
    return Math.max(minChartWindowMs, latestChartTime(), prediction ? prediction.etaMs : 0);
  };
  const zoomConstraintEnd = (spanMs) => Math.max(Math.max(1000, spanMs), latestChartTime());
  const isChartRightEdge = (endMs, fullEnd, spanMs) => (
    Math.abs(endMs - fullEnd) <= Math.max(1500, spanMs * 0.002)
  );
  const pointerPoint = (canvas, event) => {
    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const cssX = event.clientX - rect.left;
    const cssY = event.clientY - rect.top;
    return { cssX, cssY, x: cssX * dpr, y: cssY * dpr };
  };
  const inPlotArea = (geometry, point) => (
    point.x >= geometry.pad.left &&
    point.x <= geometry.pad.left + geometry.plotW &&
    point.y >= geometry.pad.top &&
    point.y <= geometry.pad.top + geometry.plotH
  );
  const clampToPlotX = (geometry, x) => (
    clamp(x, geometry.pad.left, geometry.pad.left + geometry.plotW)
  );
  const isPlotRightEdge = (geometry, x) => (
    Math.abs(x - (geometry.pad.left + geometry.plotW)) <=
      Math.max(8 * (window.devicePixelRatio || 1), geometry.plotW * 0.01)
  );
  const nearestSample = (canvas, event) => {
    const geometry = drawTemperatureChart();
    if (!geometry) return null;
    const point = pointerPoint(canvas, event);
    if (!inPlotArea(geometry, point)) return null;
    const candidates = geometry.visibleSamples.map((sample) => ({
      sample,
      distance: Math.abs(geometry.xFor(sample.timeMs) - point.x)
    })).sort((left, right) => left.distance - right.distance);
    if (!candidates.length) return null;
    return { sample: candidates[0].sample, ...point };
  };
  const showTooltip = (point) => {
    const tooltip = document.getElementById('chartTooltip');
    if (!tooltip || !point) {
      if (tooltip) tooltip.classList.add('hidden');
      return;
    }
    tooltip.innerHTML = [
      '<strong>' + formatTime(point.sample.timeMs) + '</strong>',
      '<span>Temperature: ' + formatTemperature(point.sample.temperature) + '</span>',
      '<span>Target: ' + formatTemperature(point.sample.target) + '</span>',
      '<span>Hys: ' + formatTemperature(point.sample.hysteresis) + '</span>',
      '<span>Peltier: ' + (point.sample.peltierRunning ? 'ON' : 'OFF') + '</span>',
      '<span>Fan: ' + (point.sample.fanRunning ? (point.sample.fanRunOnActive ? 'RUN-ON' : 'ON') : 'OFF') + '</span>'
    ].join('');
    tooltip.style.left = Math.min(point.cssX + 12, tooltip.parentElement.clientWidth - 150) + 'px';
    tooltip.style.top = Math.max(8, point.cssY - 52) + 'px';
    tooltip.classList.remove('hidden');
  };
  const downloadCsvFromSamples = () => {
    const rows = [['timestamp_ms', 'filtered_temperature_c', 'target_temperature_c', 'hysteresis_c', 'peltier_running', 'fan_running', 'fan_runon_active', 'sensor_status', 'sensor_valid', 'sensor_disconnected', 'flags']];
    chartSamples.forEach((sample) => {
      rows.push([
        Math.round(sample.timeMs),
        Number.isFinite(sample.temperature) ? sample.temperature.toFixed(1) : '',
        Number.isFinite(sample.target) ? sample.target.toFixed(1) : '',
        Number.isFinite(sample.hysteresis) ? sample.hysteresis.toFixed(1) : '',
        sample.peltierRunning ? '1' : '0',
        sample.fanRunning ? '1' : '0',
        sample.fanRunOnActive ? '1' : '0',
        sample.status || (sample.disconnected ? 'unavailable' : 'ok'),
        sample.sensorValid === false || sample.disconnected ? '0' : '1',
        sample.disconnected ? '1' : '0',
        String(sample.flags || 0)
      ]);
    });
    const csv = rows.map((row) => row.map((cell) => '"' + String(cell).replace(/"/g, '""') + '"').join(',')).join('\n');
    const link = document.createElement('a');
    link.href = URL.createObjectURL(new Blob([csv], { type: 'text/csv' }));
    link.download = 'cooling-history.csv';
    link.click();
    URL.revokeObjectURL(link.href);
  };
  const downloadCsv = async () => {
    try {
      const res = await fetch('/api/history.csv', { cache: 'no-store' });
      if (!res.ok) throw new Error('csv unavailable');
      const blob = await res.blob();
      const link = document.createElement('a');
      link.href = URL.createObjectURL(blob);
      link.download = 'cooling-history.csv';
      link.click();
      URL.revokeObjectURL(link.href);
    } catch (error) {
      downloadCsvFromSamples();
    }
  };
  const downloadPng = () => {
    const canvas = document.createElement('canvas');
    drawChartInto(canvas, true);
    const link = document.createElement('a');
    link.href = canvas.toDataURL('image/png');
    link.download = 'cooling-chart.png';
    link.click();
  };
  const resetChartZoom = () => {
    if (!zoomRange) return;
    zoomRange = null;
    zoomFollowsRight = false;
    showTooltip(null);
    drawTemperatureChart();
  };
  const midpoint = (left, right) => ({
    x: (left.x + right.x) / 2,
    y: (left.y + right.y) / 2
  });
  const distance = (left, right) => Math.hypot(left.x - right.x, left.y - right.y);
  const zoomAroundPoint = (geometry, centerX, scaleFactor) => {
    if (!geometry || !Number.isFinite(scaleFactor) || scaleFactor <= 0) return false;
    const fullEnd = chartDisplayEnd();
    const range = chartRange(false);
    const currentSpan = Math.max(1000, range.endMs - range.startMs);
    const minSpan = Math.max(keepaliveMs, fullEnd / 80);
    const nextSpan = clamp(currentSpan / scaleFactor, minSpan, fullEnd);
    const ratio = clamp((centerX - geometry.pad.left) / geometry.plotW, 0, 1);
    const centerTime = range.startMs + ratio * currentSpan;
    const liveEnd = zoomConstraintEnd(nextSpan);
    const maxStartMs = Math.max(0, liveEnd - nextSpan);
    let startMs = clamp(centerTime - ratio * nextSpan, 0, maxStartMs);
    zoomFollowsRight = isChartRightEdge(startMs + nextSpan, liveEnd, nextSpan) ||
      (zoomFollowsRight && ratio > 0.96) ||
      isPlotRightEdge(geometry, centerX);
    if (zoomFollowsRight) startMs = maxStartMs;
    zoomRange = { startMs, endMs: startMs + nextSpan };
    return true;
  };
  const clearChartSamples = () => {
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    showTooltip(null);
    drawTemperatureChart();
  };
  const replaceChartSamples = (samples) => {
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    (samples || []).forEach(pushChartSample);
    drawTemperatureChart();
  };
  const setPredictionEnabled = (enabled) => {
    predictionEnabled = Boolean(enabled);
    try {
      if (window.localStorage) {
        window.localStorage.setItem(predictionStorageKey, predictionEnabled ? '1' : '0');
      }
    } catch (error) {
      // Local storage can be unavailable in private or embedded contexts.
    }
    const toggle = document.getElementById('chartPredictionToggle');
    if (toggle) {
      toggle.textContent = predictionEnabled ? 'Prediction ON' : 'Prediction OFF';
      toggle.setAttribute('aria-pressed', predictionEnabled ? 'true' : 'false');
      toggle.title = predictionEnabled
        ? 'Hide browser-side temperature prediction'
        : 'Show browser-side temperature prediction';
    }
    drawTemperatureChart();
  };
  const attachChartInteractions = (chartCanvas) => {
    if (!chartCanvas || chartCanvas.dataset.chartInteractions === '1') return;
    chartCanvas.dataset.chartInteractions = '1';
    const activePointers = new Map();
    let pinchState = null;
    const stopPointerDrag = () => {
      pointerState = null;
      pinchState = null;
      showTooltip(null);
      drawTemperatureChart();
    };
    chartCanvas.addEventListener('pointerdown', (event) => {
      activePointers.set(event.pointerId, pointerPoint(chartCanvas, event));
      if (activePointers.size === 2) {
        const points = Array.from(activePointers.values());
        const geometry = drawTemperatureChart();
        const center = midpoint(points[0], points[1]);
        pointerState = null;
        pinchState = geometry && inPlotArea(geometry, center)
          ? {
              distance: distance(points[0], points[1])
            }
          : null;
        chartCanvas.setPointerCapture(event.pointerId);
        showTooltip(null);
        return;
      }
      const geometry = drawTemperatureChart();
      const point = pointerPoint(chartCanvas, event);
      if (!geometry || !inPlotArea(geometry, point)) {
        showTooltip(null);
        return;
      }
      const x = clampToPlotX(geometry, point.x);
      const activeRange = chartRange(false);
      pointerState = {
        mode: isCoarsePointer() ? 'inspect' : (zoomRange ? 'pan' : 'zoom'),
        startX: x,
        currentX: x,
        startRange: { startMs: activeRange.startMs, endMs: activeRange.endMs },
        dragging: false
      };
      chartCanvas.setPointerCapture(event.pointerId);
      showTooltip(null);
    });
    chartCanvas.addEventListener('pointermove', (event) => {
      if (activePointers.has(event.pointerId)) {
        activePointers.set(event.pointerId, pointerPoint(chartCanvas, event));
      }
      if (pinchState && activePointers.size >= 2) {
        const points = Array.from(activePointers.values()).slice(0, 2);
        const geometry = drawTemperatureChart();
        const nextDistance = distance(points[0], points[1]);
        const center = midpoint(points[0], points[1]);
        if (geometry && nextDistance > 0) {
          zoomAroundPoint(geometry, clampToPlotX(geometry, center.x), nextDistance / pinchState.distance);
          pinchState = { distance: nextDistance };
        }
        showTooltip(null);
        drawTemperatureChart();
        return;
      }
      if (pointerState) {
        const geometry = drawTemperatureChart();
        const point = pointerPoint(chartCanvas, event);
        const dpr = window.devicePixelRatio || 1;
        pointerState.currentX = geometry ? clampToPlotX(geometry, point.x) : point.x;
        pointerState.dragging = Math.abs(pointerState.currentX - pointerState.startX) > 8 * dpr;
        if (geometry && pointerState.dragging && (pointerState.mode === 'pan' || (pointerState.mode === 'inspect' && zoomRange))) {
          const spanMs = Math.max(1000, pointerState.startRange.endMs - pointerState.startRange.startMs);
          const deltaMs = ((pointerState.startX - pointerState.currentX) / geometry.plotW) * spanMs;
          zoomRange = {
            startMs: pointerState.startRange.startMs,
            endMs: pointerState.startRange.endMs
          };
          panZoomRange(deltaMs);
        }
      }
      if (pointerState && pointerState.dragging) showTooltip(null);
      else showTooltip(nearestSample(chartCanvas, event));
      drawTemperatureChart();
    });
    chartCanvas.addEventListener('wheel', (event) => {
      if (!zoomRange) return;
      const geometry = drawTemperatureChart();
      if (!geometry) return;
      const point = pointerPoint(chartCanvas, event);
      if (!inPlotArea(geometry, point)) return;
      event.preventDefault();
      const wheelDelta = Math.abs(event.deltaX) > Math.abs(event.deltaY)
        ? event.deltaX
        : event.deltaY;
      const deltaMs = (wheelDelta / geometry.plotW) * geometry.range.spanMs;
      if (panZoomRange(deltaMs)) {
        showTooltip(null);
        drawTemperatureChart();
      }
    }, { passive: false });
    chartCanvas.addEventListener('pointerup', (event) => {
      activePointers.delete(event.pointerId);
      if (pinchState) {
        if (chartCanvas.hasPointerCapture && chartCanvas.hasPointerCapture(event.pointerId)) {
          chartCanvas.releasePointerCapture(event.pointerId);
        }
        pinchState = null;
        pointerState = null;
        showTooltip(null);
        drawTemperatureChart();
        return;
      }
      if (pointerState && pointerState.dragging && pointerState.mode === 'zoom') {
        const geometry = drawTemperatureChart();
        if (geometry) {
          pointerState.currentX = clampToPlotX(geometry, pointerPoint(chartCanvas, event).x);
          const left = Math.max(geometry.pad.left, Math.min(pointerState.startX, pointerState.currentX));
          const right = Math.min(geometry.pad.left + geometry.plotW, Math.max(pointerState.startX, pointerState.currentX));
          if (right - left > 12) {
            const startMs = geometry.range.startMs +
              ((left - geometry.pad.left) / geometry.plotW) * geometry.range.spanMs;
            const endMs = geometry.range.startMs +
              ((right - geometry.pad.left) / geometry.plotW) * geometry.range.spanMs;
            zoomRange = { startMs, endMs };
            const spanMs = Math.max(1000, endMs - startMs);
            zoomFollowsRight = isPlotRightEdge(geometry, right) ||
              isChartRightEdge(endMs, zoomConstraintEnd(spanMs), spanMs);
          }
        }
      } else if (pointerState && !pointerState.dragging) {
        showTooltip(nearestSample(chartCanvas, event));
      }
      if (chartCanvas.hasPointerCapture && chartCanvas.hasPointerCapture(event.pointerId)) {
        chartCanvas.releasePointerCapture(event.pointerId);
      }
      pointerState = null;
      drawTemperatureChart();
    });
    chartCanvas.addEventListener('pointercancel', (event) => {
      activePointers.delete(event.pointerId);
      stopPointerDrag();
    });
    chartCanvas.addEventListener('lostpointercapture', () => {
      if (pointerState) stopPointerDrag();
    });
    chartCanvas.addEventListener('pointerleave', () => {
      if (!pointerState) showTooltip(null);
    });
    chartCanvas.addEventListener('contextlost', (event) => {
      event.preventDefault();
      showTooltip(null);
    });
    chartCanvas.addEventListener('contextrestored', () => {
      drawTemperatureChart();
    });
  };
  window.CoolingChart = {
    addStatusSample: addChartSample,
    addSample: pushChartSample,
    applyHistory,
    clear: clearChartSamples,
    replaceSamples: replaceChartSamples,
    draw: drawTemperatureChart,
    resetZoom: resetChartZoom,
    attachInteractions: attachChartInteractions,
    downloadPng,
    downloadCsv,
    loadHistory,
    loadPredictionPreference,
    setPredictionEnabled,
    samples: chartSamples
  };
}());

  </script>
  <script>
(function () {
  const init = ({ form, saveState, network }) => {
    let dirty = false;
    const apply = (data) => {
      if (dirty) return;
      document.getElementById('targetInput').value = Number(data.targetC).toFixed(1);
      document.getElementById('hysteresisInput').value = Number(data.hysteresisC).toFixed(1);
      document.getElementById('measurementInput').value = data.measurementIntervalMs;
      document.getElementById('fanRunOnInput').value = data.fanRunOnMs;
      network.applySettings(data);
    };
    const markDirty = (message) => {
      dirty = true;
      saveState.textContent = message || '';
    };
    const markSaved = (data) => {
      dirty = false;
      apply(data);
      saveState.textContent = 'Saved';
    };
    form.addEventListener('input', () => markDirty(''));
    return { apply, isDirty: () => dirty, markDirty, markSaved };
  };

  window.CoolingSettings = { init };
}());

  </script>
  <script>
(function () {
  const formatTemperature = (value) => (
    Number.isFinite(Number(value)) ? Number(value).toFixed(1) + ' °C' : '--'
  );

  const setState = (id, enabled) => {
    const element = document.getElementById(id);
    element.textContent = enabled ? 'ON' : 'OFF';
    element.className = 'value small ' + (enabled ? 'ok' : '');
  };

  document.addEventListener('DOMContentLoaded', () => {
    const { setText, settingsDialog } = window.DeviceWeb;
    const api = window.CoolingApi;
    const chart = window.CoolingChart;
    const settingsForm = document.getElementById('settingsForm');
    const saveState = document.getElementById('saveState');
    const network = window.SystemNetwork.init({
      onDirty: (message) => settings.markDirty(message)
    });
    const settings = window.CoolingSettings.init({
      form: settingsForm,
      saveState,
      network
    });

    settingsDialog.init({
      overlay: document.getElementById('settingsOverlay'),
      openButton: document.getElementById('settingsOpen'),
      closeButton: document.getElementById('settingsClose'),
      initialFocus: document.getElementById('targetInput')
    });
    window.SystemOta.initUploader();

    const chartCanvas = document.getElementById('temperatureChart');
    const predictionToggle = document.getElementById('chartPredictionToggle');
    const resetZoom = document.getElementById('chartResetZoom');
    resetZoom.disabled = true;
    resetZoom.textContent = 'Full range';
    resetZoom.addEventListener('click', (event) => {
      chart.resetZoom();
      event.currentTarget.blur();
    });
    chart.setPredictionEnabled(chart.loadPredictionPreference());
    predictionToggle.addEventListener('click', (event) => {
      chart.setPredictionEnabled(event.currentTarget.getAttribute('aria-pressed') !== 'true');
      event.currentTarget.blur();
    });
    document.getElementById('chartDownloadPng').addEventListener('click', chart.downloadPng);
    document.getElementById('chartDownloadCsv').addEventListener('click', chart.downloadCsv);
    chart.attachInteractions(chartCanvas);

    const renderStatusUnavailable = () => {
      setText('temperature', '-- °C');
      const sensor = document.getElementById('sensor');
      sensor.textContent = 'UNAVAILABLE';
      sensor.className = 'value small bad';
      setState('peltier', false);
      setState('fan', false);
      setText('runon', 'OFF');
      setText('count', '--');
      setText('target', '--');
      setText('uptime', '--');
      setText('wifi', 'Status unavailable');
      setText('ota', 'Status unavailable');
      chart.draw();
    };

    const renderStatus = (data) => {
      chart.addStatusSample(data);
      const available = data.hasTemperature && !data.sensorDisconnected;
      setText('temperature', available
        ? formatTemperature(data.filteredTemperatureC ?? data.temperatureC)
        : '-- °C');
      const sensor = document.getElementById('sensor');
      sensor.textContent = available ? 'OK' : 'UNAVAILABLE';
      sensor.className = 'value small ' + (available ? 'ok' : 'bad');
      setState('peltier', Boolean(data.peltierRunning));
      setState('fan', Boolean(data.fanRunning));
      setText('runon', data.fanRunOnActive
        ? Math.ceil(Number(data.fanRunOnRemainingMs) / 1000) + ' s'
        : 'OFF');
      setText('count', data.updateCount);
      setText('target', formatTemperature(data.targetC) +
        ' / hys ' + Number(data.hysteresisC).toFixed(1) + ' °C');
      setText('uptime', Math.floor(Number(data.uptimeMs) / 1000) + ' s');
      setText('wifi', data.stationSsid
        ? (data.stationConnected ? data.stationIp : (data.stationStatus || 'Disconnected'))
        : (data.stationLastFailure ? 'Failed: ' + data.stationLastFailure : 'Disabled'));
      setText('ota', window.SystemOta.statusText(data));
      network.renderStatus(data);
      window.SystemOta.renderStatus(data);
      settings.apply(data);
      chart.draw();
    };

    const refresh = async () => {
      try {
        renderStatus(await api.status());
      } catch (error) {
        renderStatusUnavailable();
      }
    };

    settingsForm.addEventListener('submit', async (event) => {
      event.preventDefault();
      const validationError = network.validate();
      if (validationError) {
        saveState.textContent = validationError;
        return;
      }
      saveState.textContent = 'Saving...';
      try {
        settings.markSaved(await api.saveSettings(settingsForm));
        await refresh();
      } catch (error) {
        saveState.textContent = 'Save failed';
      }
    });

    window.addEventListener('resize', chart.draw);
    document.getElementById('themeToggle').addEventListener('click', () => {
      window.setTimeout(chart.draw, 0);
    });
    chart.loadHistory();
    refresh();
    window.setInterval(refresh, 1000);
  });
}());

  </script>
</body>
</html>

)cooling_html";
inline const char *webPageForPath(const String &path)
{
  if (path == "/" || path == "/dashboard.html" || path == "/apps/cooling/dashboard" || path == "/apps/cooling/dashboard.html") { return kIndexHtml; }
  return nullptr;
}
