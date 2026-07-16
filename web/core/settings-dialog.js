(function () {
  const init = ({ overlay, openButton, closeButton, initialFocus }) => {
    const open = () => {
      overlay.classList.add('open');
      openButton.setAttribute('aria-expanded', 'true');
      if (initialFocus) initialFocus.focus();
    };
    const close = () => {
      overlay.classList.remove('open');
      openButton.setAttribute('aria-expanded', 'false');
      openButton.focus();
    };

    openButton.setAttribute('aria-expanded', 'false');
    openButton.addEventListener('click', open);
    closeButton.addEventListener('click', close);
    overlay.addEventListener('click', (event) => {
      if (event.target === overlay) close();
    });
    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && overlay.classList.contains('open')) close();
    });
    return { open, close };
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    settingsDialog: { init }
  };
}());
