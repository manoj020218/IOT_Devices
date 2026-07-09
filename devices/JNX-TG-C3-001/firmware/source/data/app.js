const dashboardOrder = [
  ["Project", "project"],
  ["PID", "pid"],
  ["Device ID", "device_id"],
  ["MAC", "mac"],
  ["SSID", "ssid_name"],
  ["Firmware", "firmware_version"],
  ["Build Date", "build_date"],
  ["WiFi", "wifi_status"],
  ["MQTT", "mqtt_status"],
  ["Water Level mm", "water_level_mm"],
  ["Raw Distance mm", "raw_distance_mm"],
  ["Filtered Distance mm", "filtered_distance_mm"],
  ["Zero Level mm", "zero_level_mm"],
  ["Bottom Start mm", "bottom_level_mm"],
  ["Top OFF mm", "top_level_mm"],
  ["State", "current_state"],
  ["Motor Status", "motor_status"],
  ["Last Command", "last_command"],
  ["Last Alarm", "last_alarm"],
  ["Sensor Status", "sensor_status"],
  ["Uptime s", "uptime_s"]
];

async function request(url, options = {}) {
  const response = await fetch(url, options);
  if (!response.ok) {
    let message = `HTTP ${response.status}`;
    try {
      const data = await response.json();
      message = data.error || message;
    } catch (error) {
      message = response.statusText || message;
    }
    throw new Error(message);
  }
  const type = response.headers.get("content-type") || "";
  if (type.includes("application/json")) {
    return response.json();
  }
  return response.text();
}

function setMessage(id, text, isError = false) {
  const node = document.getElementById(id);
  node.textContent = text;
  node.style.color = isError ? "#b64733" : "#1f6f5f";
}

function populateDashboard(data) {
  const container = document.getElementById("dashboard");
  container.innerHTML = "";
  dashboardOrder.forEach(([label, key]) => {
    const item = document.createElement("div");
    item.innerHTML = `<span>${label}</span><strong>${data[key] ?? "-"}</strong>`;
    container.appendChild(item);
  });
  document.getElementById("wifiStatus").textContent = data.wifi_status;
  document.getElementById("mqttStatus").textContent = data.mqtt_status;
  document.getElementById("stateName").textContent = data.current_state;
  setMessage("otaStatus", `${data.ota_phase}: ${data.ota_message || ""}`);
}

function populateConfig(data) {
  Object.entries(data).forEach(([key, value]) => {
    const field = document.querySelector(`[name="${key}"]`);
    if (field) {
      field.value = value;
    }
  });
  document.getElementById("otaUrl").value = data.ota_url || "";
}

async function refreshStatus() {
  const status = await request("/api/status");
  populateDashboard(status);
}

async function refreshConfig() {
  const config = await request("/api/config");
  populateConfig(config);
}

async function refreshLogs() {
  const logs = await request("/api/logs");
  document.getElementById("logsBox").textContent = JSON.stringify(logs, null, 2);
}

async function saveConfig() {
  const form = document.getElementById("configForm");
  const formData = new FormData(form);
  const payload = {};
  formData.forEach((value, key) => {
    payload[key] = /^\d+$/.test(value) ? Number(value) : value;
  });
  try {
    await request("/api/config", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    setMessage("configMessage", "Configuration saved.");
    await refreshStatus();
  } catch (error) {
    setMessage("configMessage", error.message, true);
  }
}

async function postAction(url, payload = {}) {
  return request(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });
}

function bindManualButtons() {
  document.querySelectorAll("[data-action]").forEach((button) => {
    button.addEventListener("click", async () => {
      try {
        await postAction("/api/manual", {
          action: button.dataset.action,
          override: document.getElementById("manualOverride").checked
        });
        await refreshStatus();
      } catch (error) {
        setMessage("configMessage", error.message, true);
      }
    });
  });
}

function bindButtons() {
  document.getElementById("saveConfig").addEventListener("click", saveConfig);
  document.getElementById("setZero").addEventListener("click", async () => {
    try {
      await postAction("/api/set-zero");
      setMessage("configMessage", "Zero level updated from current sensor reading.");
      await Promise.all([refreshConfig(), refreshStatus()]);
    } catch (error) {
      setMessage("configMessage", error.message, true);
    }
  });
  document.getElementById("restartDevice").addEventListener("click", async () => {
    try {
      await postAction("/api/restart");
      setMessage("configMessage", "Device restarting.");
    } catch (error) {
      setMessage("configMessage", error.message, true);
    }
  });
  document.getElementById("factoryReset").addEventListener("click", async () => {
    if (!confirm("Factory reset the device?")) {
      return;
    }
    try {
      await postAction("/api/factory-reset");
      setMessage("configMessage", "Factory reset requested.");
    } catch (error) {
      setMessage("configMessage", error.message, true);
    }
  });
  document.getElementById("queueCloudOta").addEventListener("click", async () => {
    try {
      await postAction("/api/ota/cloud", {
        url: document.getElementById("otaUrl").value.trim(),
        version: document.getElementById("otaVersion").value.trim(),
        checksum: document.getElementById("otaChecksum").value.trim()
      });
      setMessage("otaStatus", "Cloud OTA queued.");
    } catch (error) {
      setMessage("otaStatus", error.message, true);
    }
  });
  document.getElementById("uploadOta").addEventListener("click", async () => {
    const file = document.getElementById("otaFile").files[0];
    if (!file) {
      setMessage("otaStatus", "Select a .bin file first.", true);
      return;
    }
    const form = new FormData();
    form.append("firmware", file);
    try {
      await fetch("/update", { method: "POST", body: form });
      setMessage("otaStatus", "Firmware upload sent.");
    } catch (error) {
      setMessage("otaStatus", error.message, true);
    }
  });
  document.getElementById("loadLogs").addEventListener("click", refreshLogs);
  document.getElementById("downloadCsv").addEventListener("click", () => {
    window.location.href = "/api/logs?format=csv";
  });
}

async function boot() {
  bindManualButtons();
  bindButtons();
  await Promise.all([refreshStatus(), refreshConfig(), refreshLogs()]);
  window.setInterval(refreshStatus, 3000);
}

boot().catch((error) => {
  setMessage("configMessage", error.message, true);
});

