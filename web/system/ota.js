(function () {
  const statusText = (data) => {
    if (!data.otaEnabled) return 'Disabled';
    if (data.otaUpdating) return 'Updating ' + (Number(data.otaProgress) || 0) + '%';
    if (data.otaStatus === 'failed') {
      return 'Failed' + (data.otaLastError ? ': ' + data.otaLastError : '');
    }
    if (data.otaStatus === 'completed') return 'Completed';
    if (data.otaStarted) return 'Ready';
    return data.stationConnected ? 'Starting' : 'Waiting for Wi-Fi';
  };

  const renderStatus = (data) => {
    const { setText } = window.DeviceWeb;
    setText('otaStatus', statusText(data));
    setText('otaHostname', data.otaHostname
      ? data.otaHostname + (data.otaPasswordSet ? ' / password' : ' / open')
      : '--');
  };

  const initUploader = () => {
    const fileInput = document.getElementById('firmwareFileInput');
    const uploadButton = document.getElementById('firmwareUpload');
    const state = document.getElementById('firmwareUploadState');
    uploadButton.addEventListener('click', () => {
      const file = fileInput.files && fileInput.files[0];
      if (!file) {
        state.textContent = 'Choose firmware.bin first';
        return;
      }
      const body = new FormData();
      body.append('firmware', file, file.name);
      const request = new XMLHttpRequest();
      uploadButton.disabled = true;
      state.textContent = 'Uploading...';
      request.upload.addEventListener('progress', (event) => {
        if (event.lengthComputable) {
          state.textContent = 'Uploading ' + Math.round((event.loaded * 100) / event.total) + '%';
        }
      });
      request.addEventListener('load', () => {
        uploadButton.disabled = false;
        if (request.status >= 200 && request.status < 300) {
          state.textContent = 'Uploaded; rebooting';
          return;
        }
        try {
          state.textContent = JSON.parse(request.responseText).error || 'Upload failed';
        } catch (error) {
          state.textContent = 'Upload failed';
        }
      });
      request.addEventListener('error', () => {
        uploadButton.disabled = false;
        state.textContent = 'Upload failed';
      });
      request.open('POST', '/api/firmware');
      request.send(body);
    });
  };

  window.SystemOta = { initUploader, renderStatus, statusText };
}());
