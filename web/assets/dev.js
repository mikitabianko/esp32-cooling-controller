(function () {
  const { readStoredDevState, storeDevState, setText } = window.CoolingWeb;
  const ids = {
    enabled: 'enabledInput',
    temperatureC: 'temperatureInput',
    hasTemperature: 'hasTemperatureInput',
    sensorDisconnected: 'sensorDisconnectedInput',
    peltierRunning: 'peltierInput',
    fanRunning: 'fanInput',
    fanRunOnActive: 'runOnInput',
    fanRunOnRemainingMs: 'remainingInput',
    updateCount: 'updatesInput'
  };
  const setChecked = (id, value) => document.getElementById(id).checked = Boolean(value);
  const setValue = (id, value) => document.getElementById(id).value = value;

  document.addEventListener('DOMContentLoaded', () => {
    const saveState = document.getElementById('saveState');
    const setSaveState = (text, isError = false) => {
      saveState.textContent = text;
      saveState.className = isError ? 'danger' : '';
    };
    const formData = () => {
      const body = new URLSearchParams();
      body.set('enabled', document.getElementById(ids.enabled).checked ? '1' : '0');
      body.set('temperatureC', document.getElementById(ids.temperatureC).value || '0');
      body.set('hasTemperature', document.getElementById(ids.hasTemperature).checked ? '1' : '0');
      body.set('sensorDisconnected', document.getElementById(ids.sensorDisconnected).checked ? '1' : '0');
      body.set('peltierRunning', document.getElementById(ids.peltierRunning).checked ? '1' : '0');
      body.set('fanRunning', document.getElementById(ids.fanRunning).checked ? '1' : '0');
      body.set('fanRunOnActive', document.getElementById(ids.fanRunOnActive).checked ? '1' : '0');
      body.set('fanRunOnRemainingMs', document.getElementById(ids.fanRunOnRemainingMs).value || '0');
      body.set('updateCount', document.getElementById(ids.updateCount).value || '0');
      return body;
    };
    const applyState = (data) => {
      storeDevState(data);
      setChecked(ids.enabled, data.enabled);
      setValue(ids.temperatureC, Number(data.temperatureC).toFixed(1));
      setChecked(ids.hasTemperature, data.hasTemperature);
      setChecked(ids.sensorDisconnected, data.sensorDisconnected);
      setChecked(ids.peltierRunning, data.peltierRunning);
      setChecked(ids.fanRunning, data.fanRunning);
      setChecked(ids.fanRunOnActive, data.fanRunOnActive);
      setValue(ids.fanRunOnRemainingMs, data.fanRunOnRemainingMs);
      setValue(ids.updateCount, data.updateCount);
      setText('mode', data.enabled ? 'ON' : 'OFF');
      setText('temperature', data.hasTemperature ? Number(data.temperatureC).toFixed(1) + ' C' : '--');
      setText('peltier', data.peltierRunning ? 'ON' : 'OFF');
      setText('fan', data.fanRunning ? 'ON' : 'OFF');
    };
    async function loadDev() {
      try {
        const res = await fetch('/api/dev', { cache: 'no-store' });
        if (!res.ok) throw new Error('dev state unavailable');
        applyState(await res.json());
      } catch (error) {
        applyState(readStoredDevState());
        setSaveState('Local state');
      }
    }
    async function saveDev(body) {
      setSaveState('Saving...');
      const localState = {
        ok: true,
        enabled: body.get('enabled') === '1',
        temperatureC: Number(body.get('temperatureC')),
        hasTemperature: body.get('hasTemperature') === '1',
        sensorDisconnected: body.get('sensorDisconnected') === '1',
        updateCount: Number(body.get('updateCount')),
        peltierRunning: body.get('peltierRunning') === '1',
        fanRunning: body.get('fanRunning') === '1',
        fanRunOnActive: body.get('fanRunOnActive') === '1',
        fanRunOnRemainingMs: Number(body.get('fanRunOnRemainingMs'))
      };
      try {
        const res = await fetch('/api/dev', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        if (!res.ok) throw new Error('save failed');
        applyState(await res.json());
        setSaveState('Applied');
      } catch (error) {
        applyState(localState);
        setSaveState('Applied locally');
      }
    }
    document.getElementById('devForm').addEventListener('submit', async (event) => {
      event.preventDefault();
      try {
        await saveDev(formData());
      } catch (error) {
        setSaveState('Apply failed', true);
      }
    });
    document.getElementById('resetBtn').addEventListener('click', async () => {
      const body = formData();
      body.set('enabled', '0');
      await saveDev(body);
    });
    document.getElementById('refreshBtn').addEventListener('click', loadDev);
    loadDev();
  });
}());
