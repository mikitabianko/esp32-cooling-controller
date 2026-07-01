(function () {
  const { readStoredDevState, setText } = window.CoolingWeb;
  const startedAt = Date.now();
  const chartHistoryMs = 30 * 60 * 1000;
  const temperatureSeries = [
    {
      id: 'probe1',
      label: 'Probe 1',
      valueKey: 'temperatureC',
      hasKey: 'hasTemperature',
      invalidKey: 'sensorDisconnected',
      updateCountKey: 'updateCount',
      colorVar: '--chart-primary',
      primary: true
    },
    {
      id: 'probe2',
      label: 'Probe 2',
      valueKey: 'secondaryTemperatureC',
      hasKey: 'hasSecondaryTemperature',
      invalidKey: 'secondarySensorDisconnected',
      updateCountKey: 'secondaryUpdateCount',
      colorVar: '--chart-secondary'
    }
  ];
  const chartSamples = new Map(temperatureSeries.map((series) => [series.id, []]));
  const lastChartSampleKeys = new Map();
  let lastChartUptimeMs = 0;
  const setState = (id, on) => {
    const el = document.getElementById(id);
    el.textContent = on ? 'ON' : 'OFF';
    el.className = 'value small ' + (on ? 'ok' : '');
  };
  const isSeriesAvailable = (series, data) => (
    series.primary || Object.prototype.hasOwnProperty.call(data, series.valueKey)
  );
  const hasValidSeriesValue = (series, data) => {
    const value = Number(data[series.valueKey]);
    const hasValue = series.hasKey ? Boolean(data[series.hasKey]) : Number.isFinite(value);
    const invalid = series.invalidKey ? Boolean(data[series.invalidKey]) : false;
    return hasValue && !invalid && Number.isFinite(value);
  };
  const sampleTimeMs = (data) => {
    const uptimeMs = Number(data.uptimeMs);
    return Number.isFinite(uptimeMs) ? uptimeMs : Date.now() - startedAt;
  };
  const trimChartSamples = (samples, latestTimeMs) => {
    const cutoffMs = latestTimeMs - chartHistoryMs;
    while (samples.length && samples[0].timeMs < cutoffMs) {
      samples.shift();
    }
  };
  const addSeriesSample = (series, timeMs, value) => {
    const samples = chartSamples.get(series.id);
    if (!samples || !Number.isFinite(timeMs) || !Number.isFinite(value)) return;
    if (samples.some((sample) => sample.timeMs === timeMs)) return;
    samples.push({ timeMs, value });
    samples.sort((left, right) => left.timeMs - right.timeMs);
    trimChartSamples(samples, Math.max(timeMs, lastChartUptimeMs));
  };
  const addChartSample = (data) => {
    const timeMs = sampleTimeMs(data);
    if (timeMs + 1000 < lastChartUptimeMs) {
      chartSamples.forEach((samples) => samples.splice(0, samples.length));
      lastChartSampleKeys.clear();
    }
    lastChartUptimeMs = timeMs;

    temperatureSeries.forEach((series) => {
      if (!isSeriesAvailable(series, data)) return;
      const samples = chartSamples.get(series.id);
      const count = series.updateCountKey && data[series.updateCountKey] !== undefined
        ? data[series.updateCountKey]
        : Math.floor(timeMs / 1000);
      const sampleKey = String(count) + ':' + String(data[series.valueKey]);
      if (lastChartSampleKeys.get(series.id) === sampleKey) return;
      lastChartSampleKeys.set(series.id, sampleKey);
      if (hasValidSeriesValue(series, data)) {
        addSeriesSample(series, timeMs, Number(data[series.valueKey]));
      } else {
        trimChartSamples(samples, timeMs);
      }
    });
  };
  const applyHistory = (history) => {
    if (!history || !Array.isArray(history.series)) return;
    history.series.forEach((remoteSeries) => {
      const series = temperatureSeries.find((item) => item.id === remoteSeries.id);
      if (!series || !Array.isArray(remoteSeries.points)) return;
      remoteSeries.points.forEach((point) => {
        if (!Array.isArray(point) || point.length < 2) return;
        const flags = Number(point[2]) || 0;
        if ((flags & 1) !== 0) return;
        addSeriesSample(series, Number(point[0]), Number(point[1]) / 10);
      });
    });
  };
  const loadHistory = async () => {
    try {
      const res = await fetch('/api/history', { cache: 'no-store' });
      if (!res.ok) throw new Error('history unavailable');
      applyHistory(await res.json());
      drawTemperatureChart();
    } catch (error) {
      // History is optional; live samples will continue filling the chart.
    }
  };
  const formatAgo = (ms) => {
    const minutes = Math.max(0, Math.round(ms / 60000));
    return minutes <= 0 ? 'now' : '-' + minutes + ' min';
  };
  const chartColors = (canvas, series) => {
    const style = getComputedStyle(document.documentElement);
    return {
      text: style.getPropertyValue('--muted').trim(),
      grid: style.getPropertyValue('--chart-grid').trim(),
      line: style.getPropertyValue(series.colorVar).trim()
    };
  };
  const renderChartLegend = (legend) => {
    legend.textContent = '';
    temperatureSeries.filter((series) => chartSamples.get(series.id).length > 0).forEach((series) => {
      const item = document.createElement('span');
      item.className = 'legend-item';
      item.style.color = getComputedStyle(document.documentElement).getPropertyValue(series.colorVar).trim();
      const swatch = document.createElement('span');
      swatch.className = 'legend-swatch';
      const label = document.createElement('span');
      label.textContent = series.label;
      item.append(swatch, label);
      legend.append(item);
    });
  };
  const drawTemperatureChart = () => {
    const canvas = document.getElementById('temperatureChart');
    const empty = document.getElementById('chartEmpty');
    const legend = document.getElementById('chartLegend');
    if (!canvas || !empty || !legend) return;

    const allSamples = temperatureSeries.flatMap((series) => chartSamples.get(series.id));
    empty.classList.toggle('hidden', allSamples.length > 0);
    renderChartLegend(legend);

    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(1, Math.round(rect.width * dpr));
    const height = Math.max(1, Math.round(rect.height * dpr));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }

    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, width, height);
    if (!allSamples.length) return;

    const nowMs = Math.max(...allSamples.map((sample) => sample.timeMs));
    const startMs = Math.max(0, nowMs - chartHistoryMs);
    const minValue = Math.min(...allSamples.map((sample) => sample.value));
    const maxValue = Math.max(...allSamples.map((sample) => sample.value));
    const valuePadding = Math.max(0.5, (maxValue - minValue) * 0.18);
    const yMin = Math.floor((minValue - valuePadding) * 2) / 2;
    const yMax = Math.ceil((maxValue + valuePadding) * 2) / 2;
    const span = Math.max(1, yMax - yMin);
    const pad = {
      left: 42 * dpr,
      right: 10 * dpr,
      top: 12 * dpr,
      bottom: 28 * dpr
    };
    const plotW = Math.max(1, width - pad.left - pad.right);
    const plotH = Math.max(1, height - pad.top - pad.bottom);
    const axisColor = chartColors(canvas, temperatureSeries[0]);
    const xFor = (timeMs) => pad.left + ((timeMs - startMs) / chartHistoryMs) * plotW;
    const yFor = (value) => pad.top + (1 - ((value - yMin) / span)) * plotH;

    ctx.font = String(11 * dpr) + 'px system-ui, sans-serif';
    ctx.lineWidth = 1 * dpr;
    ctx.strokeStyle = axisColor.grid;
    ctx.fillStyle = axisColor.text;
    ctx.textBaseline = 'middle';

    for (let i = 0; i <= 4; i += 1) {
      const y = pad.top + (plotH / 4) * i;
      const value = yMax - (span / 4) * i;
      ctx.beginPath();
      ctx.moveTo(pad.left, y);
      ctx.lineTo(width - pad.right, y);
      ctx.stroke();
      ctx.textAlign = 'right';
      ctx.fillText(value.toFixed(1), pad.left - 8 * dpr, y);
    }

    ctx.textBaseline = 'top';
    for (let i = 0; i <= 3; i += 1) {
      const x = pad.left + (plotW / 3) * i;
      const ageMs = chartHistoryMs - (chartHistoryMs / 3) * i;
      ctx.beginPath();
      ctx.moveTo(x, pad.top);
      ctx.lineTo(x, pad.top + plotH);
      ctx.stroke();
      ctx.textAlign = i === 0 ? 'left' : (i === 3 ? 'right' : 'center');
      ctx.fillText(formatAgo(ageMs), x, pad.top + plotH + 8 * dpr);
    }

    temperatureSeries.forEach((series) => {
      const samples = chartSamples.get(series.id).filter((sample) => sample.timeMs >= startMs);
      if (!samples.length) return;
      ctx.strokeStyle = chartColors(canvas, series).line;
      ctx.lineWidth = 2 * dpr;
      ctx.beginPath();
      samples.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.value);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
    });
  };
  const mockStatus = () => {
    const elapsedMs = Date.now() - startedAt;
    const peltierRunning = Math.floor(elapsedMs / 7000) % 2 === 0;
    return {
      temperatureC: 5.4 + Math.sin(elapsedMs / 3500) * 0.8,
      hasTemperature: true,
      sensorDisconnected: false,
      updateCount: Math.floor(elapsedMs / 1000),
      peltierRunning,
      fanRunning: peltierRunning || Math.floor(elapsedMs / 3000) % 5 === 0,
      fanRunOnActive: !peltierRunning && Math.floor(elapsedMs / 3000) % 5 === 0,
      fanRunOnRemainingMs: 12000 - (elapsedMs % 12000),
      targetC: 5.0,
      hysteresisC: 0.1,
      measurementIntervalMs: 500,
      fanRunOnMs: 30000,
      uptimeMs: elapsedMs,
      accessPointSsid: 'CoolingController',
      accessPointIp: '',
      stationSsid: '',
      stationPasswordSet: false,
      stationConnected: false,
      stationStatus: 'disabled',
      stationIp: '',
      stationRssi: 0,
      devMode: false,
      demo: true
    };
  };
  const mockFromDevState = (devState, elapsedMs) => ({
    temperatureC: Number(devState.temperatureC) || 0,
    hasTemperature: Boolean(devState.hasTemperature),
    sensorDisconnected: Boolean(devState.sensorDisconnected),
    updateCount: Number(devState.updateCount) || 0,
    peltierRunning: Boolean(devState.peltierRunning),
    fanRunning: Boolean(devState.fanRunning),
    fanRunOnActive: Boolean(devState.fanRunOnActive),
    fanRunOnRemainingMs: Number(devState.fanRunOnRemainingMs) || 0,
    targetC: 5.0,
    hysteresisC: 0.1,
    measurementIntervalMs: 500,
    fanRunOnMs: 30000,
    uptimeMs: elapsedMs,
    accessPointSsid: 'CoolingController',
    accessPointIp: '',
    stationSsid: '',
    stationPasswordSet: false,
    stationConnected: false,
    stationStatus: 'disabled',
    stationIp: '',
    stationRssi: 0,
    devMode: true,
    demo: true
  });
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

  document.addEventListener('DOMContentLoaded', () => {
    const demoBadge = document.getElementById('demoBadge');
    const devBadge = document.getElementById('devBadge');
    const settingsForm = document.getElementById('settingsForm');
    const settingsOverlay = document.getElementById('settingsOverlay');
    const settingsOpen = document.getElementById('settingsOpen');
    const settingsClose = document.getElementById('settingsClose');
    const scanNetworks = document.getElementById('scanNetworks');
    const networkList = document.getElementById('networkList');
    const networkScanState = document.getElementById('networkScanState');
    const saveState = document.getElementById('saveState');
    const stationSsidInput = document.getElementById('stationSsidInput');
    const stationPasswordInput = document.getElementById('stationPasswordInput');
    const toggleStationPassword = document.getElementById('toggleStationPassword');
    const hiddenNetworkInput = document.getElementById('hiddenNetworkInput');
    const forgetStationNetworkInput = document.getElementById('forgetStationNetworkInput');
    const forgetStationNetwork = document.getElementById('forgetStationNetwork');
    let settingsDirty = false;
    let networkScanStartedAt = 0;
    let currentSavedStationSsid = '';
    let selectedNetworkSsid = '';
    let visibleNetworkSsids = new Set();
    const setHiddenNetworkMode = (enabled, keepValue) => {
      hiddenNetworkInput.checked = enabled;
      stationSsidInput.readOnly = !enabled;
      stationSsidInput.placeholder = enabled ? 'Enter hidden network SSID' : 'Select from nearby networks';
      if (enabled) {
        selectedNetworkSsid = '';
        if (!keepValue) stationSsidInput.value = '';
      }
      if (!enabled && selectedNetworkSsid) {
        stationSsidInput.value = selectedNetworkSsid;
      }
    };
    const applySettingsToForm = (data) => {
      if (settingsDirty) return;
      document.getElementById('targetInput').value = Number(data.targetC).toFixed(1);
      document.getElementById('hysteresisInput').value = Number(data.hysteresisC).toFixed(1);
      document.getElementById('measurementInput').value = data.measurementIntervalMs;
      document.getElementById('fanRunOnInput').value = data.fanRunOnMs;
      currentSavedStationSsid = data.stationSsid || '';
      selectedNetworkSsid = currentSavedStationSsid;
      stationSsidInput.value = currentSavedStationSsid;
      stationPasswordInput.value = '';
      stationPasswordInput.type = 'password';
      toggleStationPassword.textContent = 'Show';
      toggleStationPassword.setAttribute('aria-pressed', 'false');
      stationPasswordInput.placeholder = data.stationPasswordSet ? 'Saved; leave blank to keep' : 'Leave blank for open network';
      forgetStationNetworkInput.value = '0';
      setHiddenNetworkMode(false, true);
    };
    const renderNetworkStatus = (data) => {
      setText('apStatus', data.accessPointSsid || '--');
      setText('apPageStatus', pageUrl(data.accessPointIp));
      setText('stationStatus', data.stationSsid ? data.stationSsid : (data.stationLastFailure ? 'Failed' : 'Not connected'));
      setText('stationPageStatus', data.stationConnected ? pageUrl(data.stationIp) : (data.stationSsid ? (data.stationStatus || 'disconnected') : 'Wi-Fi off'));
      setText('currentPageStatus', window.location.origin || '--');
    };
    const renderNetworks = (networks) => {
      networkList.textContent = '';
      visibleNetworkSsids = new Set();
      if (!networks.length) {
        networkScanState.textContent = 'No 2.4 GHz networks found';
        return;
      }
      networkScanState.textContent = networks.length + ' found';
      networks.forEach((network) => {
        if (network.ssid) visibleNetworkSsids.add(network.ssid);
        const item = document.createElement('button');
        item.type = 'button';
        item.className = 'network-item';
        const selected = Boolean(network.ssid) && network.ssid === stationSsidInput.value && !hiddenNetworkInput.checked;
        item.classList.toggle('selected', selected);
        item.addEventListener('click', () => {
          if (network.ssid) {
            selectedNetworkSsid = network.ssid;
            forgetStationNetworkInput.value = '0';
            setHiddenNetworkMode(false, false);
          } else {
            forgetStationNetworkInput.value = '0';
            setHiddenNetworkMode(true, false);
            stationSsidInput.focus();
          }
          renderNetworks(networks);
          settingsDirty = true;
          saveState.textContent = '';
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
          signalQuality(Number(network.rssi)),
          Number(network.rssi) + ' dBm',
          'ch ' + network.channel,
          network.encrypted ? 'secured' : 'open'
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
        networkList.append(item);
      });
    };
    const loadNetworks = async (polling) => {
      if (!polling) {
        networkScanStartedAt = Date.now();
      }
      scanNetworks.disabled = true;
      networkScanState.textContent = polling ? 'Still scanning...' : 'Scanning...';
      try {
        const res = await fetch('/api/networks', { cache: 'no-store' });
        if (!res.ok) throw new Error('scan unavailable');
        const data = await res.json();
        if (data.scanStatus === 'scanning') {
          if (Date.now() - networkScanStartedAt > 15000) {
            networkList.textContent = '';
            networkScanState.textContent = 'Scan timeout';
            return;
          }
          window.setTimeout(() => loadNetworks(true), 600);
          return;
        }
        if (!data.ok) {
          networkList.textContent = '';
          networkScanState.textContent = 'Scan ' + (data.scanStatus || 'failed');
          return;
        }
        renderNetworks(Array.isArray(data.networks) ? data.networks : []);
      } catch (error) {
        renderNetworks([
          { ssid: 'Kitchen WiFi', rssi: -48, channel: 6, encrypted: true },
          { ssid: 'Workshop', rssi: -66, channel: 11, encrypted: true },
          { ssid: 'Guest', rssi: -78, channel: 1, encrypted: false }
        ]);
        networkScanState.textContent = 'Demo scan';
      } finally {
        if (networkScanState.textContent !== 'Still scanning...' && networkScanState.textContent !== 'Scanning...') {
          scanNetworks.disabled = false;
        }
      }
    };
    const openSettings = () => {
      settingsOverlay.classList.add('open');
      settingsOpen.setAttribute('aria-expanded', 'true');
      document.getElementById('targetInput').focus();
    };
    const closeSettings = () => {
      settingsOverlay.classList.remove('open');
      settingsOpen.setAttribute('aria-expanded', 'false');
      settingsOpen.focus();
    };
    settingsOpen.setAttribute('aria-expanded', 'false');
    settingsOpen.addEventListener('click', openSettings);
    settingsClose.addEventListener('click', closeSettings);
    scanNetworks.addEventListener('click', () => loadNetworks(false));
    toggleStationPassword.addEventListener('click', () => {
      const showing = stationPasswordInput.type === 'text';
      stationPasswordInput.type = showing ? 'password' : 'text';
      toggleStationPassword.textContent = showing ? 'Show' : 'Hide';
      toggleStationPassword.setAttribute('aria-pressed', showing ? 'false' : 'true');
      stationPasswordInput.focus();
    });
    hiddenNetworkInput.addEventListener('change', () => {
      forgetStationNetworkInput.value = '0';
      setHiddenNetworkMode(hiddenNetworkInput.checked, true);
      settingsDirty = true;
      saveState.textContent = '';
      if (hiddenNetworkInput.checked) stationSsidInput.focus();
    });
    forgetStationNetwork.addEventListener('click', () => {
      selectedNetworkSsid = '';
      stationSsidInput.value = '';
      stationPasswordInput.value = '';
      forgetStationNetworkInput.value = '1';
      setHiddenNetworkMode(false, true);
      settingsDirty = true;
      saveState.textContent = 'Network will be forgotten on save';
    });
    settingsOverlay.addEventListener('click', (event) => {
      if (event.target === settingsOverlay) closeSettings();
    });
    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && settingsOverlay.classList.contains('open')) closeSettings();
    });
    async function loadStatus() {
      try {
        const res = await fetch('/api/status', { cache: 'no-store' });
        if (!res.ok) throw new Error('status unavailable');
        const data = await res.json();
        data.demo = false;
        return data;
      } catch (error) {
        const devState = readStoredDevState();
        return devState.enabled ? mockFromDevState(devState, Date.now() - startedAt) : mockStatus();
      }
    }
    async function refresh() {
      const data = await loadStatus();
      addChartSample(data);
      demoBadge.classList.toggle('hidden', !data.demo);
      devBadge.classList.toggle('hidden', !data.devMode);
      setText('temperature', data.hasTemperature ? data.temperatureC.toFixed(1) + ' C' : '--');
      const sensor = document.getElementById('sensor');
      sensor.textContent = data.sensorDisconnected ? 'ERROR' : 'OK';
      sensor.className = 'value small ' + (data.sensorDisconnected ? 'bad' : 'ok');
      setState('peltier', data.peltierRunning);
      setState('fan', data.fanRunning);
      setText('runon', data.fanRunOnActive ? Math.ceil(data.fanRunOnRemainingMs / 1000) + ' s' : 'OFF');
      setText('count', data.updateCount);
      setText('target', data.targetC.toFixed(1) + ' C / hys ' + data.hysteresisC.toFixed(1));
      setText('uptime', Math.floor(data.uptimeMs / 1000) + ' s');
      setText('wifi', data.stationSsid ? (data.stationConnected ? data.stationIp : (data.stationStatus || 'Disconnected')) : (data.stationLastFailure ? 'Failed: ' + data.stationLastFailure : 'Disabled'));
      renderNetworkStatus(data);
      applySettingsToForm(data);
      drawTemperatureChart();
    }
    settingsForm.addEventListener('input', () => {
      settingsDirty = true;
      saveState.textContent = '';
    });
    settingsForm.addEventListener('submit', async (event) => {
      event.preventDefault();
      stationSsidInput.value = stationSsidInput.value.trim();
      const stationSsidChanged = stationSsidInput.value !== currentSavedStationSsid;
      if (!hiddenNetworkInput.checked &&
          stationSsidChanged &&
          stationSsidInput.value.length > 0 &&
          !visibleNetworkSsids.has(stationSsidInput.value)) {
        saveState.textContent = 'Scan and select a network first';
        return;
      }
      if (hiddenNetworkInput.checked && stationSsidInput.value.length === 0) {
        saveState.textContent = 'Enter hidden network SSID';
        stationSsidInput.focus();
        return;
      }
      saveState.textContent = 'Saving...';
      try {
        const body = new URLSearchParams(new FormData(settingsForm));
        const res = await fetch('/api/settings', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        if (!res.ok) throw new Error('save failed');
        const data = await res.json();
        settingsDirty = false;
        applySettingsToForm(data);
        saveState.textContent = 'Saved';
        await refresh();
      } catch (error) {
        saveState.textContent = 'Save failed';
      }
    });
    window.addEventListener('resize', drawTemperatureChart);
    document.getElementById('themeToggle').addEventListener('click', () => {
      window.setTimeout(drawTemperatureChart, 0);
    });
    loadHistory();
    refresh();
    setInterval(refresh, 1000);
  });
}());
