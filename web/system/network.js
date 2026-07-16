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
