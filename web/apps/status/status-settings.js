(function () {
  const init = ({ form, saveState, network }) => {
    let dirty = false;
    const apply = (data) => {
      if (dirty) return;
      document.getElementById('deviceNameInput').value = data.deviceName || 'Status Device';
      document.getElementById('blinkIntervalInput').value = data.blinkIntervalMs;
      network.applySettings(data);
    };
    const markDirty = (message) => {
      dirty = true;
      saveState.textContent = message || '';
    };
    const markSaved = (data) => {
      dirty = false;
      apply(data);
      saveState.textContent = 'Saved';
    };
    form.addEventListener('input', () => markDirty(''));
    return { apply, markDirty, markSaved };
  };

  window.StatusSettings = { init };
}());
