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
