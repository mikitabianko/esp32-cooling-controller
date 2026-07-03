(function () {
  const { setText } = window.CoolingWeb;
  const startedAt = Date.now();
  const minChartWindowMs = 5 * 60 * 1000;
  const keepaliveMs = 30 * 1000;
  const disconnectedFlag = 1;
  const demoChartWarmupMs = 6 * 60 * 1000;
  const demoTimeScale = 16;
  const demoSampleIntervalMs = 500;
  const statusFetchTimeoutMs = 900;
  const historyFetchTimeoutMs = 1200;
  const chartSamples = [];
  let lastChartUptimeMs = 0;
  let lastChartSampleKey = '';
  let zoomRange = null;
  let zoomFollowsRight = false;
  let pointerState = null;
  let hasRealStatus = false;
  let demoChartSeeded = false;
  const setState = (id, on) => {
    const el = document.getElementById(id);
    el.textContent = on ? 'ON' : 'OFF';
    el.className = 'value small ' + (on ? 'ok' : '');
  };
  const sampleTimeMs = (data) => {
    const uptimeMs = Number(data.uptimeMs);
    return Number.isFinite(uptimeMs) ? uptimeMs : Date.now() - startedAt;
  };
  const formatTemperature = (value) => (
    Number.isFinite(value) ? value.toFixed(1) + ' °C' : '--'
  );
  const formatTime = (timeMs) => {
    const seconds = Math.max(0, Math.round(timeMs / 1000));
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = seconds % 60;
    return minutes + ':' + String(remainingSeconds).padStart(2, '0');
  };
  const latestChartTime = () => Math.max(lastChartUptimeMs, ...chartSamples.map((sample) => sample.timeMs), 0);
  const isCoarsePointer = () => (
    window.matchMedia && window.matchMedia('(pointer: coarse)').matches
  );
  const pushChartSample = (sample) => {
    const normalizedSample = normalizeChartSample(sample);
    if (!Number.isFinite(normalizedSample.timeMs)) return;
    const existing = chartSamples.findIndex((item) => item.timeMs === normalizedSample.timeMs);
    if (existing >= 0) chartSamples.splice(existing, 1);
    chartSamples.push(normalizedSample);
    chartSamples.sort((left, right) => left.timeMs - right.timeMs);
    lastChartUptimeMs = Math.max(lastChartUptimeMs, normalizedSample.timeMs);
  };
  const numericOrNull = (value) => {
    if (value === null || value === undefined || value === '') return null;
    const number = Number(value);
    return Number.isFinite(number) ? number : null;
  };
  const normalizeChartSample = (sample) => ({
    ...sample,
    timeMs: Number(sample.timeMs),
    temperature: numericOrNull(sample.temperature),
    target: numericOrNull(sample.target),
    disconnected: Boolean(sample.disconnected)
  });
  const addChartSample = (data) => {
    if (data.demo && hasRealStatus) return;
    if (!data.demo && !hasRealStatus && demoChartSeeded) {
      chartSamples.splice(0, chartSamples.length);
      lastChartSampleKey = '';
      zoomRange = null;
      zoomFollowsRight = false;
      demoChartSeeded = false;
    }
    if (!data.demo) hasRealStatus = true;
    const timeMs = sampleTimeMs(data);
    if (timeMs + 1000 < lastChartUptimeMs) {
      chartSamples.splice(0, chartSamples.length);
      lastChartSampleKey = '';
      zoomRange = null;
      zoomFollowsRight = false;
    }
    lastChartUptimeMs = timeMs;
    const disconnected = Boolean(data.sensorDisconnected) || !data.hasTemperature;
    const temperature = disconnected ? null : Number(data.filteredTemperatureC ?? data.temperatureC);
    const target = Number(data.targetC);
    const sampleKey = [
      data.updateCount,
      disconnected ? 'x' : temperature.toFixed(2),
      Number.isFinite(target) ? target.toFixed(2) : ''
    ].join(':');
    if (lastChartSampleKey === sampleKey) return;
    lastChartSampleKey = sampleKey;
    pushChartSample(normalizeChartSample({
      timeMs,
      temperature: Number.isFinite(temperature) ? temperature : null,
      target: Number.isFinite(target) ? target : null,
      disconnected,
      sensorValid: !disconnected,
      status: disconnected ? 'unavailable' : 'ok',
      flags: disconnected ? disconnectedFlag : 0
    }));
    const cutoffMs = latestChartTime() - Math.max(minChartWindowMs, keepaliveMs * 4);
    while (chartSamples.length > 1200 && chartSamples[0].timeMs < cutoffMs) chartSamples.shift();
  };
  const applyHistory = (history) => {
    if (!history || !Array.isArray(history.series)) return;
    const historySamples = [];
    history.series.forEach((remoteSeries) => {
      if (remoteSeries.id !== 'probe1' || !Array.isArray(remoteSeries.points)) return;
      remoteSeries.points.forEach((point) => {
        if (!Array.isArray(point) || point.length < 4) return;
        const flags = Number(point[3]) || 0;
        const disconnected =
          point.length > 5 ? Number(point[5]) === 1 : (flags & disconnectedFlag) !== 0;
        const sensorValid =
          point.length > 4 ? Number(point[4]) === 1 : !disconnected;
        const sample = normalizeChartSample({
          timeMs: Number(point[0]),
          temperature: disconnected ? null : Number(point[1]) / 10,
          target: Number(point[2]) / 10,
          disconnected,
          sensorValid,
          status: sensorValid ? 'ok' : 'unavailable',
          flags
        });
        if (Number.isFinite(sample.timeMs)) historySamples.push(sample);
      });
    });
    if (!historySamples.length && chartSamples.length) return;
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    historySamples.forEach(pushChartSample);
  };
  const loadHistory = async () => {
    try {
      const res = await fetchWithTimeout('/api/history', { cache: 'no-store' }, historyFetchTimeoutMs);
      if (!res.ok) throw new Error('history unavailable');
      applyHistory(await res.json());
      drawTemperatureChart();
    } catch (error) {
      // History is optional; live samples will continue filling the chart.
    }
  };
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
    return Promise.race([fetch(url, requestOptions), timeout])
      .finally(() => {
        if (timeoutId !== null) window.clearTimeout(timeoutId);
      });
  };
  const chartColors = () => {
    const style = getComputedStyle(document.documentElement);
    return {
      text: style.getPropertyValue('--muted').trim(),
      grid: style.getPropertyValue('--chart-grid').trim(),
      temperature: style.getPropertyValue('--chart-primary').trim(),
      target: style.getPropertyValue('--chart-target').trim(),
      disconnected: style.getPropertyValue('--chart-disconnected').trim(),
      selection: style.getPropertyValue('--chart-selection').trim(),
      background: style.getPropertyValue('--card').trim()
    };
  };
  const renderChartLegend = (legend) => {
    legend.textContent = '';
    [
      { label: 'Filtered temperature', color: '--chart-primary' },
      { label: 'Target temperature', color: '--chart-target' },
      { label: 'Sensor unavailable', color: '--chart-disconnected' }
    ].forEach((series) => {
      const item = document.createElement('span');
      item.className = 'legend-item';
      item.style.color = getComputedStyle(document.documentElement).getPropertyValue(series.color).trim();
      const swatch = document.createElement('span');
      swatch.className = 'legend-swatch';
      const label = document.createElement('span');
      label.textContent = series.label;
      item.append(swatch, label);
      legend.append(item);
    });
  };
  const chartRange = (full) => {
    const fullEnd = chartDisplayEnd();
    if (full) return { startMs: 0, endMs: fullEnd, spanMs: fullEnd };
    if (zoomRange) {
      const rawStartMs = Math.min(zoomRange.startMs, zoomRange.endMs);
      const rawEndMs = Math.max(zoomRange.startMs, zoomRange.endMs);
      const spanMs = Math.max(1000, rawEndMs - rawStartMs);
      const liveEnd = zoomConstraintEnd(spanMs);
      const maxStartMs = Math.max(0, liveEnd - spanMs);
      const startMs = zoomFollowsRight ||
        isChartRightEdge(rawEndMs, liveEnd, spanMs) ||
        isChartRightEdge(rawEndMs, fullEnd, spanMs)
        ? maxStartMs
        : clamp(rawStartMs, 0, maxStartMs);
      const endMs = startMs + spanMs;
      zoomRange = { startMs, endMs };
      zoomFollowsRight = isChartRightEdge(endMs, liveEnd, spanMs);
      return { startMs, endMs, spanMs: endMs - startMs };
    }
    return { startMs: 0, endMs: fullEnd, spanMs: fullEnd };
  };
  const panZoomRange = (deltaMs) => {
    if (!zoomRange || !Number.isFinite(deltaMs)) return false;
    const startMs = Math.min(zoomRange.startMs, zoomRange.endMs);
    const endMs = Math.max(zoomRange.startMs, zoomRange.endMs);
    const spanMs = Math.max(1000, endMs - startMs);
    const liveEnd = zoomConstraintEnd(spanMs);
    const maxStartMs = Math.max(0, liveEnd - spanMs);
    const nextStartMs = clamp(startMs + deltaMs, 0, maxStartMs);
    zoomRange = { startMs: nextStartMs, endMs: nextStartMs + spanMs };
    zoomFollowsRight = isChartRightEdge(zoomRange.endMs, liveEnd, spanMs);
    return true;
  };
  const drawChartInto = (canvas, full) => {
    const rect = full ? { width: 1200, height: 620 } : canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(1, Math.round(rect.width * (full ? 1 : dpr)));
    const height = Math.max(1, Math.round(rect.height * (full ? 1 : dpr)));
    if (canvas.width !== width || canvas.height !== height) {
      canvas.width = width;
      canvas.height = height;
    }

    const ctx = canvas.getContext('2d');
    const colors = chartColors();
    ctx.clearRect(0, 0, width, height);
    if (full) {
      ctx.fillStyle = colors.background;
      ctx.fillRect(0, 0, width, height);
    }
    if (!chartSamples.length) return null;

    const range = chartRange(full);
    const visibleSamples = chartSamples.filter((sample) => sample.timeMs >= range.startMs && sample.timeMs <= range.endMs);
    const firstAfterStart = chartSamples.findIndex((sample) => sample.timeMs >= range.startMs);
    const beforeRange = firstAfterStart > 0
      ? chartSamples[firstAfterStart - 1]
      : (firstAfterStart < 0 ? chartSamples[chartSamples.length - 1] : null);
    const firstAfterEnd = chartSamples.find((sample) => sample.timeMs > range.endMs);
    const plotSamples = [beforeRange, ...visibleSamples, firstAfterEnd]
      .filter(Boolean)
      .filter((sample, index, samples) => samples.findIndex((item) => item.timeMs === sample.timeMs) === index)
      .sort((left, right) => left.timeMs - right.timeMs);
    const values = plotSamples.flatMap((sample) => [sample.temperature, sample.target].filter(Number.isFinite));
    if (!values.length && zoomRange && !full) {
      zoomRange = null;
      zoomFollowsRight = false;
      return drawChartInto(canvas, false);
    }
    if (!values.length) return null;
    const minValue = Math.min(...values);
    const maxValue = Math.max(...values);
    const valuePadding = Math.max(0.5, (maxValue - minValue) * 0.18);
    const yMin = Math.floor((minValue - valuePadding) * 2) / 2;
    const yMax = Math.ceil((maxValue + valuePadding) * 2) / 2;
    const span = Math.max(1, yMax - yMin);
    const scale = full ? 1 : dpr;
    const compact = !full && canvas.getBoundingClientRect().width < 440;
    const pad = {
      left: (compact ? 54 : 76) * scale,
      right: (compact ? 8 : 18) * scale,
      top: (full ? 54 : (compact ? 14 : 18)) * scale,
      bottom: (compact ? 42 : 50) * scale
    };
    const plotW = Math.max(1, width - pad.left - pad.right);
    const plotH = Math.max(1, height - pad.top - pad.bottom);
    const xFor = (timeMs) => pad.left + ((timeMs - range.startMs) / range.spanMs) * plotW;
    const yFor = (value) => pad.top + (1 - ((value - yMin) / span)) * plotH;

    plotSamples.forEach((sample, index) => {
      if (!sample.disconnected) return;
      const previous = plotSamples[index - 1];
      if (previous && previous.disconnected) return;
      let lastDisconnected = sample;
      let nextValid = null;
      for (let i = index + 1; i < plotSamples.length; i += 1) {
        if (!plotSamples[i].disconnected) {
          nextValid = plotSamples[i];
          break;
        }
        lastDisconnected = plotSamples[i];
      }
      const rawZoneStart = previous
        ? previous.timeMs + (sample.timeMs - previous.timeMs) / 2
        : sample.timeMs;
      const rawZoneEnd = nextValid
        ? lastDisconnected.timeMs + (nextValid.timeMs - lastDisconnected.timeMs) / 2
        : Math.min(range.endMs, lastDisconnected.timeMs + keepaliveMs);
      const zoneStart = clamp(rawZoneStart, range.startMs, range.endMs);
      const zoneEnd = clamp(
        rawZoneEnd,
        range.startMs,
        range.endMs
      );
      if (zoneEnd <= zoneStart) return;
      ctx.fillStyle = colors.disconnected;
      ctx.fillRect(xFor(zoneStart), pad.top, Math.max(1, xFor(zoneEnd) - xFor(zoneStart)), plotH);
    });

    ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
    ctx.lineWidth = 1 * scale;
    ctx.strokeStyle = colors.grid;
    ctx.fillStyle = colors.text;
    ctx.textBaseline = 'middle';

    const yTicks = compact ? 3 : 4;
    for (let i = 0; i <= yTicks; i += 1) {
      const y = pad.top + (plotH / yTicks) * i;
      const value = yMax - (span / yTicks) * i;
      ctx.beginPath();
      ctx.moveTo(pad.left, y);
      ctx.lineTo(width - pad.right, y);
      ctx.stroke();
      ctx.textAlign = 'right';
      ctx.fillText(value.toFixed(1) + ' °C', pad.left - 8 * scale, y);
    }

    ctx.textBaseline = 'top';
    const xTicks = compact ? 2 : 3;
    for (let i = 0; i <= xTicks; i += 1) {
      const x = pad.left + (plotW / xTicks) * i;
      const timeMs = range.startMs + (range.spanMs / xTicks) * i;
      ctx.beginPath();
      ctx.moveTo(x, pad.top);
      ctx.lineTo(x, pad.top + plotH);
      ctx.stroke();
      ctx.textAlign = i === 0 ? 'left' : (i === xTicks ? 'right' : 'center');
      ctx.fillText(formatTime(timeMs), x, pad.top + plotH + 8 * scale);
    }

    ctx.strokeStyle = colors.text;
    ctx.lineWidth = 1 * scale;
    ctx.beginPath();
    ctx.moveTo(pad.left, pad.top);
    ctx.lineTo(pad.left, pad.top + plotH);
    ctx.lineTo(pad.left + plotW, pad.top + plotH);
    ctx.stroke();

    ctx.fillStyle = colors.text;
    ctx.font = String((compact ? 11 : 12) * scale) + 'px system-ui, sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'bottom';
    ctx.fillText('Uptime', pad.left + plotW / 2, height - 4 * scale);
    if (!compact) {
      ctx.save();
      ctx.translate(13 * scale, pad.top + plotH / 2);
      ctx.rotate(-Math.PI / 2);
      ctx.textBaseline = 'top';
      ctx.fillText('Temperature, °C', 0, 0);
      ctx.restore();
    }

    const drawLine = (key, color, allowGaps) => {
      ctx.strokeStyle = color;
      ctx.lineWidth = 2 * scale;
      let drawing = false;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.beginPath();
      const lineSamples = plotSamples.filter((sample) => (
        Number.isFinite(sample[key]) && !(allowGaps && sample.disconnected)
      ));
      if (lineSamples.length === 1 && key === 'target') {
        const y = yFor(lineSamples[0][key]);
        ctx.moveTo(pad.left, y);
        ctx.lineTo(pad.left + plotW, y);
      }
      plotSamples.forEach((sample) => {
        const value = sample[key];
        if (!Number.isFinite(value) || (allowGaps && sample.disconnected)) {
          drawing = false;
          return;
        }
        const x = xFor(sample.timeMs);
        const y = yFor(value);
        if (!drawing) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
        drawing = true;
      });
      ctx.stroke();
      ctx.restore();
    };
    drawLine('target', colors.target, false);
    drawLine('temperature', colors.temperature, true);

    if (full) {
      ctx.textBaseline = 'middle';
      ctx.textAlign = 'left';
      ctx.font = '16px system-ui, sans-serif';
      ctx.fillStyle = colors.text;
      ctx.fillText('Temperature history', pad.left, 22);
      [
        ['Filtered temperature', colors.temperature],
        ['Target temperature', colors.target],
        ['Sensor unavailable', colors.disconnected]
      ].forEach((item, index) => {
        const x = pad.left + index * 230;
        const y = 42;
        ctx.fillStyle = item[1];
        ctx.fillRect(x, y - 2, 34, 4);
        ctx.fillStyle = colors.text;
        ctx.fillText(item[0], x + 44, y);
      });
    }

    if (!full && pointerState && pointerState.dragging && pointerState.mode === 'zoom') {
      ctx.fillStyle = colors.selection;
      const left = Math.max(pad.left, Math.min(pointerState.startX, pointerState.currentX));
      const right = Math.min(pad.left + plotW, Math.max(pointerState.startX, pointerState.currentX));
      ctx.fillRect(left, pad.top, right - left, plotH);
    }
    return { pad, plotW, plotH, range, xFor, yFor, visibleSamples };
  };
  const drawTemperatureChart = () => {
    const canvas = document.getElementById('temperatureChart');
    const empty = document.getElementById('chartEmpty');
    const legend = document.getElementById('chartLegend');
    const chartWindow = document.getElementById('chartWindow');
    const chartResetZoom = document.getElementById('chartResetZoom');
    if (!canvas || !empty || !legend) return;
    empty.classList.toggle('hidden', chartSamples.length > 0);
    renderChartLegend(legend);
    const geometry = drawChartInto(canvas, false);
    if (chartWindow) {
      const range = chartRange(false);
      chartWindow.textContent = zoomRange
        ? 'Zoom ' + formatTime(range.startMs) + ' to ' + formatTime(range.endMs)
        : 'Startup to ' + formatTime(latestChartTime());
    }
    if (chartResetZoom) {
      chartResetZoom.disabled = !zoomRange;
      chartResetZoom.textContent = zoomRange ? 'Show full range' : 'Full range';
      chartResetZoom.setAttribute('aria-disabled', zoomRange ? 'false' : 'true');
      chartResetZoom.title = zoomRange
        ? 'Return to the full temperature history'
        : 'The full temperature history is already visible';
    }
    return geometry;
  };
  const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
  const chartDisplayEnd = () => Math.max(minChartWindowMs, latestChartTime());
  const zoomConstraintEnd = (spanMs) => Math.max(Math.max(1000, spanMs), latestChartTime());
  const isChartRightEdge = (endMs, fullEnd, spanMs) => (
    Math.abs(endMs - fullEnd) <= Math.max(1500, spanMs * 0.002)
  );
  const pointerPoint = (canvas, event) => {
    const rect = canvas.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    const cssX = event.clientX - rect.left;
    const cssY = event.clientY - rect.top;
    return { cssX, cssY, x: cssX * dpr, y: cssY * dpr };
  };
  const inPlotArea = (geometry, point) => (
    point.x >= geometry.pad.left &&
    point.x <= geometry.pad.left + geometry.plotW &&
    point.y >= geometry.pad.top &&
    point.y <= geometry.pad.top + geometry.plotH
  );
  const clampToPlotX = (geometry, x) => (
    clamp(x, geometry.pad.left, geometry.pad.left + geometry.plotW)
  );
  const isPlotRightEdge = (geometry, x) => (
    Math.abs(x - (geometry.pad.left + geometry.plotW)) <=
      Math.max(8 * (window.devicePixelRatio || 1), geometry.plotW * 0.01)
  );
  const nearestSample = (canvas, event) => {
    const geometry = drawTemperatureChart();
    if (!geometry) return null;
    const point = pointerPoint(canvas, event);
    if (!inPlotArea(geometry, point)) return null;
    const candidates = geometry.visibleSamples.map((sample) => ({
      sample,
      distance: Math.abs(geometry.xFor(sample.timeMs) - point.x)
    })).sort((left, right) => left.distance - right.distance);
    if (!candidates.length) return null;
    return { sample: candidates[0].sample, ...point };
  };
  const showTooltip = (point) => {
    const tooltip = document.getElementById('chartTooltip');
    if (!tooltip || !point) {
      if (tooltip) tooltip.classList.add('hidden');
      return;
    }
    tooltip.innerHTML = [
      '<strong>' + formatTime(point.sample.timeMs) + '</strong>',
      '<span>Temperature: ' + formatTemperature(point.sample.temperature) + '</span>',
      '<span>Target: ' + formatTemperature(point.sample.target) + '</span>'
    ].join('');
    tooltip.style.left = Math.min(point.cssX + 12, tooltip.parentElement.clientWidth - 150) + 'px';
    tooltip.style.top = Math.max(8, point.cssY - 52) + 'px';
    tooltip.classList.remove('hidden');
  };
  const downloadCsvFromSamples = () => {
    const rows = [['timestamp_ms', 'filtered_temperature_c', 'target_temperature_c', 'sensor_status', 'sensor_valid', 'sensor_disconnected', 'flags']];
    chartSamples.forEach((sample) => {
      rows.push([
        Math.round(sample.timeMs),
        Number.isFinite(sample.temperature) ? sample.temperature.toFixed(1) : '',
        Number.isFinite(sample.target) ? sample.target.toFixed(1) : '',
        sample.status || (sample.disconnected ? 'unavailable' : 'ok'),
        sample.sensorValid === false || sample.disconnected ? '0' : '1',
        sample.disconnected ? '1' : '0',
        String(sample.flags || 0)
      ]);
    });
    const csv = rows.map((row) => row.map((cell) => '"' + String(cell).replace(/"/g, '""') + '"').join(',')).join('\n');
    const link = document.createElement('a');
    link.href = URL.createObjectURL(new Blob([csv], { type: 'text/csv' }));
    link.download = 'cooling-history.csv';
    link.click();
    URL.revokeObjectURL(link.href);
  };
  const downloadCsv = async () => {
    try {
      const res = await fetch('/api/history.csv', { cache: 'no-store' });
      if (!res.ok) throw new Error('csv unavailable');
      const blob = await res.blob();
      const link = document.createElement('a');
      link.href = URL.createObjectURL(blob);
      link.download = 'cooling-history.csv';
      link.click();
      URL.revokeObjectURL(link.href);
    } catch (error) {
      downloadCsvFromSamples();
    }
  };
  const downloadPng = () => {
    const canvas = document.createElement('canvas');
    drawChartInto(canvas, true);
    const link = document.createElement('a');
    link.href = canvas.toDataURL('image/png');
    link.download = 'cooling-chart.png';
    link.click();
  };
  const resetChartZoom = () => {
    if (!zoomRange) return;
    zoomRange = null;
    zoomFollowsRight = false;
    showTooltip(null);
    drawTemperatureChart();
  };
  const midpoint = (left, right) => ({
    x: (left.x + right.x) / 2,
    y: (left.y + right.y) / 2
  });
  const distance = (left, right) => Math.hypot(left.x - right.x, left.y - right.y);
  const zoomAroundPoint = (geometry, centerX, scaleFactor) => {
    if (!geometry || !Number.isFinite(scaleFactor) || scaleFactor <= 0) return false;
    const fullEnd = chartDisplayEnd();
    const range = chartRange(false);
    const currentSpan = Math.max(1000, range.endMs - range.startMs);
    const minSpan = Math.max(keepaliveMs, fullEnd / 80);
    const nextSpan = clamp(currentSpan / scaleFactor, minSpan, fullEnd);
    const ratio = clamp((centerX - geometry.pad.left) / geometry.plotW, 0, 1);
    const centerTime = range.startMs + ratio * currentSpan;
    const liveEnd = zoomConstraintEnd(nextSpan);
    const maxStartMs = Math.max(0, liveEnd - nextSpan);
    let startMs = clamp(centerTime - ratio * nextSpan, 0, maxStartMs);
    zoomFollowsRight = isChartRightEdge(startMs + nextSpan, liveEnd, nextSpan) ||
      (zoomFollowsRight && ratio > 0.96) ||
      isPlotRightEdge(geometry, centerX);
    if (zoomFollowsRight) startMs = maxStartMs;
    zoomRange = { startMs, endMs: startMs + nextSpan };
    return true;
  };
  const clearChartSamples = () => {
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    showTooltip(null);
    drawTemperatureChart();
  };
  const replaceChartSamples = (samples) => {
    chartSamples.splice(0, chartSamples.length);
    lastChartUptimeMs = 0;
    lastChartSampleKey = '';
    zoomRange = null;
    zoomFollowsRight = false;
    (samples || []).forEach(pushChartSample);
    drawTemperatureChart();
  };
  const attachChartInteractions = (chartCanvas) => {
    if (!chartCanvas || chartCanvas.dataset.chartInteractions === '1') return;
    chartCanvas.dataset.chartInteractions = '1';
    const activePointers = new Map();
    let pinchState = null;
    const stopPointerDrag = () => {
      pointerState = null;
      pinchState = null;
      showTooltip(null);
      drawTemperatureChart();
    };
    chartCanvas.addEventListener('pointerdown', (event) => {
      activePointers.set(event.pointerId, pointerPoint(chartCanvas, event));
      if (activePointers.size === 2) {
        const points = Array.from(activePointers.values());
        const geometry = drawTemperatureChart();
        const center = midpoint(points[0], points[1]);
        pointerState = null;
        pinchState = geometry && inPlotArea(geometry, center)
          ? {
              distance: distance(points[0], points[1])
            }
          : null;
        chartCanvas.setPointerCapture(event.pointerId);
        showTooltip(null);
        return;
      }
      const geometry = drawTemperatureChart();
      const point = pointerPoint(chartCanvas, event);
      if (!geometry || !inPlotArea(geometry, point)) {
        showTooltip(null);
        return;
      }
      const x = clampToPlotX(geometry, point.x);
      const activeRange = chartRange(false);
      pointerState = {
        mode: isCoarsePointer() ? 'inspect' : (zoomRange ? 'pan' : 'zoom'),
        startX: x,
        currentX: x,
        startRange: { startMs: activeRange.startMs, endMs: activeRange.endMs },
        dragging: false
      };
      chartCanvas.setPointerCapture(event.pointerId);
      showTooltip(null);
    });
    chartCanvas.addEventListener('pointermove', (event) => {
      if (activePointers.has(event.pointerId)) {
        activePointers.set(event.pointerId, pointerPoint(chartCanvas, event));
      }
      if (pinchState && activePointers.size >= 2) {
        const points = Array.from(activePointers.values()).slice(0, 2);
        const geometry = drawTemperatureChart();
        const nextDistance = distance(points[0], points[1]);
        const center = midpoint(points[0], points[1]);
        if (geometry && nextDistance > 0) {
          zoomAroundPoint(geometry, clampToPlotX(geometry, center.x), nextDistance / pinchState.distance);
          pinchState = { distance: nextDistance };
        }
        showTooltip(null);
        drawTemperatureChart();
        return;
      }
      if (pointerState) {
        const geometry = drawTemperatureChart();
        const point = pointerPoint(chartCanvas, event);
        const dpr = window.devicePixelRatio || 1;
        pointerState.currentX = geometry ? clampToPlotX(geometry, point.x) : point.x;
        pointerState.dragging = Math.abs(pointerState.currentX - pointerState.startX) > 8 * dpr;
        if (geometry && pointerState.dragging && (pointerState.mode === 'pan' || (pointerState.mode === 'inspect' && zoomRange))) {
          const spanMs = Math.max(1000, pointerState.startRange.endMs - pointerState.startRange.startMs);
          const deltaMs = ((pointerState.startX - pointerState.currentX) / geometry.plotW) * spanMs;
          zoomRange = {
            startMs: pointerState.startRange.startMs,
            endMs: pointerState.startRange.endMs
          };
          panZoomRange(deltaMs);
        }
      }
      if (pointerState && pointerState.dragging) showTooltip(null);
      else showTooltip(nearestSample(chartCanvas, event));
      drawTemperatureChart();
    });
    chartCanvas.addEventListener('wheel', (event) => {
      if (!zoomRange) return;
      const geometry = drawTemperatureChart();
      if (!geometry) return;
      const point = pointerPoint(chartCanvas, event);
      if (!inPlotArea(geometry, point)) return;
      event.preventDefault();
      const wheelDelta = Math.abs(event.deltaX) > Math.abs(event.deltaY)
        ? event.deltaX
        : event.deltaY;
      const deltaMs = (wheelDelta / geometry.plotW) * geometry.range.spanMs;
      if (panZoomRange(deltaMs)) {
        showTooltip(null);
        drawTemperatureChart();
      }
    }, { passive: false });
    chartCanvas.addEventListener('pointerup', (event) => {
      activePointers.delete(event.pointerId);
      if (pinchState) {
        if (chartCanvas.hasPointerCapture && chartCanvas.hasPointerCapture(event.pointerId)) {
          chartCanvas.releasePointerCapture(event.pointerId);
        }
        pinchState = null;
        pointerState = null;
        showTooltip(null);
        drawTemperatureChart();
        return;
      }
      if (pointerState && pointerState.dragging && pointerState.mode === 'zoom') {
        const geometry = drawTemperatureChart();
        if (geometry) {
          pointerState.currentX = clampToPlotX(geometry, pointerPoint(chartCanvas, event).x);
          const left = Math.max(geometry.pad.left, Math.min(pointerState.startX, pointerState.currentX));
          const right = Math.min(geometry.pad.left + geometry.plotW, Math.max(pointerState.startX, pointerState.currentX));
          if (right - left > 12) {
            const startMs = geometry.range.startMs +
              ((left - geometry.pad.left) / geometry.plotW) * geometry.range.spanMs;
            const endMs = geometry.range.startMs +
              ((right - geometry.pad.left) / geometry.plotW) * geometry.range.spanMs;
            zoomRange = { startMs, endMs };
            const spanMs = Math.max(1000, endMs - startMs);
            zoomFollowsRight = isPlotRightEdge(geometry, right) ||
              isChartRightEdge(endMs, zoomConstraintEnd(spanMs), spanMs);
          }
        }
      } else if (pointerState && !pointerState.dragging) {
        showTooltip(nearestSample(chartCanvas, event));
      }
      if (chartCanvas.hasPointerCapture && chartCanvas.hasPointerCapture(event.pointerId)) {
        chartCanvas.releasePointerCapture(event.pointerId);
      }
      pointerState = null;
      drawTemperatureChart();
    });
    chartCanvas.addEventListener('pointercancel', (event) => {
      activePointers.delete(event.pointerId);
      stopPointerDrag();
    });
    chartCanvas.addEventListener('lostpointercapture', () => {
      if (pointerState) stopPointerDrag();
    });
    chartCanvas.addEventListener('pointerleave', () => {
      if (!pointerState) showTooltip(null);
    });
    chartCanvas.addEventListener('contextlost', (event) => {
      event.preventDefault();
      showTooltip(null);
    });
    chartCanvas.addEventListener('contextrestored', () => {
      drawTemperatureChart();
    });
  };
  window.CoolingChart = {
    addStatusSample: addChartSample,
    addSample: pushChartSample,
    applyHistory,
    clear: clearChartSamples,
    replaceSamples: replaceChartSamples,
    draw: drawTemperatureChart,
    resetZoom: resetChartZoom,
    attachInteractions: attachChartInteractions,
    downloadPng,
    downloadCsv: downloadCsvFromSamples,
    samples: chartSamples
  };
  const demoElapsedMs = () => (
    demoChartWarmupMs + Math.floor((Date.now() - startedAt) * demoTimeScale)
  );
  const fallbackStatus = (timeMs = demoElapsedMs()) => {
    const targetC = 5.0 +
      Math.sin(timeMs / 90000) * 0.65 +
      Math.sin(timeMs / 31000) * 0.18;
    const rawTemperatureC = targetC +
      Math.sin(timeMs / 42000 + 1.1) * 0.78 +
      Math.sin(timeMs / 11500) * 0.14;
    const filteredTemperatureC = targetC +
      Math.sin((timeMs - 1800) / 42000 + 1.1) * 0.64 +
      Math.sin(timeMs / 16500) * 0.08;
    const disconnectPhaseMs = timeMs % 95000;
    const disconnected = disconnectPhaseMs > 61000 && disconnectPhaseMs < 69000;
    const peltierRunning = !disconnected && filteredTemperatureC > targetC + 0.08;
    const fanRunOnActive = !peltierRunning && (timeMs % 18000) < 6500;
    return {
      temperatureC: disconnected ? null : rawTemperatureC,
      filteredTemperatureC: disconnected ? null : filteredTemperatureC,
      hasTemperature: !disconnected,
      sensorDisconnected: disconnected,
      updateCount: Math.floor(timeMs / demoSampleIntervalMs),
      peltierRunning,
      fanRunning: peltierRunning || fanRunOnActive,
      fanRunOnActive,
      fanRunOnRemainingMs: fanRunOnActive ? 6500 - (timeMs % 18000) : 0,
      targetC,
      hysteresisC: 0.1,
      measurementIntervalMs: demoSampleIntervalMs,
      fanRunOnMs: 30000,
      uptimeMs: timeMs,
      accessPointSsid: 'CoolingController',
      accessPointIp: '',
      stationSsid: '',
      stationPasswordSet: false,
      stationConnected: false,
      stationStatus: 'disabled',
      stationIp: '',
      stationRssi: 0,
      demo: true
    };
  };
  const syncDemoChart = (latestStatus) => {
    if (!latestStatus.demo || hasRealStatus) return;
    const latestMs = sampleTimeMs(latestStatus);
    const startMs = demoChartSeeded || chartSamples.length
      ? lastChartUptimeMs + demoSampleIntervalMs
      : Math.max(0, latestMs - demoChartWarmupMs);
    for (let timeMs = startMs; timeMs <= latestMs; timeMs += demoSampleIntervalMs) {
      addChartSample(fallbackStatus(timeMs));
    }
    addChartSample(latestStatus);
    demoChartSeeded = true;
  };
  const pageUrl = (ip) => ip ? 'http://' + ip + '/' : '--';
  const signalQuality = (rssi) => {
    if (rssi >= -55) return 'Excellent';
    if (rssi >= -67) return 'Good';
    if (rssi >= -75) return 'Fair';
    return 'Weak';
  };
  const signalLevel = (rssi) => {
    const value = Number(rssi);
    if (value >= -60) return 3;
    if (value >= -72) return 2;
    return 1;
  };

  document.addEventListener('DOMContentLoaded', () => {
    const demoBadge = document.getElementById('demoBadge');
    const settingsForm = document.getElementById('settingsForm');
    const settingsOverlay = document.getElementById('settingsOverlay');
    const settingsOpen = document.getElementById('settingsOpen');
    const settingsClose = document.getElementById('settingsClose');
    const scanNetworks = document.getElementById('scanNetworks');
    const networkList = document.getElementById('networkList');
    const networkScanState = document.getElementById('networkScanState');
    const saveState = document.getElementById('saveState');
    const stationSsidInput = document.getElementById('stationSsidInput');
    const stationPasswordInput = document.getElementById('stationPasswordInput');
    const toggleStationPassword = document.getElementById('toggleStationPassword');
    const hiddenNetworkInput = document.getElementById('hiddenNetworkInput');
    const forgetStationNetworkInput = document.getElementById('forgetStationNetworkInput');
    const forgetStationNetwork = document.getElementById('forgetStationNetwork');
    const chartCanvas = document.getElementById('temperatureChart');
    const chartResetZoom = document.getElementById('chartResetZoom');
    const chartDownloadPng = document.getElementById('chartDownloadPng');
    const chartDownloadCsv = document.getElementById('chartDownloadCsv');
    let settingsDirty = false;
    let networkScanStartedAt = 0;
    let currentSavedStationSsid = '';
    let selectedNetworkSsid = '';
    let visibleNetworkSsids = new Set();
    const setHiddenNetworkMode = (enabled, keepValue) => {
      hiddenNetworkInput.checked = enabled;
      stationSsidInput.readOnly = !enabled;
      stationSsidInput.placeholder = enabled ? 'Enter hidden network SSID' : 'Select from nearby networks';
      if (enabled) {
        selectedNetworkSsid = '';
        if (!keepValue) stationSsidInput.value = '';
      }
      if (!enabled && selectedNetworkSsid) {
        stationSsidInput.value = selectedNetworkSsid;
      }
    };
    const applySettingsToForm = (data) => {
      if (settingsDirty) return;
      document.getElementById('targetInput').value = Number(data.targetC).toFixed(1);
      document.getElementById('hysteresisInput').value = Number(data.hysteresisC).toFixed(1);
      document.getElementById('measurementInput').value = data.measurementIntervalMs;
      document.getElementById('fanRunOnInput').value = data.fanRunOnMs;
      currentSavedStationSsid = data.stationSsid || '';
      selectedNetworkSsid = currentSavedStationSsid;
      stationSsidInput.value = currentSavedStationSsid;
      stationPasswordInput.value = '';
      stationPasswordInput.type = 'password';
      toggleStationPassword.textContent = 'Show';
      toggleStationPassword.setAttribute('aria-pressed', 'false');
      stationPasswordInput.placeholder = data.stationPasswordSet ? 'Saved; leave blank to keep' : 'Leave blank for open network';
      forgetStationNetworkInput.value = '0';
      setHiddenNetworkMode(false, true);
    };
    const renderNetworkStatus = (data) => {
      setText('apStatus', data.accessPointSsid || '--');
      setText('apPageStatus', pageUrl(data.accessPointIp));
      setText('stationStatus', data.stationSsid ? data.stationSsid : (data.stationLastFailure ? 'Failed' : 'Not connected'));
      setText('stationPageStatus', data.stationConnected ? pageUrl(data.stationIp) : (data.stationSsid ? (data.stationStatus || 'disconnected') : 'Wi-Fi off'));
      setText('currentPageStatus', window.location.origin || '--');
    };
    const renderNetworks = (networks) => {
      networkList.textContent = '';
      visibleNetworkSsids = new Set();
      if (!networks.length) {
        networkScanState.textContent = 'No 2.4 GHz networks found';
        return;
      }
      networkScanState.textContent = networks.length + ' found';
      networks.forEach((network) => {
        if (network.ssid) visibleNetworkSsids.add(network.ssid);
        const item = document.createElement('button');
        item.type = 'button';
        item.className = 'network-item';
        const selected = Boolean(network.ssid) && network.ssid === stationSsidInput.value && !hiddenNetworkInput.checked;
        item.classList.toggle('selected', selected);
        item.addEventListener('click', () => {
          if (network.ssid) {
            selectedNetworkSsid = network.ssid;
            forgetStationNetworkInput.value = '0';
            setHiddenNetworkMode(false, false);
          } else {
            forgetStationNetworkInput.value = '0';
            setHiddenNetworkMode(true, false);
            stationSsidInput.focus();
          }
          renderNetworks(networks);
          settingsDirty = true;
          saveState.textContent = '';
        });

        const icon = document.createElement('span');
        icon.className = 'network-icon signal-' + signalLevel(network.rssi);
        icon.setAttribute('aria-hidden', 'true');
        icon.append(document.createElement('i'), document.createElement('i'), document.createElement('i'));

        const main = document.createElement('span');
        main.className = 'network-main';
        const ssid = document.createElement('strong');
        ssid.textContent = network.ssid || '(hidden network)';
        const detail = document.createElement('span');
        detail.textContent = [
          signalQuality(Number(network.rssi)),
          Number(network.rssi) + ' dBm',
          'ch ' + network.channel,
          network.encrypted ? 'secured' : 'open'
        ].join(' / ');
        main.append(ssid, detail);

        const trailing = document.createElement('span');
        trailing.className = 'network-trailing';
        trailing.textContent = selected ? 'Selected' : (network.encrypted ? 'Secured' : '');
        if (network.encrypted && !selected) {
          const lock = document.createElement('span');
          lock.className = 'lock-dot';
          trailing.append(lock);
        }
        item.append(icon, main, trailing);
        networkList.append(item);
      });
    };
    const loadNetworks = async (polling) => {
      if (!polling) {
        networkScanStartedAt = Date.now();
      }
      scanNetworks.disabled = true;
      networkScanState.textContent = polling ? 'Still scanning...' : 'Scanning...';
      try {
        const res = await fetch('/api/networks', { cache: 'no-store' });
        if (!res.ok) throw new Error('scan unavailable');
        const data = await res.json();
        if (data.scanStatus === 'scanning') {
          if (Date.now() - networkScanStartedAt > 15000) {
            networkList.textContent = '';
            networkScanState.textContent = 'Scan timeout';
            return;
          }
          window.setTimeout(() => loadNetworks(true), 600);
          return;
        }
        if (!data.ok) {
          networkList.textContent = '';
          networkScanState.textContent = 'Scan ' + (data.scanStatus || 'failed');
          return;
        }
        renderNetworks(Array.isArray(data.networks) ? data.networks : []);
      } catch (error) {
        renderNetworks([
          { ssid: 'Kitchen WiFi', rssi: -48, channel: 6, encrypted: true },
          { ssid: 'Workshop', rssi: -66, channel: 11, encrypted: true },
          { ssid: 'Guest', rssi: -78, channel: 1, encrypted: false }
        ]);
        networkScanState.textContent = 'Demo scan';
      } finally {
        if (networkScanState.textContent !== 'Still scanning...' && networkScanState.textContent !== 'Scanning...') {
          scanNetworks.disabled = false;
        }
      }
    };
    const openSettings = () => {
      settingsOverlay.classList.add('open');
      settingsOpen.setAttribute('aria-expanded', 'true');
      document.getElementById('targetInput').focus();
    };
    const closeSettings = () => {
      settingsOverlay.classList.remove('open');
      settingsOpen.setAttribute('aria-expanded', 'false');
      settingsOpen.focus();
    };
    settingsOpen.setAttribute('aria-expanded', 'false');
    settingsOpen.addEventListener('click', openSettings);
    settingsClose.addEventListener('click', closeSettings);
    scanNetworks.addEventListener('click', () => loadNetworks(false));
    chartResetZoom.disabled = true;
    chartResetZoom.textContent = 'Full range';
    chartResetZoom.addEventListener('click', (event) => {
      resetChartZoom();
      event.currentTarget.blur();
    });
    chartDownloadPng.addEventListener('click', downloadPng);
    chartDownloadCsv.addEventListener('click', downloadCsv);
    attachChartInteractions(chartCanvas);
    toggleStationPassword.addEventListener('click', () => {
      const showing = stationPasswordInput.type === 'text';
      stationPasswordInput.type = showing ? 'password' : 'text';
      toggleStationPassword.textContent = showing ? 'Show' : 'Hide';
      toggleStationPassword.setAttribute('aria-pressed', showing ? 'false' : 'true');
      stationPasswordInput.focus();
    });
    hiddenNetworkInput.addEventListener('change', () => {
      forgetStationNetworkInput.value = '0';
      setHiddenNetworkMode(hiddenNetworkInput.checked, true);
      settingsDirty = true;
      saveState.textContent = '';
      if (hiddenNetworkInput.checked) stationSsidInput.focus();
    });
    forgetStationNetwork.addEventListener('click', () => {
      selectedNetworkSsid = '';
      stationSsidInput.value = '';
      stationPasswordInput.value = '';
      forgetStationNetworkInput.value = '1';
      setHiddenNetworkMode(false, true);
      settingsDirty = true;
      saveState.textContent = 'Network will be forgotten on save';
    });
    settingsOverlay.addEventListener('click', (event) => {
      if (event.target === settingsOverlay) closeSettings();
    });
    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && settingsOverlay.classList.contains('open')) closeSettings();
    });
    async function loadStatus() {
      try {
        const res = await fetchWithTimeout('/api/status', { cache: 'no-store' }, statusFetchTimeoutMs);
        if (!res.ok) throw new Error('status unavailable');
        const data = await res.json();
        data.demo = false;
        return data;
      } catch (error) {
        return fallbackStatus();
      }
    }
    async function refresh() {
      const data = await loadStatus();
      if (data.demo) syncDemoChart(data);
      else addChartSample(data);
      demoBadge.classList.toggle('hidden', !data.demo);
      const filteredTemperature = Number(data.filteredTemperatureC ?? data.temperatureC);
      setText('temperature', data.hasTemperature && !data.sensorDisconnected ? formatTemperature(filteredTemperature) : '-- °C');
      const sensor = document.getElementById('sensor');
      sensor.textContent = data.sensorDisconnected || !data.hasTemperature ? 'UNAVAILABLE' : 'OK';
      sensor.className = 'value small ' + (data.sensorDisconnected || !data.hasTemperature ? 'bad' : 'ok');
      setState('peltier', data.peltierRunning);
      setState('fan', data.fanRunning);
      setText('runon', data.fanRunOnActive ? Math.ceil(data.fanRunOnRemainingMs / 1000) + ' s' : 'OFF');
      setText('count', data.updateCount);
      setText('target', formatTemperature(data.targetC) + ' / hys ' + data.hysteresisC.toFixed(1) + ' °C');
      setText('uptime', Math.floor(data.uptimeMs / 1000) + ' s');
      setText('wifi', data.stationSsid ? (data.stationConnected ? data.stationIp : (data.stationStatus || 'Disconnected')) : (data.stationLastFailure ? 'Failed: ' + data.stationLastFailure : 'Disabled'));
      renderNetworkStatus(data);
      applySettingsToForm(data);
      drawTemperatureChart();
    }
    settingsForm.addEventListener('input', () => {
      settingsDirty = true;
      saveState.textContent = '';
    });
    settingsForm.addEventListener('submit', async (event) => {
      event.preventDefault();
      stationSsidInput.value = stationSsidInput.value.trim();
      const stationSsidChanged = stationSsidInput.value !== currentSavedStationSsid;
      if (!hiddenNetworkInput.checked &&
          stationSsidChanged &&
          stationSsidInput.value.length > 0 &&
          !visibleNetworkSsids.has(stationSsidInput.value)) {
        saveState.textContent = 'Scan and select a network first';
        return;
      }
      if (hiddenNetworkInput.checked && stationSsidInput.value.length === 0) {
        saveState.textContent = 'Enter hidden network SSID';
        stationSsidInput.focus();
        return;
      }
      saveState.textContent = 'Saving...';
      try {
        const body = new URLSearchParams(new FormData(settingsForm));
        const res = await fetch('/api/settings', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body
        });
        if (!res.ok) throw new Error('save failed');
        const data = await res.json();
        settingsDirty = false;
        applySettingsToForm(data);
        saveState.textContent = 'Saved';
        await refresh();
      } catch (error) {
        saveState.textContent = 'Save failed';
      }
    });
    window.addEventListener('resize', drawTemperatureChart);
    document.getElementById('themeToggle').addEventListener('click', () => {
      window.setTimeout(drawTemperatureChart, 0);
    });
    loadHistory();
    refresh();
    setInterval(refresh, 1000);
  });
}());
