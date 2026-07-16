#pragma once

#include <Arduino.h>

// Generated from web/apps/status/manifest.json and web/apps/status/dashboard.html.
const char kIndexHtml[] PROGMEM = R"status_devi_html(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Status Device</title>
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
.status-led {
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.status-led::before {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  background: var(--muted);
  content: "";
}

.status-led.on::before {
  background: var(--ok);
  box-shadow: 0 0 12px var(--ok);
}

  </style>
</head>
<body>
  <main class="narrow">
    <header>
      <h1 id="deviceTitle">Status Device</h1>
      <div class="actions">
        <button id="settingsOpen" type="button">Settings</button>
        <button id="themeToggle" type="button">Dark theme</button>
      </div>
    </header>
    <section class="grid wide">
      <div class="card"><div class="label">Status LED</div><div id="led" class="value small status-led">--</div></div>
      <div class="card"><div class="label">Toggles</div><div id="toggleCount" class="value small">--</div></div>
      <div class="card"><div class="label">Blink interval</div><div id="blinkInterval" class="value small">--</div></div>
      <div class="card"><div class="label">Uptime</div><div id="uptime" class="value small">--</div></div>
      <div class="card"><div class="label">WiFi network</div><div id="wifi" class="value small">--</div></div>
      <div class="card"><div class="label">OTA</div><div id="ota" class="value small">--</div></div>
    </section>
  </main>

  <div id="settingsOverlay" class="settings-overlay" role="dialog" aria-modal="true" aria-labelledby="settingsTitle">
    <section class="settings-dialog">
      <div class="settings-head">
        <h2 id="settingsTitle">Device settings</h2>
        <button id="settingsClose" type="button">Close</button>
      </div>
      <form id="settingsForm">
        <section class="settings-section">
          <h3 class="section-title">Device</h3>
          <label class="label">Display name<input id="deviceNameInput" name="deviceName" type="text" maxlength="32" required></label>
          <label class="label">Blink interval, ms<input id="blinkIntervalInput" name="blinkIntervalMs" type="number" min="100" max="60000" step="100" required></label>
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
              <span><strong>Available networks</strong><small id="networkScanState">Not scanned yet</small></span>
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
  const json = (path, options, timeoutMs) => (
    window.DeviceWeb.api.requestJson(path, options, timeoutMs)
  );

  window.StatusApi = {
    saveSettings: (form) => json('/api/blink/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams(new FormData(form))
    }, 3000),
    status: () => json('/api/blink/status', { cache: 'no-store' }, 900)
  };
}());

  </script>
  <script>
(function () {
  const init = ({ form, saveState, network }) => {
    let dirty = false;
    const apply = (data) => {
      if (dirty) return;
      document.getElementById('deviceNameInput').value = data.deviceName || 'Status Device';
      document.getElementById('blinkIntervalInput').value = data.blinkIntervalMs;
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
    return { apply, markDirty, markSaved };
  };

  window.StatusSettings = { init };
}());

  </script>
  <script>
(function () {
  document.addEventListener('DOMContentLoaded', () => {
    const { setText, settingsDialog } = window.DeviceWeb;
    const form = document.getElementById('settingsForm');
    const saveState = document.getElementById('saveState');
    const network = window.SystemNetwork.init({
      onDirty: (message) => settings.markDirty(message)
    });
    const settings = window.StatusSettings.init({ form, saveState, network });

    settingsDialog.init({
      overlay: document.getElementById('settingsOverlay'),
      openButton: document.getElementById('settingsOpen'),
      closeButton: document.getElementById('settingsClose'),
      initialFocus: document.getElementById('deviceNameInput')
    });
    window.SystemOta.initUploader();

    const unavailable = () => {
      setText('deviceTitle', 'Status Device');
      setText('led', 'UNAVAILABLE');
      document.getElementById('led').className = 'value small status-led bad';
      setText('toggleCount', '--');
      setText('blinkInterval', '--');
      setText('uptime', '--');
      setText('wifi', 'Status unavailable');
      setText('ota', 'Status unavailable');
    };

    const render = (data) => {
      setText('deviceTitle', data.deviceName || 'Status Device');
      setText('led', data.ledOn ? 'ON' : 'OFF');
      document.getElementById('led').className =
        'value small status-led ' + (data.ledOn ? 'on ok' : '');
      setText('toggleCount', data.toggleCount);
      setText('blinkInterval', data.blinkIntervalMs + ' ms');
      setText('uptime', Math.floor(Number(data.uptimeMs) / 1000) + ' s');
      setText('wifi', data.stationSsid
        ? (data.stationConnected ? data.stationIp : (data.stationStatus || 'Disconnected'))
        : (data.stationLastFailure ? 'Failed: ' + data.stationLastFailure : 'Disabled'));
      setText('ota', window.SystemOta.statusText(data));
      network.renderStatus(data);
      window.SystemOta.renderStatus(data);
      settings.apply(data);
    };

    const refresh = async () => {
      try {
        render(await window.StatusApi.status());
      } catch (error) {
        unavailable();
      }
    };

    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      const validationError = network.validate();
      if (validationError) {
        saveState.textContent = validationError;
        return;
      }
      saveState.textContent = 'Saving...';
      try {
        settings.markSaved(await window.StatusApi.saveSettings(form));
        await refresh();
      } catch (error) {
        saveState.textContent = 'Save failed';
      }
    });

    refresh();
    window.setInterval(refresh, 1000);
  });
}());

  </script>
</body>
</html>

)status_devi_html";
inline const char *webPageForPath(const String &path)
{
  if (path == "/" || path == "/dashboard.html" || path == "/apps/status/dashboard" || path == "/apps/status/dashboard.html") { return kIndexHtml; }
  return nullptr;
}
