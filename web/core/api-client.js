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
