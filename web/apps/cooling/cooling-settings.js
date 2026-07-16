(function () {
  const init = ({ form, saveState, network }) => {
    let dirty = false;
    const apply = (data) => {
      if (dirty) return;
      document.getElementById('targetInput').value = Number(data.targetC).toFixed(1);
      document.getElementById('hysteresisInput').value = Number(data.hysteresisC).toFixed(1);
      document.getElementById('measurementInput').value = data.measurementIntervalMs;
      document.getElementById('fanRunOnInput').value = data.fanRunOnMs;
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
    return { apply, isDirty: () => dirty, markDirty, markSaved };
  };

  window.CoolingSettings = { init };
}());
