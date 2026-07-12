#include "web_server.h"

#include <ArduinoJson.h>
#include <Update.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "cloud_client.h"
#include "config_manager.h"
#include "logger.h"
#include "mqtt_client.h"
#include "ota_manager.h"
#include "rf_output.h"
#include "sensor_dyp.h"
#include "tank_logic.h"

namespace {

WebServer g_server(80);
DeviceConfig *g_config = nullptr;
SystemStatus *g_system = nullptr;
char g_session_token[17] = "";
uint32_t g_session_refreshed_ms = 0;
constexpr uint32_t SESSION_TIMEOUT_MS = 30UL * 60UL * 1000UL;

const char LOGIN_HTML[] PROGMEM = R"TGLOGIN(
<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tank Guard Login</title>
<style>
*{box-sizing:border-box}
body{font-family:Segoe UI,sans-serif;background:#134f45;display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0;padding:16px}
.card{background:#fff;border-radius:14px;padding:32px 24px;width:100%;max-width:340px;box-shadow:0 6px 24px rgba(0,0,0,.28)}
h2{margin:0 0 8px;color:#134f45;font-size:22px;text-align:center}
.sub{text-align:center;color:#7a8792;font-size:12px;margin-bottom:22px}
label{font-size:13px;color:#55616c;display:block;margin-bottom:6px}
.pw-wrap{position:relative}
input{width:100%;padding:11px 42px 11px 12px;border:1px solid #d0d7de;border-radius:10px;font-size:15px;outline:none}
input:focus{border-color:#134f45}
.eye{position:absolute;right:12px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:16px;color:#55616c}
.btn{width:100%;margin-top:18px;padding:12px;background:#134f45;color:#fff;border:none;border-radius:10px;font-size:15px;font-weight:600;cursor:pointer}
.err{color:#b64733;font-size:13px;text-align:center;margin-top:10px;min-height:18px}
</style>
</head>
<body>
<div class="card">
<h2>Smart Tank Guard</h2>
<div class="sub">%DEVNAME%</div>
<form method="POST" action="/login">
<label>Login Password</label>
<div class="pw-wrap">
<input id="pw" type="password" name="password" autocomplete="current-password" required>
<span class="eye" onclick="var i=document.getElementById('pw');i.type=i.type==='password'?'text':'password'">&#128065;</span>
</div>
<button class="btn" type="submit">Login</button>
<div class="err">%ERR%</div>
</form>
</div>
</body></html>
)TGLOGIN";

const char EMBEDDED_INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Tank Guard</title>
  <style>
    :root{--bg:#f4f0e8;--panel:#fffdf8;--ink:#18222b;--muted:#667482;--line:#d7cfc2;--accent:#1f6f5f;--danger:#b64733}
    *{box-sizing:border-box} body{margin:0;font-family:Segoe UI,sans-serif;color:var(--ink);background:linear-gradient(180deg,#f9f6ef 0%,var(--bg) 100%)}
    .page{max-width:1200px;margin:0 auto;padding:20px}.panel{background:var(--panel);border:1px solid var(--line);border-radius:16px;padding:18px;margin-bottom:16px}
    .hero{display:grid;grid-template-columns:1.7fr 1fr;gap:16px}.statgrid,.cfg{display:grid;gap:12px}.statgrid{grid-template-columns:repeat(2,minmax(0,1fr))}
    .cfg{grid-template-columns:repeat(3,minmax(0,1fr))} .kv{padding:10px;border:1px solid #ece4d7;border-radius:12px;background:#fff}
    .kv span{display:block;color:var(--muted);font-size:12px}.kv strong{display:block;margin-top:4px}
    input,select,button,textarea{font:inherit} input,select{width:100%;padding:10px 12px;border:1px solid var(--line);border-radius:10px;background:#fff}
    label{display:grid;gap:6px;font-size:13px;color:var(--muted)} button{border:0;border-radius:999px;padding:11px 16px;background:#e9e2d5;cursor:pointer}
    button.primary{background:var(--accent);color:#fff} button.danger{background:var(--danger);color:#fff}.row{display:flex;gap:10px;flex-wrap:wrap}
    .msg{min-height:18px;color:var(--muted)} pre{white-space:pre-wrap;background:#fff;border:1px solid #ece4d7;border-radius:12px;padding:14px;min-height:160px;overflow:auto}
    .muted{color:var(--muted)} .topbar{display:flex;justify-content:space-between;align-items:center;gap:12px} .pw-note{font-size:12px;color:var(--muted)}
    @media(max-width:980px){.hero,.cfg{grid-template-columns:1fr}}
  </style>
</head>
<body>
  <main class="page">
    <section class="panel hero">
      <div>
        <div class="topbar">
          <div>
            <p class="muted" style="margin:0 0 8px">Smart Tank Guard by Jenix</p>
            <h1 style="margin:0 0 8px">Controller Dashboard</h1>
          </div>
          <a href="/logout">Logout</a>
        </div>
        <p class="muted">Local login, local OTA, and VPS or MQTT-driven updates are enabled in this firmware.</p>
      </div>
      <div class="statgrid" id="statusBox"></div>
    </section>

    <section class="panel">
      <h2 style="margin-top:0">Manual Actions</h2>
      <div class="row">
        <button class="primary" onclick="manualAction('motor_on')">RF Motor ON</button>
        <button onclick="manualAction('motor_off')">RF Motor OFF</button>
        <button onclick="manualAction('alarm_test')">RF Alarm Pulse</button>
      </div>
      <p class="msg" id="manualMsg"></p>
    </section>

    <section class="panel">
      <h2 style="margin-top:0">Configuration</h2>
      <p class="pw-note">Leave WiFi or MQTT password blank to keep the existing saved password.</p>
      <div class="cfg">
        <label>Device ID<input id="device_id"></label>
        <label>Site Name<input id="site_name"></label>
        <label>Tank Name<input id="tank_name"></label>
        <label>WiFi Mode<select id="wifi_mode"><option value="0">AP fallback</option><option value="1">Station + AP recovery</option></select></label>
        <label>WiFi SSID<input id="wifi_ssid"></label>
        <label>WiFi Password<input id="wifi_password" type="password" placeholder="Leave blank to keep current"></label>
        <label>WiFi TX Power
          <input id="wifi_tx_power_slider" type="range" min="0" max="7" step="1" value="0" oninput="updateWifiPowerUi()">
          <span class="muted" id="wifi_tx_power_label">8.5 dBm</span>
          <input id="wifi_tx_power_dbm_tenths" type="hidden">
        </label>
        <label>MQTT Enabled<select id="mqtt_enabled"><option value="0">Disabled</option><option value="1">Enabled</option></select></label>
        <label>MQTT Host<input id="mqtt_host" placeholder="mqtt.iotsoft.in"></label>
        <label>MQTT Port<input id="mqtt_port" type="number"></label>
        <label>MQTT Username<input id="mqtt_username"></label>
        <label>MQTT Password<input id="mqtt_password" type="password" placeholder="Leave blank to keep current"></label>
        <label>Cloud API URL<input id="cloud_base_url" placeholder="https://api.example.com"></label>
        <label>Device Ingest Key<input id="device_ingest_key" type="password" placeholder="Leave blank to keep current"></label>
        <label>Cloud HOME ID<input id="cloud_home_id"></label>
        <label>Owner User ID<input id="cloud_owner_user_id"></label>
        <label>OTA URL<input id="ota_url"></label>
        <label>OTA Channel<input id="ota_channel"></label>
        <label>Zero Level mm<input id="zero_level_mm" type="number"></label>
        <label>Bottom Start mm<input id="bottom_motor_start_level_mm" type="number"></label>
        <label>Top OFF mm<input id="top_motor_off_level_mm" type="number"></label>
        <label>Overflow Margin mm<input id="overflow_margin_mm" type="number"></label>
        <label>Restore Wait min<input id="power_restore_wait_minutes" type="number"></label>
        <label>Rise Confirm min<input id="motor_start_confirm_minutes" type="number"></label>
        <label>OFF Confirm min<input id="motor_off_confirm_minutes" type="number"></label>
        <label>Rise Delta mm<input id="water_rise_confirm_mm" type="number"></label>
        <label>RF ON Pulse ms<input id="rf_on_pulse_ms" type="number"></label>
        <label>RF OFF Pulse ms<input id="rf_off_pulse_ms" type="number"></label>
        <label>RF Alarm Pulse ms<input id="rf_alarm_pulse_ms" type="number"></label>
        <label>RF ON Retries<input id="rf_on_max_retries" type="number"></label>
        <label>RF OFF Retries<input id="rf_off_max_retries" type="number"></label>
        <label>RF Retry Gap min<input id="rf_retry_gap_minutes" type="number"></label>
        <label>Alarm Repeat<select id="alarm_repeat_enable"><option value="1">Enabled</option><option value="0">Disabled</option></select></label>
        <label>Alarm Repeat min<input id="alarm_repeat_minutes" type="number"></label>
        <label>Telemetry sec<input id="telemetry_interval_seconds" type="number"></label>
      </div>
      <div class="row" style="margin-top:14px">
        <button class="primary" onclick="saveConfig()">Save Config</button>
        <button onclick="setZero()">Set Zero From Current Reading</button>
        <button onclick="restartDevice()">Restart</button>
        <button class="danger" onclick="factoryReset()">Factory Reset</button>
      </div>
      <p class="msg" id="cfgMsg"></p>
    </section>

    <section class="panel">
      <h2 style="margin-top:0">Change Login Password</h2>
      <div class="cfg">
        <label>Current Password<input id="current_password" type="password"></label>
        <label>New Password<input id="new_password" type="password"></label>
      </div>
      <div class="row" style="margin-top:14px">
        <button class="primary" onclick="changePassword()">Change Password</button>
      </div>
      <p class="msg" id="pwMsg"></p>
    </section>

    <section class="panel">
      <h2 style="margin-top:0">OTA Update</h2>
      <div class="cfg">
        <label>Cloud or VPS Firmware URL<input id="ota_req_url" placeholder="https://example.com/fw.bin"></label>
        <label>Target Version<input id="ota_req_version" placeholder="JNX-TG-C3 v1.0.1"></label>
        <label>MD5 Checksum<input id="ota_req_checksum" placeholder="optional 32-char md5"></label>
      </div>
      <div class="row" style="margin-top:14px">
        <button class="primary" onclick="queueCloudOta()">Queue Cloud / VPS OTA</button>
      </div>
      <hr>
      <label>Local Firmware File<input id="ota_file" type="file" accept=".bin"></label>
      <div class="row" style="margin-top:14px">
        <button onclick="uploadLocalOta()">Upload Local Firmware</button>
      </div>
      <p class="msg" id="otaMsg"></p>
    </section>

    <section class="panel">
      <h2 style="margin-top:0">Logs</h2>
      <div class="row">
        <button onclick="loadLogs()">Refresh Logs</button>
        <button onclick="window.location='/api/logs?format=csv'">Download CSV</button>
      </div>
      <pre id="logsBox">Loading logs...</pre>
    </section>
  </main>
  <script>
    const WIFI_TX_POWER_OPTIONS = [
      {dbmTenths:85, label:'8.5 dBm'},
      {dbmTenths:110, label:'11 dBm'},
      {dbmTenths:130, label:'13 dBm'},
      {dbmTenths:150, label:'15 dBm'},
      {dbmTenths:170, label:'17 dBm'},
      {dbmTenths:185, label:'18.5 dBm'},
      {dbmTenths:190, label:'19 dBm'},
      {dbmTenths:195, label:'19.5 dBm'}
    ];

    async function api(url, options){
      const response = await fetch(url, options || {});
      const type = response.headers.get('content-type') || '';
      let payload = null;
      if(type.includes('application/json')) payload = await response.json();
      else payload = await response.text();
      if(!response.ok){
        const msg = payload && payload.error ? payload.error : (typeof payload === 'string' ? payload : ('HTTP '+response.status));
        throw new Error(msg);
      }
      return payload;
    }

    function setMsg(id, text, isError){
      const el = document.getElementById(id);
      el.textContent = text || '';
      el.style.color = isError ? '#b64733' : '#1f6f5f';
    }

    function wifiPowerIndexFromTenths(value){
      const requested = Number(value || 85);
      let bestIndex = 0;
      let bestDiff = Infinity;
      WIFI_TX_POWER_OPTIONS.forEach((option, index) => {
        const diff = Math.abs(option.dbmTenths - requested);
        if(diff < bestDiff){
          bestDiff = diff;
          bestIndex = index;
        }
      });
      return bestIndex;
    }

    function updateWifiPowerUi(){
      const slider = document.getElementById('wifi_tx_power_slider');
      const hidden = document.getElementById('wifi_tx_power_dbm_tenths');
      const label = document.getElementById('wifi_tx_power_label');
      const option = WIFI_TX_POWER_OPTIONS[Number(slider.value || 0)] || WIFI_TX_POWER_OPTIONS[0];
      hidden.value = option.dbmTenths;
      label.textContent = option.label;
    }

    function setWifiPowerFromTenths(value){
      const slider = document.getElementById('wifi_tx_power_slider');
      slider.value = wifiPowerIndexFromTenths(value);
      updateWifiPowerUi();
    }

    function fields(){
      return [
        'device_id','site_name','tank_name','wifi_mode','wifi_ssid','wifi_password',
        'wifi_tx_power_dbm_tenths',
        'mqtt_enabled','mqtt_host','mqtt_port','mqtt_username','mqtt_password',
        'cloud_base_url','device_ingest_key','cloud_home_id','cloud_owner_user_id',
        'ota_url','ota_channel',
        'zero_level_mm','bottom_motor_start_level_mm','top_motor_off_level_mm','overflow_margin_mm',
        'power_restore_wait_minutes','motor_start_confirm_minutes','motor_off_confirm_minutes',
        'water_rise_confirm_mm','rf_on_pulse_ms','rf_off_pulse_ms','rf_alarm_pulse_ms',
        'rf_on_max_retries','rf_off_max_retries','rf_retry_gap_minutes','alarm_repeat_enable',
        'alarm_repeat_minutes','telemetry_interval_seconds'
      ];
    }

    async function refreshStatus(){
      const d = await api('/api/status');
      const items = [
        ['PID', d.pid], ['SSID', d.ssid_name], ['MAC', d.mac], ['WiFi', d.wifi_status],
        ['Local IP', d.local_ip], ['Local URL', d.local_url], ['RSSI dBm', d.wifi_rssi_dbm],
        ['WiFi TX', d.wifi_tx_power_label],
        ['MQTT', d.mqtt_status], ['Cloud', d.cloud_phase],
        ['State', d.current_state], ['Motor', d.motor_status], ['Water mm', d.water_level_mm],
        ['Raw mm', d.raw_distance_mm], ['Zero mm', d.zero_level_mm], ['Top mm', d.top_level_mm],
        ['Alarm', d.last_alarm || d.alarm_code || '-'], ['Uptime s', d.uptime_s]
      ];
      const box = document.getElementById('statusBox');
      box.innerHTML = items.map(([k,v]) => '<div class="kv"><span>'+k+'</span><strong>'+(v ?? '-')+'</strong></div>').join('');
    }

    async function refreshConfig(){
      const d = await api('/api/config');
      fields().forEach(id => {
        const el = document.getElementById(id);
        if(el && d[id] !== undefined) el.value = d[id];
      });
      setWifiPowerFromTenths(d.wifi_tx_power_dbm_tenths ?? 85);
      document.getElementById('wifi_password').value = '';
      document.getElementById('mqtt_password').value = '';
      document.getElementById('device_ingest_key').value = '';
    }

    async function saveConfig(){
      try{
        updateWifiPowerUi();
        const payload = {};
        fields().forEach(id => {
          const el = document.getElementById(id);
          if(!el) return;
          payload[id] = /^-?\\d+$/.test(el.value) ? Number(el.value) : el.value;
        });
        await api('/api/config', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(payload)});
        setMsg('cfgMsg', 'Configuration saved.', false);
        await refreshStatus();
        await refreshConfig();
      }catch(error){ setMsg('cfgMsg', error.message, true); }
    }

    async function manualAction(action){
      try{
        await api('/api/manual', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify({action})});
        setMsg('manualMsg', 'Action sent: '+action, false);
        await refreshStatus();
      }catch(error){ setMsg('manualMsg', error.message, true); }
    }

    async function setZero(){
      try{
        await api('/api/set-zero', {method:'POST'});
        setMsg('cfgMsg', 'Zero calibrated from current reading.', false);
        await refreshConfig();
        await refreshStatus();
      }catch(error){ setMsg('cfgMsg', error.message, true); }
    }

    async function restartDevice(){
      try{ await api('/api/restart', {method:'POST'}); setMsg('cfgMsg', 'Restart requested.', false); }
      catch(error){ setMsg('cfgMsg', error.message, true); }
    }

    async function factoryReset(){
      if(!confirm('Factory reset the device?')) return;
      try{ await api('/api/factory-reset', {method:'POST'}); setMsg('cfgMsg', 'Factory reset requested.', false); }
      catch(error){ setMsg('cfgMsg', error.message, true); }
    }

    async function changePassword(){
      try{
        await api('/api/change-password', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({
            current_password: document.getElementById('current_password').value,
            new_password: document.getElementById('new_password').value
          })
        });
        document.getElementById('current_password').value = '';
        document.getElementById('new_password').value = '';
        setMsg('pwMsg', 'Login password changed.', false);
      }catch(error){ setMsg('pwMsg', error.message, true); }
    }

    async function queueCloudOta(){
      try{
        await api('/api/ota/cloud', {
          method:'POST',
          headers:{'Content-Type':'application/json'},
          body:JSON.stringify({
            url: document.getElementById('ota_req_url').value.trim(),
            version: document.getElementById('ota_req_version').value.trim(),
            checksum: document.getElementById('ota_req_checksum').value.trim()
          })
        });
        setMsg('otaMsg', 'Cloud / VPS OTA queued.', false);
      }catch(error){ setMsg('otaMsg', error.message, true); }
    }

    async function uploadLocalOta(){
      const file = document.getElementById('ota_file').files[0];
      if(!file){ setMsg('otaMsg', 'Select a .bin file first.', true); return; }
      const form = new FormData();
      form.append('firmware', file);
      try{
        const response = await fetch('/update', {method:'POST', body:form});
        const text = await response.text();
        if(!response.ok) throw new Error(text || ('HTTP '+response.status));
        setMsg('otaMsg', text || 'OTA upload complete.', false);
      }catch(error){ setMsg('otaMsg', error.message, true); }
    }

    async function loadLogs(){
      try{
        const data = await api('/api/logs');
        document.getElementById('logsBox').textContent = JSON.stringify(data, null, 2);
      }catch(error){
        document.getElementById('logsBox').textContent = error.message;
      }
    }

    async function boot(){
      updateWifiPowerUi();
      await refreshStatus();
      await refreshConfig();
      await loadLogs();
      setInterval(refreshStatus, 3000);
    }
    boot();
  </script>
</body>
</html>
)HTML";

void session_create() {
  snprintf(g_session_token, sizeof(g_session_token), "%08X%08X", esp_random(), esp_random());
  g_session_refreshed_ms = millis();
}

bool session_valid() {
  if (g_session_token[0] == '\0') {
    return false;
  }
  if ((millis() - g_session_refreshed_ms) >= SESSION_TIMEOUT_MS) {
    g_session_token[0] = '\0';
    return false;
  }
  const String cookie = g_server.header("Cookie");
  const String needle = String("sess=") + g_session_token;
  if (cookie.indexOf(needle) < 0) {
    return false;
  }
  g_session_refreshed_ms = millis();
  return true;
}

void require_auth_redirect() {
  g_server.sendHeader("Location", "/login");
  g_server.send(302, "text/plain", "");
}

bool ensure_api_auth() {
  if (session_valid()) {
    return true;
  }
  g_server.send(403, "application/json", "{\"ok\":false,\"error\":\"Forbidden\"}");
  return false;
}

void serve_login(const char *err = "") {
  String html = FPSTR(LOGIN_HTML);
  html.replace("%DEVNAME%", g_system->device_name);
  html.replace("%ERR%", err);
  g_server.send(200, "text/html", html);
}

String build_status_json() {
  const SensorSnapshot &sensor = sensor_dyp_get_snapshot();
  const TankRuntime &tank = tank_logic_get_runtime();
  const RfRuntime &rf = rf_output_get_runtime();

  DynamicJsonDocument doc(2560);
  doc["project"] = PRODUCT_NAME;
  doc["pid"] = PRODUCT_PID;
  doc["sku"] = PRODUCT_SKU;
  doc["device_id"] = g_config->device_id;
  doc["site_name"] = g_config->site_name;
  doc["tank_name"] = g_config->tank_name;
  doc["mac"] = g_system->mac_address;
  doc["ssid_name"] = g_system->active_wifi_ssid;
  doc["mdns"] = g_system->mdns_name;
  doc["local_ip"] = g_system->local_ip;
  doc["local_url"] = g_system->local_url;
  doc["firmware_version"] = PRODUCT_FW_VERSION;
  doc["build_date"] = PRODUCT_BUILD_DATE;
  doc["hardware_revision"] = PRODUCT_HW_REVISION;
  doc["wifi_status"] = g_system->sta_connected ? "connected" : (g_system->ap_active ? "ap_mode" : "disconnected");
  doc["wifi_rssi_dbm"] = g_system->wifi_rssi_dbm;
  doc["wifi_tx_power_dbm_tenths"] = g_system->wifi_tx_power_dbm_tenths;
  doc["wifi_tx_power_dbm"] = wifi_tx_power_dbm_value(g_system->wifi_tx_power_dbm_tenths);
  doc["wifi_tx_power_label"] =
      wifi_tx_power_option_for_dbm_tenths(g_system->wifi_tx_power_dbm_tenths).label;
  doc["mqtt_status"] = mqtt_client_is_connected() ? "connected" : "disconnected";
  doc["cloud_registered"] = g_system->cloud_registered;
  doc["cloud_phase"] = cloud_client_phase();
  doc["cloud_message"] = cloud_client_message();
  doc["sta_connected"] = g_system->sta_connected;
  doc["ap_active"] = g_system->ap_active;
  doc["ap_ip"] = g_system->ap_ip;
  doc["sta_ip"] = g_system->sta_ip;
  doc["ap_recovery_remaining_s"] = g_system->ap_recovery_remaining_s;
  doc["water_level_mm"] = tank.water_level_mm;
  doc["raw_distance_mm"] = sensor.raw_distance_mm;
  doc["filtered_distance_mm"] = sensor.filtered_distance_mm;
  doc["zero_level_mm"] = g_config->zero_level_mm;
  doc["bottom_level_mm"] = g_config->bottom_motor_start_level_mm;
  doc["top_level_mm"] = g_config->top_motor_off_level_mm;
  doc["overflow_margin_mm"] = g_config->overflow_margin_mm;
  doc["current_state"] = tank_state_to_string(tank.state);
  doc["motor_status"] = motor_status_to_string(tank.motor_status);
  doc["water_trend"] = water_trend_to_string(tank.water_trend);
  doc["rise_rate_mm_per_min"] = tank.rise_rate_mm_per_min;
  doc["last_command"] = last_command_to_string(tank.last_command);
  doc["last_command_ms"] = tank.last_command_ms;
  doc["last_alarm"] = tank.last_alarm_message;
  doc["alarm_code"] = alarm_code_to_string(tank.alarm_code);
  doc["alarm_active"] = tank.alarm_active;
  doc["sensor_status"] = sensor_status_to_string(sensor.status);
  doc["uptime_s"] = g_system->uptime_s;
  doc["rf_on_count"] = rf.rf_on_count;
  doc["rf_off_count"] = rf.rf_off_count;
  doc["rf_alarm_count"] = rf.rf_alarm_count;
  doc["ota_phase"] = ota_manager_phase();
  doc["ota_message"] = ota_manager_message();

  String output;
  serializeJson(doc, output);
  return output;
}

String build_config_json() {
  DynamicJsonDocument doc(2304);
  doc["device_id"] = g_config->device_id;
  doc["site_name"] = g_config->site_name;
  doc["tank_name"] = g_config->tank_name;
  doc["wifi_mode"] = g_config->wifi_mode;
  doc["wifi_ssid"] = g_config->wifi_ssid;
  doc["wifi_tx_power_dbm_tenths"] = g_config->wifi_tx_power_dbm_tenths;
  doc["mqtt_enabled"] = g_config->mqtt_enabled;
  doc["mqtt_host"] = g_config->mqtt_host;
  doc["mqtt_port"] = g_config->mqtt_port;
  doc["mqtt_username"] = g_config->mqtt_username;
  doc["cloud_base_url"] = g_config->cloud_base_url;
  doc["cloud_home_id"] = g_config->cloud_home_id;
  doc["cloud_owner_user_id"] = g_config->cloud_owner_user_id;
  doc["zero_level_mm"] = g_config->zero_level_mm;
  doc["bottom_motor_start_level_mm"] = g_config->bottom_motor_start_level_mm;
  doc["top_motor_off_level_mm"] = g_config->top_motor_off_level_mm;
  doc["overflow_margin_mm"] = g_config->overflow_margin_mm;
  doc["power_restore_wait_minutes"] = g_config->power_restore_wait_minutes;
  doc["motor_start_confirm_minutes"] = g_config->motor_start_confirm_minutes;
  doc["motor_off_confirm_minutes"] = g_config->motor_off_confirm_minutes;
  doc["water_rise_confirm_mm"] = g_config->water_rise_confirm_mm;
  doc["rf_on_pulse_ms"] = g_config->rf_on_pulse_ms;
  doc["rf_off_pulse_ms"] = g_config->rf_off_pulse_ms;
  doc["rf_alarm_pulse_ms"] = g_config->rf_alarm_pulse_ms;
  doc["rf_on_max_retries"] = g_config->rf_on_max_retries;
  doc["rf_off_max_retries"] = g_config->rf_off_max_retries;
  doc["rf_retry_gap_minutes"] = g_config->rf_retry_gap_minutes;
  doc["alarm_repeat_enable"] = g_config->alarm_repeat_enable;
  doc["alarm_repeat_minutes"] = g_config->alarm_repeat_minutes;
  doc["telemetry_interval_seconds"] = g_config->telemetry_interval_seconds;
  doc["rf_on_active_high"] = g_config->rf_on_active_high;
  doc["rf_off_active_high"] = g_config->rf_off_active_high;
  doc["rf_alarm_active_high"] = g_config->rf_alarm_active_high;
  doc["ota_url"] = g_config->ota_url;
  doc["ota_channel"] = g_config->ota_channel;

  String output;
  serializeJson(doc, output);
  return output;
}

void apply_config_patch_from_json(JsonVariantConst doc, DeviceConfig &candidate) {
  if (doc.containsKey("zero_level_mm")) candidate.zero_level_mm = doc["zero_level_mm"];
  if (doc.containsKey("bottom_motor_start_level_mm")) {
    candidate.bottom_motor_start_level_mm = doc["bottom_motor_start_level_mm"];
  }
  if (doc.containsKey("top_motor_off_level_mm")) {
    candidate.top_motor_off_level_mm = doc["top_motor_off_level_mm"];
  }
  if (doc.containsKey("overflow_margin_mm")) candidate.overflow_margin_mm = doc["overflow_margin_mm"];
  if (doc.containsKey("power_restore_wait_minutes")) {
    candidate.power_restore_wait_minutes = doc["power_restore_wait_minutes"];
  }
  if (doc.containsKey("motor_start_confirm_minutes")) {
    candidate.motor_start_confirm_minutes = doc["motor_start_confirm_minutes"];
  }
  if (doc.containsKey("motor_off_confirm_minutes")) {
    candidate.motor_off_confirm_minutes = doc["motor_off_confirm_minutes"];
  }
  if (doc.containsKey("water_rise_confirm_mm")) candidate.water_rise_confirm_mm = doc["water_rise_confirm_mm"];
  if (doc.containsKey("rf_on_pulse_ms")) candidate.rf_on_pulse_ms = doc["rf_on_pulse_ms"];
  if (doc.containsKey("rf_off_pulse_ms")) candidate.rf_off_pulse_ms = doc["rf_off_pulse_ms"];
  if (doc.containsKey("rf_alarm_pulse_ms")) candidate.rf_alarm_pulse_ms = doc["rf_alarm_pulse_ms"];
  if (doc.containsKey("rf_on_max_retries")) candidate.rf_on_max_retries = doc["rf_on_max_retries"];
  if (doc.containsKey("rf_off_max_retries")) candidate.rf_off_max_retries = doc["rf_off_max_retries"];
  if (doc.containsKey("rf_retry_gap_minutes")) candidate.rf_retry_gap_minutes = doc["rf_retry_gap_minutes"];
  if (doc.containsKey("alarm_repeat_enable")) candidate.alarm_repeat_enable = doc["alarm_repeat_enable"];
  if (doc.containsKey("alarm_repeat_minutes")) candidate.alarm_repeat_minutes = doc["alarm_repeat_minutes"];
  if (doc.containsKey("telemetry_interval_seconds")) {
    candidate.telemetry_interval_seconds = doc["telemetry_interval_seconds"];
  }
  if (doc.containsKey("wifi_tx_power_dbm_tenths")) {
    candidate.wifi_tx_power_dbm_tenths = doc["wifi_tx_power_dbm_tenths"];
  }
  if (doc.containsKey("wifi_mode")) candidate.wifi_mode = doc["wifi_mode"];
  if (doc.containsKey("mqtt_enabled")) candidate.mqtt_enabled = doc["mqtt_enabled"];
  if (doc.containsKey("mqtt_port")) candidate.mqtt_port = doc["mqtt_port"];

  if (doc.containsKey("device_id")) {
    strlcpy(candidate.device_id, doc["device_id"] | candidate.device_id, sizeof(candidate.device_id));
  }
  if (doc.containsKey("site_name")) {
    strlcpy(candidate.site_name, doc["site_name"] | candidate.site_name, sizeof(candidate.site_name));
  }
  if (doc.containsKey("tank_name")) {
    strlcpy(candidate.tank_name, doc["tank_name"] | candidate.tank_name, sizeof(candidate.tank_name));
  }
  if (doc.containsKey("wifi_ssid")) {
    strlcpy(candidate.wifi_ssid, doc["wifi_ssid"] | candidate.wifi_ssid, sizeof(candidate.wifi_ssid));
  }
  if (doc.containsKey("wifi_password")) {
    const char *wifi_password = doc["wifi_password"] | "";
    if (wifi_password[0] != '\0') {
      strlcpy(candidate.wifi_password, wifi_password, sizeof(candidate.wifi_password));
    }
  }
  if (doc.containsKey("mqtt_host")) {
    strlcpy(candidate.mqtt_host, doc["mqtt_host"] | candidate.mqtt_host, sizeof(candidate.mqtt_host));
  }
  if (doc.containsKey("mqtt_username")) {
    strlcpy(candidate.mqtt_username, doc["mqtt_username"] | candidate.mqtt_username,
            sizeof(candidate.mqtt_username));
  }
  if (doc.containsKey("mqtt_password")) {
    const char *mqtt_password = doc["mqtt_password"] | "";
    if (mqtt_password[0] != '\0') {
      strlcpy(candidate.mqtt_password, mqtt_password, sizeof(candidate.mqtt_password));
    }
  }
  if (doc.containsKey("cloud_base_url")) {
    strlcpy(candidate.cloud_base_url, doc["cloud_base_url"] | candidate.cloud_base_url,
            sizeof(candidate.cloud_base_url));
  }
  if (doc.containsKey("device_ingest_key")) {
    const char *device_ingest_key = doc["device_ingest_key"] | "";
    if (device_ingest_key[0] != '\0') {
      strlcpy(candidate.device_ingest_key, device_ingest_key, sizeof(candidate.device_ingest_key));
    }
  }
  if (doc.containsKey("cloud_home_id")) {
    strlcpy(candidate.cloud_home_id, doc["cloud_home_id"] | candidate.cloud_home_id,
            sizeof(candidate.cloud_home_id));
  }
  if (doc.containsKey("cloud_owner_user_id")) {
    strlcpy(candidate.cloud_owner_user_id,
            doc["cloud_owner_user_id"] | candidate.cloud_owner_user_id,
            sizeof(candidate.cloud_owner_user_id));
  }
  if (doc.containsKey("ota_url")) {
    strlcpy(candidate.ota_url, doc["ota_url"] | candidate.ota_url, sizeof(candidate.ota_url));
  }
  if (doc.containsKey("ota_channel")) {
    strlcpy(candidate.ota_channel, doc["ota_channel"] | candidate.ota_channel, sizeof(candidate.ota_channel));
  }
}

void send_json_response(int code, const String &body) {
  g_server.sendHeader("Cache-Control", "no-store");
  g_server.send(code, "application/json", body);
}

String build_provisioning_info_json() {
  DynamicJsonDocument doc(768);
  doc["ok"] = true;
  doc["device_id"] = g_config->device_id;
  doc["pid"] = PRODUCT_PID;
  doc["sku"] = PRODUCT_SKU;
  doc["product_name"] = PRODUCT_NAME;
  doc["firmware_version"] = PRODUCT_FW_VERSION;
  doc["hardware_revision"] = PRODUCT_HW_REVISION;
  doc["ap_ssid"] = g_system->device_name;
  doc["ap_ip"] = g_system->ap_ip;
  doc["local_url"] = g_system->local_url;
  doc["ble_name"] = g_system->device_name;
  doc["provisioning_ready"] = true;
  JsonArray methods = doc.createNestedArray("provisioning_methods");
  methods.add("ap");
  methods.add("ble");

  String output;
  serializeJson(doc, output);
  return output;
}

void handle_get_provisioning_info() {
  send_json_response(200, build_provisioning_info_json());
}

void handle_post_provisioning_apply() {
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, g_server.arg("plain")) != DeserializationError::Ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  DeviceConfig candidate = *g_config;

  const char *ssid = doc["ssid"] | doc["wifi_ssid"] | nullptr;
  if (ssid != nullptr) {
    strlcpy(candidate.wifi_ssid, ssid, sizeof(candidate.wifi_ssid));
    candidate.wifi_mode = candidate.wifi_ssid[0] == '\0' ? TG_WIFI_MODE_AP_FALLBACK : TG_WIFI_MODE_STA;
  }

  if (doc.containsKey("password") || doc.containsKey("wifi_password")) {
    const char *wifi_password = doc["password"] | doc["wifi_password"] | "";
    strlcpy(candidate.wifi_password, wifi_password, sizeof(candidate.wifi_password));
  }
  if (doc.containsKey("wifi_tx_power_dbm_tenths")) {
    candidate.wifi_tx_power_dbm_tenths = doc["wifi_tx_power_dbm_tenths"];
  }

  if (doc.containsKey("device_id")) {
    strlcpy(candidate.device_id, doc["device_id"] | candidate.device_id, sizeof(candidate.device_id));
  }
  if (doc.containsKey("site_name")) {
    strlcpy(candidate.site_name, doc["site_name"] | candidate.site_name, sizeof(candidate.site_name));
  }
  if (doc.containsKey("tank_name")) {
    strlcpy(candidate.tank_name, doc["tank_name"] | candidate.tank_name, sizeof(candidate.tank_name));
  }
  if (doc.containsKey("display_name")) {
    strlcpy(candidate.tank_name, doc["display_name"] | candidate.tank_name, sizeof(candidate.tank_name));
  }
  if (doc.containsKey("mqtt_enabled")) candidate.mqtt_enabled = doc["mqtt_enabled"];
  if (doc.containsKey("mqtt_host")) {
    strlcpy(candidate.mqtt_host, doc["mqtt_host"] | candidate.mqtt_host, sizeof(candidate.mqtt_host));
  }
  if (doc.containsKey("mqtt_port")) candidate.mqtt_port = doc["mqtt_port"];
  if (doc.containsKey("mqtt_username")) {
    strlcpy(candidate.mqtt_username, doc["mqtt_username"] | candidate.mqtt_username,
            sizeof(candidate.mqtt_username));
  }
  if (doc.containsKey("mqtt_password")) {
    const char *mqtt_password = doc["mqtt_password"] | "";
    strlcpy(candidate.mqtt_password, mqtt_password, sizeof(candidate.mqtt_password));
  }
  if (doc.containsKey("cloud_base_url")) {
    strlcpy(candidate.cloud_base_url, doc["cloud_base_url"] | candidate.cloud_base_url,
            sizeof(candidate.cloud_base_url));
  }
  if (doc.containsKey("device_ingest_key")) {
    const char *device_ingest_key = doc["device_ingest_key"] | "";
    strlcpy(candidate.device_ingest_key, device_ingest_key, sizeof(candidate.device_ingest_key));
  }
  if (doc.containsKey("home_id") || doc.containsKey("cloud_home_id")) {
    strlcpy(candidate.cloud_home_id, doc["home_id"] | doc["cloud_home_id"] | candidate.cloud_home_id,
            sizeof(candidate.cloud_home_id));
  }
  if (doc.containsKey("owner_user_id") || doc.containsKey("cloud_owner_user_id")) {
    strlcpy(candidate.cloud_owner_user_id,
            doc["owner_user_id"] | doc["cloud_owner_user_id"] | candidate.cloud_owner_user_id,
            sizeof(candidate.cloud_owner_user_id));
  }

  config_manager_validate(candidate);
  if (!config_manager_save_config(candidate)) {
    send_json_response(500, "{\"ok\":false,\"error\":\"Failed to save provisioning config\"}");
    return;
  }

  *g_config = candidate;
  rf_output_apply_config(*g_config);

  DynamicJsonDocument response(384);
  response["ok"] = true;
  response["device_id"] = g_config->device_id;
  response["pid"] = PRODUCT_PID;
  response["wifi_mode"] = g_config->wifi_mode;
  response["cloud_configured"] = g_config->cloud_base_url[0] != '\0' && g_config->device_ingest_key[0] != '\0';
  String output;
  serializeJson(response, output);
  send_json_response(200, output);
}

void handle_get_root() {
  if (!session_valid()) {
    require_auth_redirect();
    return;
  }
  g_server.sendHeader("Cache-Control", "no-store");
  g_server.send(200, "text/html", FPSTR(EMBEDDED_INDEX_HTML));
}

void handle_get_login() {
  serve_login();
}

void handle_post_login() {
  const String password = g_server.arg("password");
  if (password.equals(g_config->ui_password)) {
    session_create();
    g_server.sendHeader("Set-Cookie", String("sess=") + g_session_token + "; Path=/; HttpOnly");
    g_server.sendHeader("Location", "/");
    g_server.send(302, "text/plain", "");
    return;
  }
  serve_login("Invalid password");
}

void handle_logout() {
  g_session_token[0] = '\0';
  g_server.sendHeader("Set-Cookie", "sess=; Path=/; Max-Age=0");
  g_server.sendHeader("Location", "/login");
  g_server.send(302, "text/plain", "");
}

void handle_get_status() {
  if (!ensure_api_auth()) return;
  send_json_response(200, build_status_json());
}

void handle_get_config() {
  if (!ensure_api_auth()) return;
  send_json_response(200, build_config_json());
}

void handle_post_config() {
  if (!ensure_api_auth()) return;

  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, g_server.arg("plain")) != DeserializationError::Ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  DeviceConfig candidate = *g_config;
  apply_config_patch_from_json(doc.as<JsonVariantConst>(), candidate);
  config_manager_validate(candidate);
  if (!config_manager_save_config(candidate)) {
    send_json_response(500, "{\"ok\":false,\"error\":\"Failed to save config\"}");
    return;
  }

  *g_config = candidate;
  rf_output_apply_config(*g_config);
  logger_log("INFO", "config", "CONFIG_CHANGED", "Configuration updated via WebUI");
  send_json_response(200, "{\"ok\":true}");
}

void handle_set_zero() {
  if (!ensure_api_auth()) return;
  const SensorSnapshot &sensor = sensor_dyp_get_snapshot();
  if (sensor.status != SENSOR_OK || sensor.filtered_distance_mm == 0) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Valid sensor reading required\"}");
    return;
  }
  g_config->zero_level_mm = sensor.filtered_distance_mm;
  config_manager_validate(*g_config);
  config_manager_save_config(*g_config);
  tank_logic_set_zero_reference(g_config->zero_level_mm);
  logger_log("INFO", "calibration", "ZERO_CALIBRATED", "Zero level calibrated from current sensor");
  send_json_response(200, "{\"ok\":true}");
}

void handle_manual_action() {
  if (!ensure_api_auth()) return;
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, g_server.arg("plain")) != DeserializationError::Ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  const char *action = doc["action"] | "";
  const bool override_flag = doc["override"] | false;
  bool ok = false;
  if (strcmp(action, "motor_on") == 0) {
    ok = tank_logic_manual_motor_on(*g_config, override_flag);
  } else if (strcmp(action, "motor_off") == 0) {
    ok = tank_logic_manual_motor_off(*g_config);
  } else if (strcmp(action, "alarm_test") == 0) {
    ok = tank_logic_manual_alarm_test(*g_config);
  }

  if (!ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Action failed\"}");
    return;
  }
  send_json_response(200, "{\"ok\":true}");
}

void handle_logs() {
  if (!ensure_api_auth()) return;
  const String format = g_server.arg("format");
  if (format == "csv") {
    g_server.send(200, "text/csv", logger_get_recent_csv(LOG_MAX_DOWNLOAD_ITEMS));
    return;
  }
  g_server.send(200, "application/json", logger_get_recent_json(LOG_MAX_DOWNLOAD_ITEMS));
}

void handle_factory_reset() {
  if (!ensure_api_auth()) return;
  RuntimePersist runtime = {};
  config_manager_apply_runtime_defaults(runtime);
  config_manager_factory_reset(*g_config, runtime);
  logger_log("WARN", "system", "FACTORY_RESET", "Factory reset triggered from WebUI");
  send_json_response(200, "{\"ok\":true,\"restarting\":true}");
  delay(200);
  ESP.restart();
}

void handle_restart() {
  if (!ensure_api_auth()) return;
  send_json_response(200, "{\"ok\":true,\"restarting\":true}");
  delay(200);
  ESP.restart();
}

void handle_ota_status() {
  if (!ensure_api_auth()) return;
  DynamicJsonDocument doc(256);
  doc["phase"] = ota_manager_phase();
  doc["message"] = ota_manager_message();
  doc["busy"] = ota_manager_is_busy();
  String output;
  serializeJson(doc, output);
  send_json_response(200, output);
}

void handle_cloud_ota() {
  if (!ensure_api_auth()) return;
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, g_server.arg("plain")) != DeserializationError::Ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  OtaRequest request = {};
  strlcpy(request.url, doc["url"] | g_config->ota_url, sizeof(request.url));
  strlcpy(request.version, doc["version"] | "", sizeof(request.version));
  strlcpy(request.checksum, doc["checksum"] | "", sizeof(request.checksum));

  char error[128] = {};
  if (!ota_manager_request_cloud_update(request, error, sizeof(error))) {
    DynamicJsonDocument err_doc(192);
    err_doc["ok"] = false;
    err_doc["error"] = error;
    String output;
    serializeJson(err_doc, output);
    send_json_response(400, output);
    return;
  }

  send_json_response(200, "{\"ok\":true}");
}

void handle_change_password() {
  if (!ensure_api_auth()) return;
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, g_server.arg("plain")) != DeserializationError::Ok) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }
  const char *current_password = doc["current_password"] | "";
  const char *new_password = doc["new_password"] | "";
  if (strcmp(current_password, g_config->ui_password) != 0) {
    send_json_response(400, "{\"ok\":false,\"error\":\"Current password incorrect\"}");
    return;
  }
  if (strlen(new_password) < 4 || strlen(new_password) > 31) {
    send_json_response(400, "{\"ok\":false,\"error\":\"New password must be 4-31 characters\"}");
    return;
  }
  strlcpy(g_config->ui_password, new_password, sizeof(g_config->ui_password));
  if (!config_manager_save_config(*g_config)) {
    send_json_response(500, "{\"ok\":false,\"error\":\"Failed to save password\"}");
    return;
  }
  logger_log("INFO", "auth", "PASSWORD_CHANGED", "Login password changed via WebUI");
  send_json_response(200, "{\"ok\":true}");
}

void handle_update_done() {
  if (!session_valid()) {
    g_server.send(403, "text/plain", "Forbidden");
    return;
  }
  char error[128] = {};
  if (ota_manager_finalize_local(error, sizeof(error))) {
    g_server.send(200, "text/plain", "OK: Firmware update verified. Device rebooting.");
    delay(500);
    ESP.restart();
  } else {
    g_server.send(500, "text/plain", String("ERROR: ") + error);
  }
}

void handle_update_upload() {
  if (!session_valid()) {
    ota_manager_abort_local("Forbidden");
    return;
  }
  HTTPUpload &upload = g_server.upload();
  char error[128] = {};
  if (upload.status == UPLOAD_FILE_START) {
    if (!ota_manager_begin_local(upload.totalSize, error, sizeof(error))) {
      ota_manager_abort_local(error);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    ota_manager_write_local(upload.buf, upload.currentSize, error, sizeof(error));
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    ota_manager_abort_local("Upload aborted");
  }
}

void handle_not_found() {
  if (!session_valid()) {
    require_auth_redirect();
    return;
  }
  g_server.send(404, "text/plain", "Not found");
}

}  // namespace

bool web_server_init(DeviceConfig *config, SystemStatus *system) {
  g_config = config;
  g_system = system;

  if (!MDNS.begin(system->mdns_name)) {
    logger_log("WARN", "network", "MDNS_FAIL", "mDNS startup failed");
  }

  const char *headers[] = {"Cookie"};
  g_server.collectHeaders(headers, 1);

  g_server.on("/", HTTP_GET, handle_get_root);
  g_server.on("/login", HTTP_GET, handle_get_login);
  g_server.on("/login", HTTP_POST, handle_post_login);
  g_server.on("/logout", HTTP_GET, handle_logout);
  g_server.on("/api/status", HTTP_GET, handle_get_status);
  g_server.on("/api/ping", HTTP_GET, []() { g_server.send(200, "text/plain", "pong"); });
  g_server.on("/api/provisioning/info", HTTP_GET, handle_get_provisioning_info);
  g_server.on("/api/provisioning/apply", HTTP_POST, handle_post_provisioning_apply);
  g_server.on("/api/config", HTTP_GET, handle_get_config);
  g_server.on("/api/config", HTTP_POST, handle_post_config);
  g_server.on("/api/set-zero", HTTP_POST, handle_set_zero);
  g_server.on("/api/manual", HTTP_POST, handle_manual_action);
  g_server.on("/api/logs", HTTP_GET, handle_logs);
  g_server.on("/api/factory-reset", HTTP_POST, handle_factory_reset);
  g_server.on("/api/restart", HTTP_POST, handle_restart);
  g_server.on("/api/ota/status", HTTP_GET, handle_ota_status);
  g_server.on("/api/ota/cloud", HTTP_POST, handle_cloud_ota);
  g_server.on("/api/change-password", HTTP_POST, handle_change_password);
  g_server.on("/update", HTTP_POST, handle_update_done, handle_update_upload);
  g_server.onNotFound(handle_not_found);
  g_server.begin();
  Serial.println("[HTTP] Web server started on port 80");
  return true;
}

void web_server_handle() {
  g_server.handleClient();
}
