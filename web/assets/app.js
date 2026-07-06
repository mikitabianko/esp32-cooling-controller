(function () {
  const storageKeys = {
    theme: 'cooling-dashboard.theme',
    legacyTheme: 'theme'
  };
  const setText = (id, text) => document.getElementById(id).textContent = text;
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
    initTheme,
    setText
  };
  document.addEventListener('DOMContentLoaded', initTheme);
}());
