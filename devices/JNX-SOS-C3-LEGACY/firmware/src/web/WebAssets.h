#pragma once

#include <Arduino.h>

namespace WebAssets {
constexpr char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Jenix Loud SOS Siren</title>
  <style>
    :root{--bg:#f5f1e7;--card:#fffdf8;--ink:#18222f;--muted:#68768a;--line:#ddd2c0;--accent:#cd4527;--good:#1e8657;--warn:#d9a33d;--bad:#b52121}
    *{box-sizing:border-box} body{margin:0;font-family:"Segoe UI",Tahoma,sans-serif;background:linear-gradient(180deg,#efe7d7,#faf8f1);color:var(--ink)}
    header{padding:22px 16px;background:linear-gradient(135deg,#f7c55a,#e68d2c 40%,#cd4527);color:#fff}
    header h1{margin:0 0 6px;font-size:1.7rem} header p{margin:0;max-width:760px;opacity:.92}
    main{padding:16px;display:grid;gap:14px;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
    .card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:16px;box-shadow:0 12px 24px rgba(29,36,44,.06)}
    .wide{grid-column:1/-1} h2{margin:0 0 12px;font-size:1.02rem}
    .grid{display:grid;gap:10px}.row{display:flex;gap:10px;flex-wrap:wrap;align-items:center}
    .status{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px}
    .tile{padding:12px;border:1px solid var(--line);border-radius:14px;background:#fff}
    .label{font-size:.8rem;color:var(--muted);margin-bottom:4px}.value{font-weight:700}
    button,a.buttonlike{border:none;border-radius:12px;padding:12px 14px;font-weight:700;background:var(--ink);color:#fff;cursor:pointer;text-decoration:none;display:inline-block}
    button.alt{background:#ebe2d4;color:var(--ink)} button.good{background:var(--good)} button.warn{background:var(--warn);color:#33220a} button.bad{background:var(--bad)}
    input,select,textarea{width:100%;border:1px solid var(--line);background:#fff;padding:11px 12px;border-radius:12px;font:inherit}
    textarea{min-height:120px;resize:vertical}.profiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:12px}
    .profile{padding:14px;border:1px solid var(--line);border-radius:16px;background:#fff}.profile.selected{border-color:var(--accent);box-shadow:0 0 0 2px rgba(205,69,39,.12)}
    .profile h3{margin:0 0 6px;font-size:1rem}.meta{font-size:.84rem;color:var(--muted);min-height:38px}.tiny{font-size:.84rem;color:var(--muted)}
    progress{width:100%;height:12px}.sos{width:100%;padding:18px;background:linear-gradient(135deg,#ff6f57,#b52121);font-size:1.08rem}
  </style>
</head>
<body>
  <header>
    <h1>Jenix Loud SOS Siren</h1>
    <p>Legacy comparison build with persisted tone selection, VT trigger control, Wi-Fi setup, and OTA update.</p>
  </header>
  <main>
    <section class="card wide">
      <h2>Dashboard</h2>
      <div class="status" id="statusGrid"></div>
    </section>

    <section class="card wide">
      <h2>Tone Profiles</h2>
      <div class="profiles" id="profiles"></div>
    </section>

    <section class="card">
      <h2>SOS Control</h2>
      <div class="grid">
        <button class="sos" onclick="startSos()">START SOS</button>
        <button class="alt" onclick="postJson('/api/stop',{})">Stop Siren</button>
      </div>
    </section>

    <section class="card">
      <h2>VT Trigger</h2>
      <div class="grid">
        <label><span class="label">GPIO3 VT Trigger</span><select id="vtTriggerEnabled"><option value="true">Enabled</option><option value="false">Disabled</option></select></label>
        <label><span class="label">VT Tone Profile</span><select id="vtTriggerProfileId"></select></label>
        <label><span class="label">VT Duration Seconds</span><input id="vtTriggerDurationSec" type="number" min="10" max="1800"></label>
        <label><span class="label">Retrigger Mode</span><select id="vtRetriggerMode"><option value="extend">Extend Timer</option><option value="restart">Restart Timer</option></select></label>
        <label><span class="label">Cloud Notification</span><select id="vtCloudNotify"><option value="false">Disabled</option><option value="true">Enabled</option></select></label>
      </div>
    </section>

    <section class="card">
      <h2>PWM / Safety Settings</h2>
      <div class="grid">
        <label><span class="label">Input Voltage Profile</span><select id="inputVoltageProfile"><option value="24V">24V</option><option value="30V">30V</option></select></label>
        <label><span class="label">Normal Duty Percent</span><input id="normalDutyPercent" type="number" min="5" max="95"></label>
        <label><span class="label">Boost Duty Percent</span><input id="boostDutyPercent" type="number" min="5" max="95"></label>
        <label><span class="label">Boost Duration Seconds</span><input id="boostDurationSec" type="number" min="0" max="120"></label>
        <label><span class="label">Maximum Duty Percent</span><input id="maxDutyPercent" type="number" min="5" max="95"></label>
        <label><span class="label">ON Duration Seconds</span><input id="onDurationSec" type="number" min="10" max="1800"></label>
        <label><span class="label">REST Duration Seconds</span><input id="restDurationSec" type="number" min="5" max="600"></label>
        <label><span class="label">Long Run Mode</span><select id="longRunMode"><option value="true">Enabled</option><option value="false">Disabled</option></select></label>
        <label><span class="label">OTA Admin Password</span><input id="otaAdminPassword" type="password"></label>
        <div class="row">
          <button class="good" onclick="saveSettings()">Save Settings</button>
          <button class="warn" onclick="restoreDefaults()">Restore Defaults</button>
        </div>
      </div>
    </section>

    <section class="card">
      <h2>Wi-Fi Settings</h2>
      <div class="grid">
        <div class="row"><button onclick="scanWifi()">Scan Networks</button><span class="tiny" id="wifiHint"></span></div>
        <label><span class="label">SSID</span><select id="wifiSsid"></select></label>
        <label><span class="label">Password</span><input id="wifiPassword" type="password"></label>
        <button class="good" onclick="saveWifi()">Save and Reconnect</button>
      </div>
    </section>

    <section class="card">
      <h2>Firmware OTA Update</h2>
      <div class="grid">
        <label><span class="label">OTA Password</span><input id="otaUploadPassword" type="password"></label>
        <input id="otaFile" type="file" accept=".bin,application/octet-stream">
        <progress id="otaProgress" value="0" max="100"></progress>
        <button class="good" onclick="uploadFirmware()">Upload Firmware</button>
      </div>
    </section>

    <section class="card wide">
      <h2>System</h2>
      <div class="grid">
        <div class="row">
          <button onclick="postJson('/api/reboot',{})">Reboot</button>
          <button class="bad" onclick="factoryReset()">Factory Reset</button>
          <a class="buttonlike" href="/api/settings/export" target="_blank">Export Settings JSON</a>
        </div>
        <label><span class="label">Import Settings JSON</span><textarea id="importJson" placeholder='{"selectedProfileId":2}'></textarea></label>
        <button class="warn" onclick="importSettings()">Import Settings</button>
      </div>
    </section>
  </main>
  <script>
    const statusGrid=document.getElementById('statusGrid');
    const profilesEl=document.getElementById('profiles');
    const fields=['inputVoltageProfile','normalDutyPercent','boostDutyPercent','boostDurationSec','maxDutyPercent','onDurationSec','restDurationSec','longRunMode','vtTriggerEnabled','vtTriggerProfileId','vtTriggerDurationSec','vtRetriggerMode','vtCloudNotify','otaAdminPassword'];
    let profiles=[];

    async function getJson(url){const r=await fetch(url); if(!r.ok) throw new Error(await r.text()); return r.json();}
    async function postJson(url,payload){const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload||{})}); const t=await r.text(); if(!r.ok) throw new Error(t||r.statusText); if(t){try{return JSON.parse(t)}catch{return t}} return null;}

    function renderStatus(status){
      const items=[
        ['Tone',status.selectedTone||'-'],['State',status.sirenState||'-'],['Active',status.activeTone||'-'],
        ['Frequency',`${status.activeFrequencyHz||0} Hz`],['Duty',`${status.activeDutyPercent||0}%`],['Remaining',status.remainingLabel||'00:00'],
        ['Wi-Fi',status.wifiMode||'-'],['IP',status.ipAddress||'-'],['AP IP',status.apIpAddress||'-'],
        ['SSID',status.connectedSsid||'-'],['VT Input',status.vtTriggerHigh?'HIGH':'LOW'],['SOS Presses',String(status.sosPressCount||0)]
      ];
      statusGrid.innerHTML=items.map(([label,value])=>`<div class="tile"><div class="label">${label}</div><div class="value">${value}</div></div>`).join('');
    }

    function renderProfiles(selectedId){
      profilesEl.innerHTML=profiles.map(profile=>{
        const selected=profile.id===selectedId?'selected':'';
        return `<article class="profile ${selected}">
          <h3>${profile.name}</h3>
          <div class="meta">${profile.description}</div>
          <div class="tiny">${profile.frequencyPattern}</div>
          <div class="tiny">${profile.dutyProfile}</div>
          <div class="row" style="margin-top:10px">
            <button onclick="selectTone(${profile.id})">Select</button>
            <button class="alt" onclick="testTone(${profile.id})">Test</button>
          </div>
        </article>`;
      }).join('');
    }

    function fillSettings(settings){
      fields.forEach(id=>{const el=document.getElementById(id); if(el&&settings[id]!==undefined){el.value=String(settings[id]);}});
      const vtSelect=document.getElementById('vtTriggerProfileId');
      vtSelect.innerHTML=profiles.map(p=>`<option value="${p.id}">${p.name}</option>`).join('');
      vtSelect.value=String(settings.vtTriggerProfileId||9);
      const wifi=document.getElementById('wifiSsid');
      if(!wifi.options.length&&settings.wifiSsid){wifi.innerHTML=`<option value="${settings.wifiSsid}">${settings.wifiSsid}</option>`;}
      document.getElementById('wifiPassword').value=settings.wifiPassword||'';
    }

    async function refresh(){
      const [status,loadedProfiles,settings]=await Promise.all([getJson('/api/status'),getJson('/api/profiles'),getJson('/api/settings')]);
      profiles=loadedProfiles;
      renderStatus(status);
      renderProfiles(status.selectedProfileId);
      fillSettings(settings);
    }

    async function selectTone(id){await postJson('/api/select-profile',{id}); await refresh();}
    async function testTone(id){await postJson('/api/test-profile',{id,durationSec:10});}
    async function startSos(){await postJson('/api/sos',{}); await refresh();}

    async function saveSettings(){
      const payload={};
      fields.forEach(id=>payload[id]=document.getElementById(id).value);
      ['normalDutyPercent','boostDutyPercent','boostDurationSec','maxDutyPercent','onDurationSec','restDurationSec','vtTriggerProfileId','vtTriggerDurationSec'].forEach(id=>payload[id]=Number(payload[id]));
      ['longRunMode','vtTriggerEnabled','vtCloudNotify'].forEach(id=>payload[id]=payload[id]==='true');
      await postJson('/api/settings',payload);
      await refresh();
    }

    async function restoreDefaults(){await postJson('/api/settings/default',{}); await refresh();}
    async function importSettings(){await postJson('/api/settings/import',{json:document.getElementById('importJson').value}); await refresh();}
    async function saveWifi(){await postJson('/api/wifi/save',{ssid:document.getElementById('wifiSsid').value,password:document.getElementById('wifiPassword').value});}

    async function scanWifi(){
      const items=await getJson('/api/wifi/scan');
      const select=document.getElementById('wifiSsid');
      select.innerHTML=items.map(item=>`<option value="${item.ssid}">${item.ssid} (${item.rssi} dBm)</option>`).join('');
      document.getElementById('wifiHint').textContent=`${items.length} network(s) found`;
    }

    async function factoryReset(){
      if(!confirm('Factory reset and reboot?')) return;
      await postJson('/api/factory-reset',{});
    }

    async function uploadFirmware(){
      const file=document.getElementById('otaFile').files[0];
      if(!file){alert('Select a firmware file first.'); return;}
      const form=new FormData();
      form.append('update',file);
      const request=new XMLHttpRequest();
      request.open('POST','/update');
      request.setRequestHeader('X-Admin-Password',document.getElementById('otaUploadPassword').value);
      request.upload.onprogress=event=>{
        if(event.lengthComputable){document.getElementById('otaProgress').value=Math.round((event.loaded/event.total)*100);}
      };
      request.onload=()=>alert(request.responseText||'Upload complete');
      request.onerror=()=>alert('OTA upload failed');
      request.send(form);
    }

    refresh();
    setInterval(refresh,2000);
  </script>
</body>
</html>
)HTML";
}  // namespace WebAssets
