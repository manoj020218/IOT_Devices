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
    :root{--bg:#f7f4ed;--card:#fffdf8;--ink:#16212f;--muted:#66748a;--line:#dfd5c4;--accent:#d64b2a;--accent2:#f3a33b;--good:#1f8a5b;--warn:#b36b00;--bad:#b82323;}
    *{box-sizing:border-box} body{margin:0;font-family:"Segoe UI",Tahoma,sans-serif;background:linear-gradient(180deg,#f4efe6,#fcfaf5 38%,#efe6d9);color:var(--ink)}
    header{padding:24px 18px 12px;background:radial-gradient(circle at top left,#ffd06a,#f3a33b 35%,#d64b2a 90%);color:#fff}
    header h1{margin:0 0 6px;font-size:1.8rem} header p{margin:0;max-width:720px;opacity:.92}
    main{padding:16px;display:grid;gap:14px;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
    .card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:16px;box-shadow:0 12px 24px rgba(34,38,44,.06)}
    .wide{grid-column:1/-1} h2{margin:0 0 12px;font-size:1.05rem} .grid{display:grid;gap:10px}
    .status{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:10px}
    .pill{display:inline-block;padding:6px 10px;border-radius:999px;background:#f1e7d7;color:#6b4f2d;font-size:.82rem;font-weight:600}
    .label{font-size:.8rem;color:var(--muted);margin-bottom:4px} .value{font-weight:700}
    button,.buttonlike{border:none;border-radius:12px;padding:12px 14px;font-weight:700;background:var(--ink);color:#fff;cursor:pointer}
    button.alt{background:#ebe2d4;color:var(--ink)} button.warn{background:var(--accent2);color:#2f2514} button.bad{background:var(--bad)}
    button.good{background:var(--good)} button:disabled{opacity:.55;cursor:not-allowed}
    .row{display:flex;gap:10px;flex-wrap:wrap;align-items:center}
    input,select,textarea{width:100%;border:1px solid var(--line);background:#fff;padding:11px 12px;border-radius:12px;font:inherit}
    textarea{min-height:120px;resize:vertical}
    .profiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(230px,1fr));gap:12px}
    .profile-group{display:grid;gap:10px;margin-top:14px}.profile-group h3{margin:0;font-size:1rem}
    .profile{padding:14px;border:1px solid var(--line);border-radius:16px;background:#fff}
    .profile.selected{border-color:var(--accent);box-shadow:0 0 0 2px rgba(214,75,42,.13)}
    .profile h3{margin:0 0 6px;font-size:1rem} .meta{font-size:.84rem;color:var(--muted);min-height:36px}
    .sos{width:100%;padding:18px;background:linear-gradient(135deg,#ff6b57,#b82323);font-size:1.1rem}
    .tiny{font-size:.84rem;color:var(--muted)} progress{width:100%;height:12px}
    .notice{display:grid;gap:10px;padding:14px;border:1px solid #f0c878;background:#fff3d8;border-radius:16px}
    .notice strong{font-size:.95rem}.notice a{color:var(--ink);font-weight:700;word-break:break-all}
    @media (max-width:700px){header{padding:18px 14px 12px}main{padding:12px}}
  </style>
</head>
<body>
  <header>
    <h1>Jenix Loud SOS Siren</h1>
    <p>Field-test controller for ESP32-C3 with AP/STA access, persisted tone selection, OTA update, and safety duty cycling.</p>
  </header>
  <main>
    <section class="card wide" id="networkCard" style="display:none">
      <h2>Network Access</h2>
      <div class="notice" id="networkNotice"></div>
    </section>

    <section class="card wide">
      <h2>Dashboard</h2>
      <div class="status" id="statusGrid"></div>
    </section>

    <section class="card wide">
      <h2>Tone Test / Selection</h2>
      <div id="profiles"></div>
      <div class="row" style="margin-top:12px">
        <button class="good" onclick="saveSelectedTone()">Save Selected Tone</button>
        <button onclick="testTone(500)">Test 500Hz</button>
        <button onclick="testTone(1000)">Test 1000Hz</button>
        <button onclick="testTone(1500)">Test 1500Hz</button>
        <button class="alt" onclick="runSweepTest()">Run Sweep Test</button>
        <button class="warn" onclick="runBenchTest()">Run Bench Test</button>
      </div>
      <div class="tiny" style="margin-top:10px">The saved tone profile is stored in NVS and is used by START SOS, the push button, and the VT trigger after power loss.</div>
    </section>

    <section class="card">
      <h2>SOS Control</h2>
      <div class="grid">
        <button class="sos" onclick="startSos()">START SOS</button>
        <button class="alt" onclick="postJson('/api/stop',{})">Stop Siren</button>
      </div>
    </section>

    <section class="card">
      <h2>VT Trigger SOS</h2>
      <div class="grid">
        <label><span class="label">GPIO3 VT Trigger</span><select id="vtTriggerEnabled"><option value="true">Enabled</option><option value="false">Disabled</option></select></label>
        <label><span class="label">VT Mode</span><select id="vtTriggerMode"><option value="inching">Inching: ON while VT stays HIGH</option><option value="timed">Timed: fixed duration per trigger</option><option value="toggle">Toggle: first trigger ON, second trigger OFF</option></select></label>
        <div class="tiny">VT trigger uses the saved tone profile shown in the Tone Selection section.</div>
        <div class="tiny">Timed mode uses the duration and retrigger settings below. Toggle mode stays active until the next VT fire. Inching mode follows the VT HIGH level and stops on VT LOW, while still obeying the horn cooling protection.</div>
        <label><span class="label">VT Siren Duration Seconds</span><input id="vtTriggerDurationSec" type="number" min="10" max="1800"></label>
        <label><span class="label">Retrigger Mode</span><select id="vtRetriggerMode"><option value="extend">Extend Timer</option><option value="restart">Restart Timer</option></select></label>
        <label><span class="label">Cloud Notification</span><select id="vtCloudNotify"><option value="false">Disabled</option><option value="true">Enabled</option></select></label>
        <div class="row">
          <button class="good" onclick="saveVtSettings()">Save VT Settings</button>
          <button class="warn" onclick="restoreVtDefaults()">Default VT Settings</button>
        </div>
        <div class="tiny">Save VT Settings stores only the VT trigger configuration to NVS so it remains after power cut.</div>
      </div>
    </section>

    <section class="card">
      <h2>Speaker / Safety Settings</h2>
      <div class="grid">
        <div class="notice">
          <strong>Maximum loudness stays fixed.</strong>
          <div class="tiny">These settings change tone behavior and thermal safety, not the output volume. Lower sweep minimum tends to travel farther outdoors, higher sweep maximum cuts through machine noise, shorter sweep step sounds more urgent, longer ON time gives more coverage but heats the horn longer, longer cooling pause protects the speaker and MOSFET, and burst OFF gaps reduce heat while keeping attention.</div>
        </div>
        <label><span class="label">Speaker Profile</span><select id="speakerProfileId"></select></label>
        <label><span class="label">Installer / Admin Password</span><input id="speakerProfilePassword" type="password" placeholder="admin123"></label>
        <div class="tiny">Speaker profile changes require the admin password. Default password is <strong>admin123</strong> unless you changed the OTA Admin Password below.</div>
        <button onclick="saveSpeakerProfile()">Apply Speaker Profile</button>
        <div class="tiny">Applying a speaker profile loads safe defaults for ON time, cooling pause, sweep range, and burst timing for that horn model.</div>
        <label><span class="label">Sweep Minimum Hz</span><input id="sweepMinHz" type="number" min="300" max="1999"></label>
        <label><span class="label">Sweep Maximum Hz</span><input id="sweepMaxHz" type="number" min="301" max="2000"></label>
        <label><span class="label">Sweep Step Interval ms</span><input id="sweepStepMs" type="number" min="5" max="100"></label>
        <label><span class="label">Maximum ON Duration Seconds</span><input id="onDurationSec" type="number" min="1" max="30"></label>
        <label><span class="label">Cooling Pause Seconds</span><input id="restDurationSec" type="number" min="5" max="30"></label>
        <label><span class="label">Burst ON ms</span><input id="burstOnMs" type="number" min="0" max="5000"></label>
        <label><span class="label">Burst OFF ms</span><input id="burstOffMs" type="number" min="0" max="5000"></label>
        <label><span class="label">Burst Cycle Limit</span><input id="burstCycleLimit" type="number" min="0" max="30"></label>
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
        <div class="row">
          <button onclick="scanWifi()">Scan Networks</button>
          <span class="tiny" id="wifiHint"></span>
        </div>
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
    const statusGrid=document.getElementById('statusGrid'), profilesEl=document.getElementById('profiles');
    const networkCard=document.getElementById('networkCard'), networkNotice=document.getElementById('networkNotice');
    let profiles=[], speakerProfiles=[], statusData={}, pendingProfileId=0;
    const fields=['onDurationSec','restDurationSec','sweepMinHz','sweepMaxHz','sweepStepMs','burstOnMs','burstOffMs','burstCycleLimit','vtTriggerEnabled','vtTriggerMode','vtTriggerDurationSec','vtRetriggerMode','vtCloudNotify','otaAdminPassword'];
    const profileGroups=[
      {title:'Emergency Profiles', ids:[1,2,3,4,9]},
      {title:'Routine Profiles', ids:[5,6,7,8]},
      {title:'Status / Test Profiles', ids:[10]}
    ];

    async function getJson(url){const r=await fetch(url); if(!r.ok) throw new Error(await r.text()); return r.json();}
    async function postJson(url,payload){const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload||{})}); const text=await r.text(); if(!r.ok) throw new Error(text||r.statusText); if(text){try{return JSON.parse(text)}catch{return text}} return null;}
    function vtAgeLabel(){
      if(!statusData.vtTriggerSeen) return 'Never';
      const age=Math.max(0,(statusData.uptimeSec||0)-(statusData.vtLastTriggerUptimeSec||0));
      return `${age}s ago`;
    }
    function renderStatus(){
      const items=[
        ['Device',statusData.deviceName],['PID',statusData.pid],['Firmware',statusData.firmwareVersion],['Wi-Fi Mode',statusData.wifiMode],
        ['Station IP',statusData.ipAddress||'-'],['AP IP',statusData.apIpAddress||'-'],['Selected Tone',statusData.selectedTone],['State',statusData.sirenState],
        ['Speaker',statusData.speakerProfile||'-'],['Remaining',statusData.remainingLabel],['Elapsed ON',`${Math.floor((statusData.elapsedOnMs||0)/1000)} sec`],
        ['Cooling',statusData.coolingRemainingLabel||'00:00'],['PWM Duty',statusData.activeDutyPercent+'%'],['Frequency',statusData.activeFrequencyHz?statusData.activeFrequencyHz+' Hz':'-'],['Button',statusData.buttonPressed?'PRESSED':'IDLE'],
        ['VT Input',statusData.vtTriggerHigh?'HIGH':'LOW'],['VT Mode',(statusData.vtTriggerMode||'-').toUpperCase()],['VT Control',statusData.vtControlLatched?'LATCHED':'IDLE'],['VT Tone',statusData.vtTriggerProfileName||`#${statusData.vtTriggerProfileId||0}`],['VT Duration',`${statusData.vtTriggerDurationSec||0} sec`],
        ['VT Last Trigger',statusData.vtLastTriggerLabel||'Never'],['VT Age',vtAgeLabel()],['Press Count',String(statusData.sosPressCount||0)],
        ['Retrigger',statusData.vtRetriggerMode||'-'],['Cloud Notify',statusData.vtCloudNotify?'ON':'OFF'],['Last Stop',statusData.lastStopReason||'-'],['mDNS URL',statusData.mdnsUrl||'-']
      ];
      statusGrid.innerHTML=items.map(([k,v])=>`<div><div class="label">${k}</div><div class="value">${v}</div></div>`).join('');
    }
    function renderNetworkNotice(){
      const currentHost=(location.hostname||'').toLowerCase();
      const mdnsHost=(statusData.mdnsHost||'').toLowerCase();
      const stationIp=(statusData.ipAddress||'').toLowerCase();
      const onStationIp=stationIp && currentHost===stationIp;
      const onApIp=statusData.apIpAddress && currentHost===statusData.apIpAddress.toLowerCase();
      const onMdns=mdnsHost && currentHost===mdnsHost;
      const preferred=statusData.preferredUrl||statusData.mdnsUrl||statusData.apUrl||'';

      if(!statusData.staConnected){
        networkCard.style.display='block';
        networkNotice.innerHTML=`<strong>AP mode is active.</strong><div class="tiny">Use the fallback AP URL below while no router connection is active.</div><a href="${statusData.apUrl}">${statusData.apUrl}</a>`;
        return;
      }

      networkCard.style.display='block';
      if(onMdns){
        networkNotice.innerHTML=`<strong>Using the preferred hostname.</strong><div class="tiny">This device is reachable on the router network through mDNS.</div><a href="${statusData.mdnsUrl}">${statusData.mdnsUrl}</a>`;
        return;
      }

      networkNotice.innerHTML=`<strong>Preferred browser URL</strong><div class="tiny">Once station Wi-Fi is connected, use the .local hostname for normal browser access on phones and other clients.</div><a href="${statusData.mdnsUrl}">${statusData.mdnsUrl}</a><div class="row"><button onclick="openPreferredUrl()">Open .local URL</button></div>`;

      if(onStationIp && !sessionStorage.getItem('mdnsRedirectAttempted')){
        sessionStorage.setItem('mdnsRedirectAttempted','1');
        setTimeout(()=>window.location.replace(preferred),1800);
      } else if(onApIp) {
        sessionStorage.removeItem('mdnsRedirectAttempted');
      }
    }
    function openPreferredUrl(){ if(statusData.mdnsUrl) window.location.assign(statusData.mdnsUrl); }
    function renderProfileCard(p, chosenId){
      return `<article class="profile ${chosenId===p.id?'selected':''}">
        <span class="pill">#${p.id}</span><h3>${p.name}</h3><div class="meta">${p.description}</div>
        <div class="tiny">${p.frequencyPattern}</div><div class="tiny">${p.recommendedUse}</div><div class="tiny">Full-cycle preview: ${p.testDurationSec||15}s</div>
        <div class="row" style="margin-top:10px"><button onclick="testProfile(${p.id})">Play Test</button><button class="alt" onclick="chooseProfile(${p.id})">Choose</button></div>
      </article>`;
    }
    function renderProfiles(){
      const chosenId=pendingProfileId||statusData.selectedProfileId;
      profilesEl.innerHTML=profileGroups.map(group=>{
        const items=profiles.filter(p=>group.ids.includes(p.id));
        return `<div class="profile-group"><h3>${group.title}</h3><div class="profiles">${items.map(p=>renderProfileCard(p, chosenId)).join('')}</div></div>`;
      }).join('');
    }
    function fillSettings(s){
      const speakerSelect=document.getElementById('speakerProfileId');
      speakerSelect.innerHTML=speakerProfiles.map(p=>`<option value="${p.id}">${p.name}</option>`).join('');
      fields.forEach(id=>document.getElementById(id).value=String(s[id]));
      speakerSelect.value=String(s.speakerProfileId||1);
      document.getElementById('otaUploadPassword').value=s.otaAdminPassword||'';
    }
    async function refresh(){try{statusData=await getJson('/api/status'); if(!pendingProfileId) pendingProfileId=statusData.selectedProfileId||1; renderStatus(); renderNetworkNotice(); profiles=await getJson('/api/profiles'); speakerProfiles=await getJson('/api/speaker-profiles'); renderProfiles(); fillSettings(await getJson('/api/settings'));}catch(e){console.error(e);}}
    function chooseProfile(id){pendingProfileId=id; renderProfiles();}
    async function saveSelectedTone(){await postJson('/api/select-profile',{id:pendingProfileId||statusData.selectedProfileId||1}); pendingProfileId=0; await refresh(); alert('Selected tone saved to NVS.');}
    async function testProfile(id){const profile=profiles.find(p=>p.id===id); await postJson('/api/test-profile',{id,durationSec:(profile&&profile.testDurationSec)||15}); await refresh();}
    async function testTone(frequencyHz){await postJson('/api/test-tone',{frequencyHz}); await refresh();}
    async function runSweepTest(){await postJson('/api/test-sweep',{}); await refresh();}
    async function runBenchTest(){await postJson('/api/bench-test',{}); await refresh();}
    async function startSos(){await postJson('/api/sos',{}); await refresh();}
    async function saveVtSettings(){
      const payload={
        vtTriggerEnabled:document.getElementById('vtTriggerEnabled').value==='true',
        vtTriggerMode:document.getElementById('vtTriggerMode').value,
        vtTriggerDurationSec:Number(document.getElementById('vtTriggerDurationSec').value),
        vtRetriggerMode:document.getElementById('vtRetriggerMode').value,
        vtCloudNotify:document.getElementById('vtCloudNotify').value==='true'
      };
      await postJson('/api/settings/vt',payload);
      await refresh();
      alert('VT trigger settings saved to NVS.');
    }
    async function restoreVtDefaults(){await postJson('/api/settings/vt-default',{}); await refresh(); alert('VT trigger settings restored to default values.');}
    async function saveSettings(){const payload={}; fields.forEach(id=>payload[id]=document.getElementById(id).value); payload.vtTriggerEnabled=payload.vtTriggerEnabled==='true'; payload.vtCloudNotify=payload.vtCloudNotify==='true'; payload.vtTriggerDurationSec=Number(payload.vtTriggerDurationSec); payload.onDurationSec=Number(payload.onDurationSec); payload.restDurationSec=Number(payload.restDurationSec); payload.sweepMinHz=Number(payload.sweepMinHz); payload.sweepMaxHz=Number(payload.sweepMaxHz); payload.sweepStepMs=Number(payload.sweepStepMs); payload.burstOnMs=Number(payload.burstOnMs); payload.burstOffMs=Number(payload.burstOffMs); payload.burstCycleLimit=Number(payload.burstCycleLimit); await postJson('/api/settings',payload); await refresh(); alert('Speaker and safety settings saved to NVS.');}
    async function saveSpeakerProfile(){
      const password=document.getElementById('speakerProfilePassword').value||document.getElementById('otaAdminPassword').value||'admin123';
      const id=Number(document.getElementById('speakerProfileId').value);
      const r=await fetch('/api/speaker-profile',{method:'POST',headers:{'Content-Type':'application/json','X-Admin-Password':password},body:JSON.stringify({id})});
      const text=await r.text();
      if(!r.ok) throw new Error((text||r.statusText)+' Use admin password, default admin123 unless changed.');
      alert('Speaker profile saved to NVS.');
      await refresh();
    }
    async function restoreDefaults(){await postJson('/api/settings/default',{}); await refresh();}
    async function scanWifi(){
      document.getElementById('wifiHint').textContent='Scanning...';
      const networks=await getJson('/api/wifi/scan');
      const select=document.getElementById('wifiSsid');
      select.innerHTML=networks.map(n=>`<option value="${n.ssid}">${n.ssid || '(hidden)'} (${n.rssi} dBm)</option>`).join('');
      document.getElementById('wifiHint').textContent=networks.length?`${networks.length} network(s) found`:'No networks found';
    }
    async function saveWifi(){await postJson('/api/wifi/save',{ssid:document.getElementById('wifiSsid').value,password:document.getElementById('wifiPassword').value}); alert('Wi-Fi settings saved.');}
    async function uploadFirmware(){
      const file=document.getElementById('otaFile').files[0]; if(!file) return alert('Select a firmware .bin file first.');
      const xhr=new XMLHttpRequest(), progress=document.getElementById('otaProgress'), data=new FormData();
      data.append('firmware',file); xhr.open('POST','/update'); xhr.setRequestHeader('X-Admin-Password',document.getElementById('otaUploadPassword').value);
      xhr.upload.onprogress=e=>{if(e.lengthComputable) progress.value=(e.loaded/e.total)*100;};
      xhr.onload=()=>alert(xhr.responseText||'Upload finished'); xhr.onerror=()=>alert('Upload failed'); xhr.send(data);
    }
    async function importSettings(){await postJson('/api/settings/import',{json:document.getElementById('importJson').value}); await refresh();}
    async function factoryReset(){if(confirm('Factory reset all settings and Wi-Fi credentials?')) await postJson('/api/factory-reset',{});}
    setInterval(refresh,3000); refresh(); scanWifi().catch(()=>{});
  </script>
</body>
</html>
)HTML";
}  // namespace WebAssets
