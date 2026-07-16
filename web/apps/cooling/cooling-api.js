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
