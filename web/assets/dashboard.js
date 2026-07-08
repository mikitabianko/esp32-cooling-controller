(function () {
  const { setText } = window.CoolingWeb;
  const startedAt = Date.now();
  const minChartWindowMs = 5 * 60 * 1000;
  const keepaliveMs = 30 * 1000;
  const disconnectedFlag = 1;
  const statusFetchTimeoutMs = 900;
  const historyFetchTimeoutMs = 1200;
  const predictionStorageKey = 'cooling-chart-prediction';
  const predictionMaxHorizonMs = 6 * 60 * 60 * 1000;
  const predictionLookbackMs = 20 * 60 * 1000;
  const predictionMaxSampleGapMs = 3 * 60 * 1000;
  const predictionMinSpanMs = 90 * 1000;
  const predictionMinSamples = 4;
  const predictionTargetToleranceC = 0.05;
  const predictionMaxTargetShiftC = 0.25;
  const predictionMinClosingSlopeCPerSecond = 0.000015;
  const chartSamples = [];
  let lastChartUptimeMs = 0;
  let lastChartSampleKey = '';
  let zoomRange = null;
  let zoomFollowsRight = false;
  let pointerState = null;
  let predictionEnabled = false;
  const loadPredictionPreference = () => {
    try {
      return window.localStorage && window.localStorage.getItem(predictionStorageKey) === '1';
    } catch (error) {
      return false;
    }
  };
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
    hysteresis: numericOrNull(sample.hysteresis),
    peltierRunning: Boolean(sample.peltierRunning),
    fanRunning: Boolean(sample.fanRunning),
    fanRunOnActive: Boolean(sample.fanRunOnActive),
    disconnected: Boolean(sample.disconnected)
  });
  const addChartSample = (data) => {
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
    const hysteresis = Number(data.hysteresisC);
    const sampleKey = [
      data.updateCount,
      disconnected ? 'x' : temperature.toFixed(2),
      Number.isFinite(target) ? target.toFixed(2) : '',
      Number.isFinite(hysteresis) ? hysteresis.toFixed(2) : '',
      data.peltierRunning ? 'p1' : 'p0',
      data.fanRunning ? 'f1' : 'f0',
      data.fanRunOnActive ? 'r1' : 'r0'
    ].join(':');
    if (lastChartSampleKey === sampleKey) return;
    lastChartSampleKey = sampleKey;
    pushChartSample(normalizeChartSample({
      timeMs,
      temperature: Number.isFinite(temperature) ? temperature : null,
      target: Number.isFinite(target) ? target : null,
      hysteresis: Number.isFinite(hysteresis) ? hysteresis : null,
      peltierRunning: Boolean(data.peltierRunning),
      fanRunning: Boolean(data.fanRunning),
      fanRunOnActive: Boolean(data.fanRunOnActive),
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
    const historyHysteresis = numericOrNull(history.hysteresisC);
    const peltierFlag = Number(history.peltierFlag) || 2;
    const fanFlag = Number(history.fanFlag) || 4;
    const fanRunOnFlag = Number(history.fanRunOnFlag) || 8;
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
          hysteresis: point.length > 6 ? numericOrNull(Number(point[6]) / 10) : historyHysteresis,
          peltierRunning: point.length > 7 ? Number(point[7]) === 1 : (flags & peltierFlag) !== 0,
          fanRunning: point.length > 8 ? Number(point[8]) === 1 : (flags & fanFlag) !== 0,
          fanRunOnActive: point.length > 9 ? Number(point[9]) === 1 : (flags & fanRunOnFlag) !== 0,
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
    const predictionColor = document.documentElement.dataset.theme === 'dark'
      ? '#ff8df2'
      : '#e044d0';
    return {
      text: style.getPropertyValue('--muted').trim(),
      grid: style.getPropertyValue('--chart-grid').trim(),
      temperature: style.getPropertyValue('--chart-primary').trim(),
      target: style.getPropertyValue('--chart-target').trim(),
      hysteresis: style.getPropertyValue('--chart-hysteresis').trim(),
      current: style.getPropertyValue('--chart-current').trim(),
      prediction: predictionColor,
      peltier: style.getPropertyValue('--chart-peltier').trim(),
      fan: style.getPropertyValue('--chart-fan').trim(),
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
      { label: 'Hysteresis band', color: '--chart-hysteresis' },
      { label: 'Peltier', color: '--chart-peltier' },
      { label: 'Fan', color: '--chart-fan' },
      ...(predictionEnabled ? [{ label: 'Prediction', value: chartColors().prediction }] : []),
      { label: 'Sensor unavailable', color: '--chart-disconnected' }
    ].forEach((series) => {
      const item = document.createElement('span');
      item.className = 'legend-item';
      item.style.color = series.value ||
        getComputedStyle(document.documentElement).getPropertyValue(series.color).trim();
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
  const lastValidTemperatureSample = () => (
    chartSamples.slice().reverse().find((sample) => (
      Number.isFinite(sample.temperature) &&
      Number.isFinite(sample.target) &&
      !sample.disconnected
    )) || null
  );
  const weightedLinearFit = (points) => {
    if (!points || points.length < 2) return null;
    let sumW = 0;
    let sumX = 0;
    let sumY = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      sumW += weight;
      sumX += point.x * weight;
      sumY += point.y * weight;
    });
    if (sumW <= 0) return null;
    const meanX = sumX / sumW;
    const meanY = sumY / sumW;
    let sxx = 0;
    let sxy = 0;
    let syy = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      const dx = point.x - meanX;
      const dy = point.y - meanY;
      sxx += weight * dx * dx;
      sxy += weight * dx * dy;
      syy += weight * dy * dy;
    });
    if (sxx <= 0) return null;
    const slope = sxy / sxx;
    const intercept = meanY - slope * meanX;
    let residualSum = 0;
    points.forEach((point) => {
      const weight = Number.isFinite(point.weight) && point.weight > 0 ? point.weight : 1;
      const residual = point.y - (intercept + slope * point.x);
      residualSum += weight * residual * residual;
    });
    const r2 = syy > 0 ? Math.max(0, Math.min(1, (sxy * sxy) / (sxx * syy))) : 0;
    return {
      slope,
      intercept,
      r2,
      rmse: Math.sqrt(residualSum / sumW)
    };
  };
  const collectPredictionSamples = (latest) => {
    const samples = [];
    const startMs = latest.timeMs - predictionLookbackMs;
    const targetBandC = Math.max(
      predictionMaxTargetShiftC,
      Number.isFinite(latest.hysteresis) ? Math.abs(latest.hysteresis) * 2 : 0
    );
    let previousValid = null;
    for (let i = chartSamples.length - 1; i >= 0; i -= 1) {
      const sample = chartSamples[i];
      if (sample.timeMs > latest.timeMs) continue;
      if (sample.timeMs < startMs) break;
      const valid = Number.isFinite(sample.timeMs) &&
        Number.isFinite(sample.temperature) &&
        Number.isFinite(sample.target) &&
        !sample.disconnected;
      if (!valid) {
        if (samples.length && sample.disconnected) break;
        continue;
      }
      if (Math.abs(sample.target - latest.target) > targetBandC) {
        if (samples.length) break;
        continue;
      }
      if (previousValid && previousValid.timeMs - sample.timeMs > predictionMaxSampleGapMs) break;
      samples.push(sample);
      previousValid = sample;
    }
    return samples.reverse();
  };
  const predictionTrendStats = (samples, direction) => {
    let closingWeight = 0;
    let totalWeight = 0;
    let totalClosingC = 0;
    let totalSeconds = 0;
    for (let i = 1; i < samples.length; i += 1) {
      const previous = samples[i - 1];
      const sample = samples[i];
      const dt = (sample.timeMs - previous.timeMs) / 1000;
      if (dt <= 0) continue;
      const previousDistance = direction * (previous.temperature - previous.target);
      const distance = direction * (sample.temperature - sample.target);
      if (!Number.isFinite(previousDistance) || !Number.isFinite(distance)) continue;
      const closingC = previousDistance - distance;
      const weight = Math.min(dt, 120);
      totalWeight += weight;
      totalSeconds += dt;
      totalClosingC += closingC;
      if (closingC >= -predictionTargetToleranceC * 0.3) closingWeight += weight;
    }
    return {
      ratio: totalWeight > 0 ? closingWeight / totalWeight : 0,
      netSlope: totalSeconds > 0 ? totalClosingC / totalSeconds : 0
    };
  };
  const buildTemperaturePrediction = () => {
    if (!predictionEnabled) return null;
    const latest = lastValidTemperatureSample();
    if (!latest || latestChartTime() - latest.timeMs > keepaliveMs * 2) return null;
    const latestErrorC = latest.temperature - latest.target;
    const latestDistanceC = Math.abs(latestErrorC);
    if (!Number.isFinite(latestDistanceC) || latestDistanceC <= predictionTargetToleranceC) return null;
    const direction = latestErrorC >= 0 ? 1 : -1;
    const fitSamples = collectPredictionSamples(latest)
      .map((sample) => {
        const errorC = sample.temperature - sample.target;
        const distanceC = direction * errorC;
        return { ...sample, errorC, distanceC };
      })
      .filter((sample) => sample.distanceC > predictionTargetToleranceC * 0.5);
    if (fitSamples.length < predictionMinSamples) return null;
    const spanMs = fitSamples[fitSamples.length - 1].timeMs - fitSamples[0].timeMs;
    if (spanMs < predictionMinSpanMs) return null;
    const stats = predictionTrendStats(fitSamples, direction);
    if (stats.ratio < 0.45 && stats.netSlope <= predictionMinClosingSlopeCPerSecond) return null;
    const durationMs = Math.max(1, spanMs);
    const weightedPoints = fitSamples.map((sample) => {
      const recency = (sample.timeMs - fitSamples[0].timeMs) / durationMs;
      const weight = 0.35 + 0.65 * recency * recency;
      return {
        x: (sample.timeMs - latest.timeMs) / 1000,
        errorC: sample.errorC,
        distanceC: sample.distanceC,
        weight
      };
    });
    const linearFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x,
      y: point.errorC,
      weight: point.weight
    })));
    const expFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x,
      y: Math.log(point.distanceC),
      weight: point.weight
    })));
    const maxEtaSeconds = predictionMaxHorizonMs / 1000;
    const linearClosingSlope = linearFit ? -direction * linearFit.slope : 0;
    const linearEtaSeconds = linearClosingSlope > predictionMinClosingSlopeCPerSecond
      ? (latestDistanceC - predictionTargetToleranceC) / linearClosingSlope
      : NaN;
    const expK = expFit ? -expFit.slope : 0;
    const expEtaSeconds = expK > 0
      ? Math.log(latestDistanceC / predictionTargetToleranceC) / expK
      : NaN;
    const linearValid = Number.isFinite(linearEtaSeconds) &&
      linearEtaSeconds >= 5 &&
      linearEtaSeconds <= maxEtaSeconds &&
      linearClosingSlope > predictionMinClosingSlopeCPerSecond;
    const expValid = Number.isFinite(expEtaSeconds) &&
      expEtaSeconds >= 5 &&
      expEtaSeconds <= maxEtaSeconds &&
      expK > 0;
    if (!linearValid && !expValid) return null;
    const linearScore = linearFit
      ? linearFit.r2 - Math.min(1, linearFit.rmse / Math.max(latestDistanceC, 0.1)) * 0.25 + stats.ratio * 0.2
      : -Infinity;
    const expScore = expFit
      ? expFit.r2 - Math.min(1, expFit.rmse) * 0.2 + stats.ratio * 0.2 + (latestDistanceC > 0.3 ? 0.05 : 0)
      : -Infinity;
    const useExp = expValid && (!linearValid || expScore >= linearScore - 0.05);
    const etaSeconds = useExp ? expEtaSeconds : linearEtaSeconds;
    const etaMs = etaSeconds * 1000;
    const endMs = latest.timeMs + etaMs;
    const points = [{ timeMs: latest.timeMs, temperature: latest.temperature }];
    const stepMs = Math.max(15000, Math.min(120000, etaMs / 24));
    for (let timeMs = latest.timeMs + stepMs; timeMs < endMs; timeMs += stepMs) {
      const elapsedSeconds = (timeMs - latest.timeMs) / 1000;
      const distanceC = useExp
        ? latestDistanceC * Math.exp(-expK * elapsedSeconds)
        : Math.max(predictionTargetToleranceC, latestDistanceC - linearClosingSlope * elapsedSeconds);
      points.push({
        timeMs,
        temperature: latest.target + direction * Math.max(predictionTargetToleranceC, distanceC)
      });
    }
    points.push({
      timeMs: endMs,
      temperature: latest.target + direction * predictionTargetToleranceC
    });
    return {
      points,
      etaMs: endMs,
      target: latest.target,
      model: useExp ? 'exponential' : 'linear',
      confidence: Math.max(0, Math.min(1, useExp ? expScore : linearScore))
    };
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
    const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
    const predictionSamples = prediction && prediction.points
      ? prediction.points.filter((sample) => sample.timeMs >= range.startMs && sample.timeMs <= range.endMs)
      : [];
    const currentSample = chartSamples.slice().reverse().find((sample) => (
      Number.isFinite(sample.temperature) && !sample.disconnected
    ));
    const values = plotSamples
      .flatMap((sample) => [
        sample.temperature,
        sample.target,
        Number.isFinite(sample.target) && Number.isFinite(sample.hysteresis)
          ? sample.target + sample.hysteresis
          : null
      ].filter(Number.isFinite))
      .concat(predictionSamples.map((sample) => sample.temperature).filter(Number.isFinite))
      .concat(currentSample ? [currentSample.temperature] : []);
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
    const stateLaneH = Math.min((compact ? 18 : 22) * scale, Math.max(0, plotH * 0.18));
    const stateInset = 2 * scale;
    const axisY = pad.top + plotH;
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
      ctx.fillText(formatTime(timeMs), x, axisY + 8 * scale);
    }

    ctx.strokeStyle = colors.text;
    ctx.lineWidth = 1 * scale;
    ctx.beginPath();
    ctx.moveTo(pad.left, pad.top);
    ctx.lineTo(pad.left, pad.top + plotH);
    ctx.moveTo(pad.left, axisY);
    ctx.lineTo(pad.left + plotW, axisY);
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

    const drawHysteresisBand = () => {
      const bandSamples = plotSamples.filter((sample) => (
        Number.isFinite(sample.target) &&
        Number.isFinite(sample.hysteresis) &&
        sample.hysteresis > 0
      ));
      if (bandSamples.length < 1) return;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.fillStyle = colors.hysteresis;
      ctx.beginPath();
      bandSamples.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.target + sample.hysteresis);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      bandSamples.slice().reverse().forEach((sample) => {
        ctx.lineTo(xFor(sample.timeMs), yFor(sample.target));
      });
      ctx.closePath();
      ctx.fill();
      ctx.strokeStyle = colors.target;
      ctx.globalAlpha = 0.42;
      ctx.setLineDash([5 * scale, 5 * scale]);
      ctx.beginPath();
      bandSamples.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.target + sample.hysteresis);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.restore();
    };

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
    const drawStateLane = () => {
      const rowGap = 1 * scale;
      const rowH = Math.max(1.5 * scale, Math.min(2.5 * scale, (stateLaneH - rowGap) / 2));
      const laneTop = pad.top + plotH - rowH * 2 - rowGap - stateInset;
      const rows = [
        { key: 'peltierRunning', color: colors.peltier, y: laneTop },
        { key: 'fanRunning', color: colors.fan, y: laneTop + rowH + rowGap }
      ];
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, laneTop, plotW, rowH * 2 + rowGap);
      ctx.clip();
      ctx.globalAlpha = 0.16;
      ctx.fillStyle = colors.grid;
      rows.forEach((row) => ctx.fillRect(pad.left, row.y, plotW, rowH));
      ctx.globalAlpha = 1;
      rows.forEach((row) => {
        ctx.fillStyle = row.color;
        plotSamples.forEach((sample, index) => {
          if (!sample[row.key]) return;
          const next = plotSamples[index + 1];
          const start = clamp(sample.timeMs, range.startMs, range.endMs);
          const end = next ? clamp(next.timeMs, range.startMs, range.endMs) : start;
          if (end <= start) return;
          ctx.globalAlpha = row.key === 'fanRunning' && sample.fanRunOnActive ? 0.52 : 0.82;
          ctx.fillRect(xFor(start), row.y, Math.max(1, xFor(end) - xFor(start)), rowH);
        });
      });
      ctx.restore();
    };
    const drawCurrentTemperatureMarker = () => {
      const latest = currentSample;
      if (!latest) return;
      const y = yFor(latest.temperature);
      const rawX = xFor(latest.timeMs);
      const latestVisible = latest.timeMs >= range.startMs && latest.timeMs <= range.endMs;
      const x = latestVisible
        ? clamp(rawX, pad.left, pad.left + plotW)
        : pad.left + plotW;
      ctx.save();
      ctx.strokeStyle = colors.current;
      ctx.lineWidth = 1 * scale;
      ctx.setLineDash([3 * scale, 5 * scale]);
      ctx.beginPath();
      ctx.moveTo(pad.left, y);
      ctx.lineTo(x, y);
      ctx.stroke();
      ctx.setLineDash([]);
      const label = latest.temperature.toFixed(1) + ' °C';
      ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
      const labelW = ctx.measureText(label).width + 8 * scale;
      const labelH = 18 * scale;
      const labelX = Math.max(2 * scale, pad.left - labelW - 4 * scale);
      const labelY = clamp(y - labelH / 2, pad.top, pad.top + plotH - labelH);
      ctx.fillStyle = colors.background;
      ctx.fillRect(labelX, labelY, labelW, labelH);
      ctx.strokeStyle = colors.current;
      ctx.strokeRect(labelX, labelY, labelW, labelH);
      ctx.fillStyle = colors.current;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(label, labelX + labelW / 2, labelY + labelH / 2);
      ctx.restore();
    };
    const drawPrediction = () => {
      if (!prediction || !prediction.points || prediction.points.length < 2) return;
      ctx.save();
      ctx.beginPath();
      ctx.rect(pad.left, pad.top, plotW, plotH);
      ctx.clip();
      ctx.strokeStyle = colors.prediction;
      ctx.lineWidth = 2 * scale;
      ctx.setLineDash([7 * scale, 5 * scale]);
      ctx.beginPath();
      prediction.points.forEach((sample, index) => {
        const x = xFor(sample.timeMs);
        const y = yFor(sample.temperature);
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.setLineDash([]);
      const eta = prediction.points[prediction.points.length - 1];
      if (eta.timeMs >= range.startMs && eta.timeMs <= range.endMs) {
        const x = xFor(eta.timeMs);
        const y = yFor(eta.temperature);
        ctx.fillStyle = colors.prediction;
        ctx.beginPath();
        ctx.arc(x, y, 3.5 * scale, 0, Math.PI * 2);
        ctx.fill();
        ctx.font = String(11 * scale) + 'px system-ui, sans-serif';
        ctx.textAlign = 'left';
        ctx.textBaseline = 'bottom';
        ctx.fillText('ETA ' + formatTime(eta.timeMs), Math.min(x + 7 * scale, pad.left + plotW - 62 * scale), y - 5 * scale);
      }
      ctx.restore();
    };
    drawHysteresisBand();
    drawLine('target', colors.target, false);
    drawPrediction();
    drawLine('temperature', colors.temperature, true);
    drawCurrentTemperatureMarker();
    drawStateLane();

    if (full) {
      ctx.textBaseline = 'middle';
      ctx.textAlign = 'left';
      ctx.font = '16px system-ui, sans-serif';
      ctx.fillStyle = colors.text;
      ctx.fillText('Temperature history', pad.left, 22);
      [
        ['Filtered temperature', colors.temperature],
        ['Target temperature', colors.target],
        ['Hysteresis band', colors.target],
        ['Peltier', colors.peltier],
        ['Fan', colors.fan],
        ...(predictionEnabled ? [['Prediction', colors.prediction]] : []),
        ['Sensor unavailable', colors.disconnected]
      ].forEach((item, index) => {
        const x = pad.left + index * 190;
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
      const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
      chartWindow.textContent = zoomRange
        ? 'Zoom ' + formatTime(range.startMs) + ' to ' + formatTime(range.endMs)
        : (prediction
            ? 'Startup to ' + formatTime(latestChartTime()) + ' / ETA ' + formatTime(prediction.etaMs)
            : 'Startup to ' + formatTime(latestChartTime()));
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
  const chartDisplayEnd = () => {
    const prediction = predictionEnabled ? buildTemperaturePrediction() : null;
    return Math.max(minChartWindowMs, latestChartTime(), prediction ? prediction.etaMs : 0);
  };
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
      '<span>Target: ' + formatTemperature(point.sample.target) + '</span>',
      '<span>Hys: ' + formatTemperature(point.sample.hysteresis) + '</span>',
      '<span>Peltier: ' + (point.sample.peltierRunning ? 'ON' : 'OFF') + '</span>',
      '<span>Fan: ' + (point.sample.fanRunning ? (point.sample.fanRunOnActive ? 'RUN-ON' : 'ON') : 'OFF') + '</span>'
    ].join('');
    tooltip.style.left = Math.min(point.cssX + 12, tooltip.parentElement.clientWidth - 150) + 'px';
    tooltip.style.top = Math.max(8, point.cssY - 52) + 'px';
    tooltip.classList.remove('hidden');
  };
  const downloadCsvFromSamples = () => {
    const rows = [['timestamp_ms', 'filtered_temperature_c', 'target_temperature_c', 'hysteresis_c', 'peltier_running', 'fan_running', 'fan_runon_active', 'sensor_status', 'sensor_valid', 'sensor_disconnected', 'flags']];
    chartSamples.forEach((sample) => {
      rows.push([
        Math.round(sample.timeMs),
        Number.isFinite(sample.temperature) ? sample.temperature.toFixed(1) : '',
        Number.isFinite(sample.target) ? sample.target.toFixed(1) : '',
        Number.isFinite(sample.hysteresis) ? sample.hysteresis.toFixed(1) : '',
        sample.peltierRunning ? '1' : '0',
        sample.fanRunning ? '1' : '0',
        sample.fanRunOnActive ? '1' : '0',
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
  const setPredictionEnabled = (enabled) => {
    predictionEnabled = Boolean(enabled);
    try {
      if (window.localStorage) {
        window.localStorage.setItem(predictionStorageKey, predictionEnabled ? '1' : '0');
      }
    } catch (error) {
      // Local storage can be unavailable in private or embedded contexts.
    }
    const toggle = document.getElementById('chartPredictionToggle');
    if (toggle) {
      toggle.textContent = predictionEnabled ? 'Prediction ON' : 'Prediction OFF';
      toggle.setAttribute('aria-pressed', predictionEnabled ? 'true' : 'false');
      toggle.title = predictionEnabled
        ? 'Hide browser-side temperature prediction'
        : 'Show browser-side temperature prediction';
    }
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
    setPredictionEnabled,
    samples: chartSamples
  };
  const pageUrl = (ip) => ip ? 'http://' + ip + '/' : '--';
  const otaStatusText = (data) => {
    if (!data.otaEnabled) return 'Disabled';
    if (data.otaUpdating) return 'Updating ' + (Number(data.otaProgress) || 0) + '%';
    if (data.otaStatus === 'failed') return 'Failed' + (data.otaLastError ? ': ' + data.otaLastError : '');
    if (data.otaStatus === 'completed') return 'Completed';
    if (data.otaStarted) return 'Ready';
    return data.stationConnected ? 'Starting' : 'Waiting for Wi-Fi';
  };
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
    const chartPredictionToggle = document.getElementById('chartPredictionToggle');
    const chartResetZoom = document.getElementById('chartResetZoom');
    const chartDownloadPng = document.getElementById('chartDownloadPng');
    const chartDownloadCsv = document.getElementById('chartDownloadCsv');
    const firmwareFileInput = document.getElementById('firmwareFileInput');
    const firmwareUpload = document.getElementById('firmwareUpload');
    const firmwareUploadState = document.getElementById('firmwareUploadState');
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
      setText('otaStatus', otaStatusText(data));
      setText('otaHostname', data.otaHostname ? data.otaHostname + (data.otaPasswordSet ? ' / password' : ' / open') : '--');
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
        networkList.textContent = '';
        networkScanState.textContent = 'Scan unavailable';
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
    setPredictionEnabled(loadPredictionPreference());
    if (chartPredictionToggle) {
      chartPredictionToggle.addEventListener('click', (event) => {
        setPredictionEnabled(!predictionEnabled);
        event.currentTarget.blur();
      });
    }
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
    firmwareUpload.addEventListener('click', () => {
      const file = firmwareFileInput.files && firmwareFileInput.files[0];
      if (!file) {
        firmwareUploadState.textContent = 'Choose firmware.bin first';
        return;
      }

      const body = new FormData();
      body.append('firmware', file, file.name);
      const request = new XMLHttpRequest();
      firmwareUpload.disabled = true;
      firmwareUploadState.textContent = 'Uploading...';

      request.upload.addEventListener('progress', (event) => {
        if (!event.lengthComputable) return;
        const percent = Math.round((event.loaded * 100) / event.total);
        firmwareUploadState.textContent = 'Uploading ' + percent + '%';
      });

      request.addEventListener('load', () => {
        firmwareUpload.disabled = false;
        if (request.status >= 200 && request.status < 300) {
          firmwareUploadState.textContent = 'Uploaded; rebooting';
          return;
        }
        try {
          const data = JSON.parse(request.responseText);
          firmwareUploadState.textContent = data.error || 'Upload failed';
        } catch (error) {
          firmwareUploadState.textContent = 'Upload failed';
        }
      });

      request.addEventListener('error', () => {
        firmwareUpload.disabled = false;
        firmwareUploadState.textContent = 'Upload failed';
      });

      request.open('POST', '/api/firmware');
      request.send(body);
    });
    settingsOverlay.addEventListener('click', (event) => {
      if (event.target === settingsOverlay) closeSettings();
    });
    document.addEventListener('keydown', (event) => {
      if (event.key === 'Escape' && settingsOverlay.classList.contains('open')) closeSettings();
    });
    async function loadStatus() {
      const res = await fetchWithTimeout('/api/status', { cache: 'no-store' }, statusFetchTimeoutMs);
      if (!res.ok) throw new Error('status unavailable');
      return res.json();
    }
    function renderStatusUnavailable() {
      setText('temperature', '-- °C');
      const sensor = document.getElementById('sensor');
      sensor.textContent = 'UNAVAILABLE';
      sensor.className = 'value small bad';
      setState('peltier', false);
      setState('fan', false);
      setText('runon', 'OFF');
      setText('count', '--');
      setText('uptime', '--');
      setText('wifi', 'Status unavailable');
      setText('ota', 'Status unavailable');
      drawTemperatureChart();
    }
    async function refresh() {
      let data;
      try {
        data = await loadStatus();
      } catch (error) {
        renderStatusUnavailable();
        return;
      }
      addChartSample(data);
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
      setText('ota', otaStatusText(data));
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
