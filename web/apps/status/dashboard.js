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
