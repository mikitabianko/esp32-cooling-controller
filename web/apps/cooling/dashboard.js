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
