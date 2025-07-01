// Tab switching
document.getElementById('btnCreds').addEventListener('click', () => {
  document.getElementById('credsSection').classList.add('active');
  document.getElementById('wifiSection').classList.remove('active');
});
document.getElementById('btnWifi').addEventListener('click', () => {
  document.getElementById('wifiSection').classList.add('active');
  document.getElementById('credsSection').classList.remove('active');
});

// SSID change handler
document.getElementById('wifiForm').addEventListener('submit', async e => {
  e.preventDefault();
  const ssid = document.getElementById('ssidInput').value.trim();
  const res = await fetch('/admin/wifi', {
    method: 'POST',
    headers: {'Content-Type':'application/json'},
    body: JSON.stringify({ ssid })
  });
  const json = await res.json();
  const out = document.getElementById('wifiResult');
  if (json.result === 'ok') {
    out.style.color = '#0a0';
    out.textContent = `SSID changed to "${json.ssid}"`;
  } else {
    out.style.color = '#a00';
    out.textContent = 'Error changing SSID';
  }
});
