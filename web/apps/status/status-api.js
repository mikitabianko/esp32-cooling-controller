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
