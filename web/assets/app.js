(function () {
  const storageKeys = {
    theme: 'cooling-dashboard.theme',
    legacyTheme: 'theme',
    devState: 'cooling-dashboard.devState'
  };
  const defaultDevState = {
    ok: true,
    enabled: false,
    temperatureC: 5.0,
    hasTemperature: true,
    sensorDisconnected: false,
    updateCount: 0,
    peltierRunning: false,
    fanRunning: false,
    fanRunOnActive: false,
    fanRunOnRemainingMs: 0
  };
  const setText = (id, text) => document.getElementById(id).textContent = text;
  const readStoredDevState = () => {
    try {
      return { ...defaultDevState, ...JSON.parse(localStorage.getItem(storageKeys.devState) || '{}') };
    } catch (error) {
      return { ...defaultDevState };
    }
  };
  const storeDevState = (data) => {
    localStorage.setItem(storageKeys.devState, JSON.stringify({ ...defaultDevState, ...data }));
  };
  const initTheme = () => {
    const themeToggle = document.getElementById('themeToggle');
    if (!themeToggle) return;
    const applyTheme = (theme) => {
      document.documentElement.dataset.theme = theme;
      themeToggle.textContent = theme === 'dark' ? 'Light theme' : 'Dark theme';
      localStorage.setItem(storageKeys.theme, theme);
      localStorage.setItem(storageKeys.legacyTheme, theme);
    };
    applyTheme(localStorage.getItem(storageKeys.theme) || localStorage.getItem(storageKeys.legacyTheme) || 'light');
    themeToggle.addEventListener('click', () => {
      applyTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark');
    });
  };
  window.CoolingWeb = {
    storageKeys,
    defaultDevState,
    initTheme,
    readStoredDevState,
    storeDevState,
    setText
  };
  document.addEventListener('DOMContentLoaded', initTheme);
}());
