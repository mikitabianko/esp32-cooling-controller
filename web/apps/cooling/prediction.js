(function () {
  const maxHorizonMs = 6 * 60 * 60 * 1000;
  const lookbackMs = 20 * 60 * 1000;
  const maxSampleGapMs = 3 * 60 * 1000;
  const minSpanMs = 90 * 1000;
  const minSamples = 4;
  const targetToleranceC = 0.05;
  const maxTargetShiftC = 0.25;
  const minClosingSlopeCPerSecond = 0.000015;

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
    return {
      slope,
      intercept,
      r2: syy > 0 ? Math.max(0, Math.min(1, (sxy * sxy) / (sxx * syy))) : 0,
      rmse: Math.sqrt(residualSum / sumW)
    };
  };

  const collectSamples = (samples, latest) => {
    const result = [];
    const startMs = latest.timeMs - lookbackMs;
    const targetBandC = Math.max(
      maxTargetShiftC,
      Number.isFinite(latest.hysteresis) ? Math.abs(latest.hysteresis) * 2 : 0
    );
    let previousValid = null;
    for (let index = samples.length - 1; index >= 0; index -= 1) {
      const sample = samples[index];
      if (sample.timeMs > latest.timeMs) continue;
      if (sample.timeMs < startMs) break;
      const valid = Number.isFinite(sample.timeMs) &&
        Number.isFinite(sample.temperature) &&
        Number.isFinite(sample.target) && !sample.disconnected;
      if (!valid) {
        if (result.length && sample.disconnected) break;
        continue;
      }
      if (Math.abs(sample.target - latest.target) > targetBandC) {
        if (result.length) break;
        continue;
      }
      if (previousValid && previousValid.timeMs - sample.timeMs > maxSampleGapMs) break;
      result.push(sample);
      previousValid = sample;
    }
    return result.reverse();
  };

  const trendStats = (samples, direction) => {
    let closingWeight = 0;
    let totalWeight = 0;
    let totalClosingC = 0;
    let totalSeconds = 0;
    for (let index = 1; index < samples.length; index += 1) {
      const previous = samples[index - 1];
      const sample = samples[index];
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
      if (closingC >= -targetToleranceC * 0.3) closingWeight += weight;
    }
    return {
      ratio: totalWeight > 0 ? closingWeight / totalWeight : 0,
      netSlope: totalSeconds > 0 ? totalClosingC / totalSeconds : 0
    };
  };

  const build = (samples, latestChartTime, keepaliveMs) => {
    const latest = samples.slice().reverse().find((sample) => (
      Number.isFinite(sample.temperature) && Number.isFinite(sample.target) && !sample.disconnected
    ));
    if (!latest || latestChartTime - latest.timeMs > keepaliveMs * 2) return null;
    const latestErrorC = latest.temperature - latest.target;
    const latestDistanceC = Math.abs(latestErrorC);
    if (!Number.isFinite(latestDistanceC) || latestDistanceC <= targetToleranceC) return null;
    const direction = latestErrorC >= 0 ? 1 : -1;
    const fitSamples = collectSamples(samples, latest)
      .map((sample) => ({
        ...sample,
        errorC: sample.temperature - sample.target,
        distanceC: direction * (sample.temperature - sample.target)
      }))
      .filter((sample) => sample.distanceC > targetToleranceC * 0.5);
    if (fitSamples.length < minSamples) return null;
    const spanMs = fitSamples[fitSamples.length - 1].timeMs - fitSamples[0].timeMs;
    if (spanMs < minSpanMs) return null;
    const stats = trendStats(fitSamples, direction);
    if (stats.ratio < 0.45 && stats.netSlope <= minClosingSlopeCPerSecond) return null;
    const durationMs = Math.max(1, spanMs);
    const weightedPoints = fitSamples.map((sample) => ({
      x: (sample.timeMs - latest.timeMs) / 1000,
      errorC: sample.errorC,
      distanceC: sample.distanceC,
      weight: 0.35 + 0.65 * Math.pow((sample.timeMs - fitSamples[0].timeMs) / durationMs, 2)
    }));
    const linearFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x, y: point.errorC, weight: point.weight
    })));
    const expFit = weightedLinearFit(weightedPoints.map((point) => ({
      x: point.x, y: Math.log(point.distanceC), weight: point.weight
    })));
    const maxEtaSeconds = maxHorizonMs / 1000;
    const linearClosingSlope = linearFit ? -direction * linearFit.slope : 0;
    const linearEtaSeconds = linearClosingSlope > minClosingSlopeCPerSecond
      ? (latestDistanceC - targetToleranceC) / linearClosingSlope : NaN;
    const expK = expFit ? -expFit.slope : 0;
    const expEtaSeconds = expK > 0 ? Math.log(latestDistanceC / targetToleranceC) / expK : NaN;
    const linearValid = Number.isFinite(linearEtaSeconds) && linearEtaSeconds >= 5 &&
      linearEtaSeconds <= maxEtaSeconds && linearClosingSlope > minClosingSlopeCPerSecond;
    const expValid = Number.isFinite(expEtaSeconds) && expEtaSeconds >= 5 &&
      expEtaSeconds <= maxEtaSeconds && expK > 0;
    if (!linearValid && !expValid) return null;
    const linearScore = linearFit
      ? linearFit.r2 - Math.min(1, linearFit.rmse / Math.max(latestDistanceC, 0.1)) * 0.25 + stats.ratio * 0.2
      : -Infinity;
    const expScore = expFit
      ? expFit.r2 - Math.min(1, expFit.rmse) * 0.2 + stats.ratio * 0.2 +
        (latestDistanceC > 0.3 ? 0.05 : 0)
      : -Infinity;
    const useExp = expValid && (!linearValid || expScore >= linearScore - 0.05);
    const etaMs = (useExp ? expEtaSeconds : linearEtaSeconds) * 1000;
    const endMs = latest.timeMs + etaMs;
    const points = [{ timeMs: latest.timeMs, temperature: latest.temperature }];
    const stepMs = Math.max(15000, Math.min(120000, etaMs / 24));
    for (let timeMs = latest.timeMs + stepMs; timeMs < endMs; timeMs += stepMs) {
      const elapsedSeconds = (timeMs - latest.timeMs) / 1000;
      const distanceC = useExp
        ? latestDistanceC * Math.exp(-expK * elapsedSeconds)
        : Math.max(targetToleranceC, latestDistanceC - linearClosingSlope * elapsedSeconds);
      points.push({
        timeMs,
        temperature: latest.target + direction * Math.max(targetToleranceC, distanceC)
      });
    }
    points.push({
      timeMs: endMs,
      temperature: latest.target + direction * targetToleranceC
    });
    return {
      points,
      etaMs: endMs,
      target: latest.target,
      model: useExp ? 'exponential' : 'linear',
      confidence: Math.max(0, Math.min(1, useExp ? expScore : linearScore))
    };
  };

  window.CoolingPrediction = { build };
}());
