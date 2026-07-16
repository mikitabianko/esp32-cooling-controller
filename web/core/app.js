(function () {
  const setText = (id, text) => {
    const element = document.getElementById(id);
    if (element) element.textContent = text;
  };

  window.DeviceWeb = {
    ...(window.DeviceWeb || {}),
    setText
  };
}());
