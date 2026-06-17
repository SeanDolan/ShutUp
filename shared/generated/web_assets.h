#pragma once

namespace shutup {

static constexpr const char kConfigCabHtml[] = R"SHUTUP_HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ShutUp Cab Config</title>
  <style>
    :root { color-scheme: light; --ink:#172126; --muted:#5e6b73; --line:#cdd6dc; --panel:#f7f9fa; --accent:#0b6bcb; --good:#168447; --warn:#a35a00; }
    * { box-sizing: border-box; }
    body { margin: 0; font-family: Arial, Helvetica, sans-serif; color: var(--ink); background: #e9eef2; }
    header { padding: 20px 18px 12px; background: #ffffff; border-bottom: 1px solid var(--line); }
    h1 { margin: 0; font-size: 24px; }
    main { max-width: 860px; margin: 0 auto; padding: 18px; }
    section { background: #fff; border: 1px solid var(--line); border-radius: 6px; margin-bottom: 14px; padding: 16px; }
    h2 { margin: 0 0 12px; font-size: 18px; }
    label { display: block; font-weight: 700; margin: 12px 0 6px; }
    input[type="text"] { width: 100%; padding: 10px; border: 1px solid var(--line); border-radius: 4px; font-size: 16px; }
    button { border: 0; border-radius: 4px; padding: 10px 14px; background: var(--accent); color: #fff; font-weight: 700; font-size: 15px; cursor: pointer; }
    button.secondary { background: #56636b; }
    button.danger { background: #9c2f2f; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(210px, 1fr)); gap: 10px; }
    .status { background: var(--panel); border: 1px solid var(--line); border-radius: 4px; padding: 10px; min-height: 42px; }
    .muted { color: var(--muted); }
    .row { display: flex; gap: 10px; flex-wrap: wrap; align-items: center; }
    .kv { margin: 6px 0; }
    .ok { color: var(--good); font-weight: 700; }
    .warn { color: var(--warn); font-weight: 700; }
  </style>
</head>
<body>
  <header>
    <h1>ShutUp Cab Config</h1>
    <div class="muted">SSID: SHUTUP-CABCONF</div>
  </header>
  <main>
    <section>
      <h2>Device</h2>
      <label for="deviceName">Device name</label>
      <input id="deviceName" name="deviceName" type="text" maxlength="31" value="ShutUp Cab">
      <div class="grid">
        <div class="status"><div class="muted">Local MAC</div><div id="localMac">Loading...</div></div>
        <div class="status"><div class="muted">Paired peer</div><div id="peerMac">Loading...</div></div>
      </div>
      <p class="muted">Normal operation uses ESP-NOW. Wi-Fi is only used for this configuration mode.</p>
      <div class="row">
        <button id="saveBtn">Save config</button>
        <button id="rebootBtn" class="secondary">Reboot</button>
      </div>
    </section>

    <section>
      <h2>Pair Devices</h2>
      <p>Put both devices into config mode. Connect your phone to either config Wi-Fi network, open that config page, then press Pair Devices. The other device does not need its page open.</p>
      <div class="status" id="pairStatus">Pairing idle.</div>
      <div class="row" style="margin-top:12px">
        <button id="pairBtn">Pair Devices</button>
      </div>
    </section>
  </main>

  <script>
    const demoConfig = {
      role: "Cab",
      deviceName: "ShutUp Cab",
      localMac: "24:6F:28:CA:B0:01",
      hasPeer: true,
      peerMac: "34:85:18:CA:10:02",
      doorEnabledMask: 0
    };

    const $ = (id) => document.getElementById(id);

    function render(config) {
      $("deviceName").value = config.deviceName || "ShutUp Cab";
      $("localMac").textContent = config.localMac || "Unknown";
      $("peerMac").innerHTML = config.hasPeer ? `<span class="ok">${config.peerMac}</span>` : `<span class="warn">Not paired</span>`;
    }

    function updatePairStatus(status) {
      const paired = status.hasPeer ? `Paired peer: ${status.peerMac}` : "No peer stored";
      $("pairStatus").textContent = `${status.message || "Pairing status"} - ${paired}`;
    }
    const shouldPollPairStatus = true;

    async function api(path, options) {
      const response = await fetch(path, options);
      if (!response.ok) throw new Error("Request failed");
      return response.json();
    }

    async function loadConfig() {
      try { render(await api("/api/config")); }
      catch { render(demoConfig); }
    }

    async function saveConfig() {
      const body = new URLSearchParams();
      body.set("deviceName", $("deviceName").value);
      try { render(await api("/api/config", { method: "POST", body })); }
      catch { render({ ...demoConfig, deviceName: $("deviceName").value }); }
    }

    async function startPairing() {
      $("pairStatus").textContent = "Pairing started. Keep both devices powered in config mode.";
      try { updatePairStatus(await api("/api/pair/start", { method: "POST" })); }
      catch { updatePairStatus({ active: true, hasPeer: true, peerMac: demoConfig.peerMac, message: "Demo pairing complete." }); }
    }

    async function pollPairStatus() {
      try { updatePairStatus(await api("/api/pair/status")); } catch {}
    }

    async function rebootDevice() {
      try { await api("/api/reboot", { method: "POST" }); } catch {}
    }

    $("saveBtn").addEventListener("click", saveConfig);
    $("pairBtn").addEventListener("click", startPairing);
    $("rebootBtn").addEventListener("click", rebootDevice);
    loadConfig();
    if (shouldPollPairStatus) setInterval(pollPairStatus, 2000);
  </script>
</body>
</html>
)SHUTUP_HTML";

static constexpr const char kConfigCanHtml[] = R"SHUTUP_HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ShutUp Canopy Config</title>
  <style>
    :root { color-scheme: light; --ink:#172126; --muted:#5e6b73; --line:#cdd6dc; --panel:#f7f9fa; --accent:#0b6bcb; --good:#168447; --warn:#a35a00; }
    * { box-sizing: border-box; }
    body { margin: 0; font-family: Arial, Helvetica, sans-serif; color: var(--ink); background: #e9eef2; }
    header { padding: 20px 18px 12px; background: #ffffff; border-bottom: 1px solid var(--line); }
    h1 { margin: 0; font-size: 24px; }
    main { max-width: 860px; margin: 0 auto; padding: 18px; }
    section { background: #fff; border: 1px solid var(--line); border-radius: 6px; margin-bottom: 14px; padding: 16px; }
    h2 { margin: 0 0 12px; font-size: 18px; }
    label { display: block; font-weight: 700; margin: 12px 0 6px; }
    input[type="text"], input[type="number"], select { width: 100%; padding: 10px; border: 1px solid var(--line); border-radius: 4px; font-size: 16px; }
    button { border: 0; border-radius: 4px; padding: 10px 14px; background: var(--accent); color: #fff; font-weight: 700; font-size: 15px; cursor: pointer; }
    button.secondary { background: #56636b; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(210px, 1fr)); gap: 10px; }
    .status { background: var(--panel); border: 1px solid var(--line); border-radius: 4px; padding: 10px; min-height: 42px; }
    .muted { color: var(--muted); }
    .row { display: flex; gap: 10px; flex-wrap: wrap; align-items: center; }
    .door-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 8px; }
    .door { border: 1px solid var(--line); border-radius: 4px; padding: 10px; background: var(--panel); }
    .door label { display: flex; gap: 8px; align-items: center; margin: 0; }
    .ok { color: var(--good); font-weight: 700; }
    .warn { color: var(--warn); font-weight: 700; }
    .sound-table { width: 100%; border-collapse: collapse; table-layout: fixed; }
    .sound-table th, .sound-table td { border-bottom: 1px solid var(--line); padding: 8px; text-align: left; vertical-align: middle; }
    .sound-table th { background: var(--panel); }
    .sound-table td:first-child { font-weight: 700; }
    .sound-table .action-col { width: 150px; }
    .sound-table .sound-col { width: auto; }
    .sound-table .repeat-col { width: 72px; text-align: center; }
    .sound-table .delay-col { width: 96px; }
    .sound-table .demo-col { width: 76px; }
    .repeat-cell { text-align: center; }
    .repeat-cell input { width: auto; }
    .sound-table button { width: 100%; padding-left: 8px; padding-right: 8px; }
    .overlay-table { width: 100%; border-collapse: collapse; table-layout: fixed; }
    .overlay-table th, .overlay-table td { border-bottom: 1px solid var(--line); padding: 8px; text-align: left; vertical-align: middle; }
    .overlay-table th { background: var(--panel); }
    .overlay-table .num-col { width: 42px; text-align: center; font-weight: 700; }
    .overlay-table .name-col { width: auto; }
    .overlay-table .small-col { width: 78px; }
    .overlay-table .color-col { width: 76px; }
    .overlay-table .save-col { width: 72px; }
    .overlay-table button { width: 100%; padding-left: 8px; padding-right: 8px; }
  </style>
</head>
<body>
  <header>
    <h1>ShutUp Canopy Config</h1>
    <div class="muted">SSID: SHUTUP-CANCONF</div>
  </header>
  <main>
    <section>
      <h2>Device</h2>
      <label for="deviceName">Device name</label>
      <input id="deviceName" name="deviceName" type="text" maxlength="31" value="ShutUp Canopy">
      <label for="uteColor">Ute colour</label>
      <select id="uteColor" name="uteColor">
        <option value="black">Black</option>
        <option value="white">White</option>
        <option value="gray">Gray</option>
        <option value="red">Red</option>
        <option value="blue">Blue</option>
        <option value="green">Green</option>
        <option value="yellow">Yellow</option>
      </select>
      <div class="grid">
        <div class="status"><div class="muted">Local MAC</div><div id="localMac">Loading...</div></div>
        <div class="status"><div class="muted">Paired peer</div><div id="peerMac">Loading...</div></div>
      </div>
      <p class="muted">The Canopy device stores door state and responds to Cab state requests over ESP-NOW.</p>
      <div class="row">
        <button id="saveBtn">Save config</button>
        <button id="rebootBtn" class="secondary">Reboot</button>
      </div>
    </section>

    <section>
      <h2>Action Sounds</h2>
      <table class="sound-table">
        <thead>
          <tr>
            <th class="action-col">Action</th>
            <th class="sound-col">Cab</th>
            <th class="sound-col">Canopy</th>
            <th class="repeat-col">Repeat</th>
            <th class="delay-col">Delay</th>
            <th class="demo-col">Demo</th>
          </tr>
        </thead>
        <tbody id="soundRows"></tbody>
      </table>
    </section>

    <section>
      <h2>Door Sensors</h2>
      <p class="muted">Enable only the normally closed reed switch inputs that are physically installed.</p>
      <div class="door-grid">
        <div class="door"><label><input id="door1" type="checkbox"> Door sensor 1</label></div>
        <div class="door"><label><input id="door2" type="checkbox"> Door sensor 2</label></div>
        <div class="door"><label><input id="door3" type="checkbox"> Door sensor 3</label></div>
        <div class="door"><label><input id="door4" type="checkbox"> Door sensor 4</label></div>
        <div class="door"><label><input id="door5" type="checkbox"> Door sensor 5</label></div>
        <div class="door"><label><input id="door6" type="checkbox"> Door sensor 6</label></div>
      </div>
    </section>

    <section>
      <h2>Cab Door Overlay</h2>
      <table class="overlay-table">
        <thead>
          <tr>
            <th class="num-col"></th>
            <th class="name-col">Name</th>
            <th class="small-col">Width</th>
            <th class="small-col">Height</th>
            <th class="small-col">X</th>
            <th class="small-col">Y</th>
            <th class="color-col">Closed</th>
            <th class="color-col">Open</th>
            <th class="save-col">Save</th>
          </tr>
        </thead>
        <tbody id="overlayRows"></tbody>
      </table>
    </section>

    <section>
      <h2>Pair Devices</h2>
      <p>Put both devices into config mode. Connect your phone to either config Wi-Fi network, open that config page, then press Pair Devices. The other device does not need its page open.</p>
      <div class="status" id="pairStatus">Pairing idle.</div>
      <div class="row" style="margin-top:12px">
        <button id="pairBtn">Pair Devices</button>
      </div>
    </section>
  </main>

  <script>
    const demoConfig = {
      role: "Canopy",
      deviceName: "ShutUp Canopy",
      localMac: "34:85:18:CA:10:02",
      hasPeer: true,
      peerMac: "24:6F:28:CA:B0:01",
      uteColor: "black",
      doorEnabledMask: 0b00111111,
      doorOverlays: [
        { name: "Door #1", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" },
        { name: "Door #2", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" },
        { name: "Door #3", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" },
        { name: "Door #4", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" },
        { name: "Door #5", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" },
        { name: "Door #6", width: 0, height: 0, x: 0, y: 0, closed: "#00FF00", open: "#FF0000" }
      ],
      soundOptions: ["None", "PowerUp", "PowerDown", "PewPew", "Pulse", "Confirm", "Error", "SaveOK", "Restarting", "ConnectWiFi", "LostWiFi", "SearchingLoop", "AlarmFast", "AlarmSlow", "MuteOn", "MuteOff", "DoorOpenWarn", "DoorFault", "Click1", "Click2", "SuccessHappy", "WarningBeep", "ChargingUp", "ChargingDown", "Notify", "BossFight"],
      soundActions: [
        { name: "Startup", cab: "None", canopy: "None", repeat: false, delay: 0 },
        { name: "Connectivity success", cab: "None", canopy: "None", repeat: false, delay: 0 },
        { name: "Connectivity error", cab: "None", canopy: "None", repeat: false, delay: 0 },
        { name: "Doors ok", cab: "None", canopy: "None", repeat: false, delay: 0 },
        { name: "Door alarm", cab: "None", canopy: "None", repeat: false, delay: 0 }
      ]
    };
    const demoTones = {
      "PowerUp": [[800,80], [1000,80], [1300,120]],
      "PowerDown": [[1200,100], [900,100], [600,140]],
      "PewPew": [[1800,60], [0,40], [2000,60]],
      "Pulse": [[1200,120]],
      "Confirm": [[1100,70], [0,40], [1400,90]],
      "Error": [[400,160], [0,60], [1000,60]],
      "SaveOK": [[900,60], [0,40], [1200,80]],
      "Restarting": [[1000,120], [800,120], [600,180]],
      "ConnectWiFi": [[1400,60], [1600,60], [1800,80]],
      "LostWiFi": [[900,120], [600,160]],
      "SearchingLoop": [[1600,60], [0,300]],
      "AlarmFast": [[1500,80], [0,40], [1500,80], [0,40], [1500,80]],
      "AlarmSlow": [[700,400], [0,300]],
      "MuteOn": [[500,70], [0,40], [500,70]],
      "MuteOff": [[1000,70], [0,40], [1000,70]],
      "DoorOpenWarn": [[800,70], [1200,90], [800,70]],
      "DoorFault": [[400,200], [0,100]],
      "Click1": [[2000,40]],
      "Click2": [[2200,40], [0,40], [2200,40]],
      "SuccessHappy": [[1000,60], [1400,60], [1800,100]],
      "WarningBeep": [[800,200]],
      "ChargingUp": [[700,60], [900,60], [1100,60], [1400,80]],
      "ChargingDown": [[1400,60], [1100,60], [900,60], [700,80]],
      "Notify": [[1300,120]],
      "BossFight": [[1800,70], [800,90], [2000,120]],
    };

    const $ = (id) => document.getElementById(id);

    function render(config) {
      $("deviceName").value = config.deviceName || "ShutUp Canopy";
      $("uteColor").value = config.uteColor || "black";
      $("localMac").textContent = config.localMac || "Unknown";
      $("peerMac").innerHTML = config.hasPeer ? `<span class="ok">${config.peerMac}</span>` : `<span class="warn">Not paired</span>`;
      for (let i = 0; i < 6; i++) {
        $("door" + (i + 1)).checked = (config.doorEnabledMask & (1 << i)) !== 0;
      }
      renderSoundRows(config);
      renderOverlayRows(config);
    }

    function soundOptionsHtml(options, selected) {
      return (options || ["None"]).map((name) => {
        const safe = String(name).replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll('"', "&quot;");
        return `<option value="${safe}" ${name === selected ? "selected" : ""}>${safe}</option>`;
      }).join("");
    }

    function renderSoundRows(config) {
      const options = config.soundOptions || ["None"];
      const actions = config.soundActions || demoConfig.soundActions;
      $("soundRows").innerHTML = actions.map((action, index) => `
        <tr>
          <td>${action.name}</td>
          <td><select id="action${index}Cab">${soundOptionsHtml(options, action.cab || "None")}</select></td>
          <td><select id="action${index}Canopy">${soundOptionsHtml(options, action.canopy || "None")}</select></td>
          <td class="repeat-cell"><input id="action${index}Repeat" type="checkbox" ${action.repeat ? "checked" : ""}></td>
          <td><input id="action${index}Delay" type="number" min="0" step="100" value="${action.delay || 0}"></td>
          <td><button type="button" data-demo="${index}">Play</button></td>
        </tr>`).join("");
      document.querySelectorAll("[data-demo]").forEach((button) => {
        button.addEventListener("click", () => demoSound(Number(button.dataset.demo)));
      });
    }

    function renderOverlayRows(config) {
      const overlays = config.doorOverlays || demoConfig.doorOverlays;
      $("overlayRows").innerHTML = overlays.map((overlay, index) => `
        <tr>
          <td class="num-col">${index + 1}</td>
          <td><input id="overlay${index}Name" type="text" maxlength="31" value="${escapeAttr(overlay.name || `Door #${index + 1}`)}"></td>
          <td><input id="overlay${index}Width" type="number" min="0" max="170" value="${overlay.width || 0}"></td>
          <td><input id="overlay${index}Height" type="number" min="0" max="320" value="${overlay.height || 0}"></td>
          <td><input id="overlay${index}X" type="number" min="0" max="170" value="${overlay.x || 0}"></td>
          <td><input id="overlay${index}Y" type="number" min="0" max="320" value="${overlay.y || 0}"></td>
          <td><input id="overlay${index}Closed" type="color" value="${overlay.closed || "#00FF00"}"></td>
          <td><input id="overlay${index}Open" type="color" value="${overlay.open || "#FF0000"}"></td>
          <td><button type="button" data-overlay-save="${index}">Save</button></td>
        </tr>`).join("");
      document.querySelectorAll("[data-overlay-save]").forEach((button) => {
        button.addEventListener("click", () => saveOverlay(Number(button.dataset.overlaySave)));
      });
    }

    function escapeAttr(value) {
      return String(value).replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll('"', "&quot;");
    }

    function updatePairStatus(status) {
      const paired = status.hasPeer ? `Paired peer: ${status.peerMac}` : "No peer stored";
      $("pairStatus").textContent = `${status.message || "Pairing status"} - ${paired}`;
    }

    let previewAudio = null;
    let previewOscillators = [];

    function stopTonePreview() {
      previewOscillators.forEach((oscillator) => {
        try { oscillator.stop(); } catch {}
      });
      previewOscillators = [];
    }

    function playTonePreview(name) {
      const steps = demoTones[name];
      if (!steps || !steps.length) return;
      const AudioContext = window.AudioContext || window.webkitAudioContext;
      if (!AudioContext) return;
      if (!previewAudio) previewAudio = new AudioContext();
      previewAudio.resume();
      stopTonePreview();

      let when = previewAudio.currentTime + 0.03;
      steps.forEach((step) => {
        const frequency = Number(step[0]);
        const duration = Math.max(0.001, Number(step[1]) / 1000);
        if (frequency > 0) {
          const oscillator = previewAudio.createOscillator();
          const gain = previewAudio.createGain();
          oscillator.type = "square";
          oscillator.frequency.setValueAtTime(frequency, when);
          gain.gain.setValueAtTime(0.08, when);
          gain.gain.setTargetAtTime(0, when + Math.max(0, duration - 0.02), 0.01);
          oscillator.connect(gain).connect(previewAudio.destination);
          oscillator.start(when);
          oscillator.stop(when + duration);
          previewOscillators.push(oscillator);
        }
        when += duration;
      });
    }
    const shouldPollPairStatus = true;

    async function api(path, options) {
      const response = await fetch(path, options);
      if (!response.ok) throw new Error("Request failed");
      return response.json();
    }

    async function loadConfig() {
      try { render(await api("/api/config")); }
      catch { render(demoConfig); }
    }

    async function saveConfig() {
      const body = new URLSearchParams();
      body.set("deviceName", $("deviceName").value);
      body.set("uteColor", $("uteColor").value);
      body.set("doorTouched", "on");
      body.set("soundTouched", "on");
      for (let i = 1; i <= 6; i++) {
        if ($("door" + i).checked) body.set("door" + i, "on");
      }
      for (let i = 0; i < demoConfig.soundActions.length; i++) {
        body.set(`action${i}Cab`, $(`action${i}Cab`).value);
        body.set(`action${i}Canopy`, $(`action${i}Canopy`).value);
        if ($(`action${i}Repeat`).checked) body.set(`action${i}Repeat`, "on");
        body.set(`action${i}Delay`, $(`action${i}Delay`).value);
      }
      try { render(await api("/api/config", { method: "POST", body })); }
      catch {
        let mask = 0;
        for (let i = 0; i < 6; i++) if ($("door" + (i + 1)).checked) mask |= (1 << i);
        render({ ...demoConfig, deviceName: $("deviceName").value, uteColor: $("uteColor").value, doorEnabledMask: mask });
      }
    }

    async function startPairing() {
      $("pairStatus").textContent = "Pairing started. Keep both devices powered in config mode.";
      try { updatePairStatus(await api("/api/pair/start", { method: "POST" })); }
      catch { updatePairStatus({ active: true, hasPeer: true, peerMac: demoConfig.peerMac, message: "Demo pairing complete." }); }
    }

    async function pollPairStatus() {
      try { updatePairStatus(await api("/api/pair/status")); } catch {}
    }

    async function rebootDevice() {
      try { await api("/api/reboot", { method: "POST" }); } catch {}
    }

    async function saveOverlay(index) {
      const body = new URLSearchParams();
      body.set(`overlay${index}Touched`, "on");
      body.set(`overlay${index}Name`, $(`overlay${index}Name`).value || `Door #${index + 1}`);
      body.set(`overlay${index}Width`, $(`overlay${index}Width`).value);
      body.set(`overlay${index}Height`, $(`overlay${index}Height`).value);
      body.set(`overlay${index}X`, $(`overlay${index}X`).value);
      body.set(`overlay${index}Y`, $(`overlay${index}Y`).value);
      body.set(`overlay${index}Closed`, $(`overlay${index}Closed`).value);
      body.set(`overlay${index}Open`, $(`overlay${index}Open`).value);
      try { render(await api("/api/config", { method: "POST", body })); }
      catch {
        const overlays = [...demoConfig.doorOverlays];
        overlays[index] = {
          name: body.get(`overlay${index}Name`),
          width: Number(body.get(`overlay${index}Width`)),
          height: Number(body.get(`overlay${index}Height`)),
          x: Number(body.get(`overlay${index}X`)),
          y: Number(body.get(`overlay${index}Y`)),
          closed: body.get(`overlay${index}Closed`),
          open: body.get(`overlay${index}Open`)
        };
        render({ ...demoConfig, doorOverlays: overlays });
      }
    }

    async function demoSound(index) {
      const body = new URLSearchParams();
      body.set("sound", $(`action${index}Canopy`).value);
      try { await api("/api/sound/demo", { method: "POST", body }); } catch {}
    }

    $("saveBtn").addEventListener("click", saveConfig);
    $("pairBtn").addEventListener("click", startPairing);
    $("rebootBtn").addEventListener("click", rebootDevice);
    loadConfig();
    if (shouldPollPairStatus) setInterval(pollPairStatus, 2000);
  </script>
</body>
</html>
)SHUTUP_HTML";

static constexpr size_t kSoundAssetCount = 25;
static constexpr const char *kSoundAssets[] = {
  "PowerUp",
  "PowerDown",
  "PewPew",
  "Pulse",
  "Confirm",
  "Error",
  "SaveOK",
  "Restarting",
  "ConnectWiFi",
  "LostWiFi",
  "SearchingLoop",
  "AlarmFast",
  "AlarmSlow",
  "MuteOn",
  "MuteOff",
  "DoorOpenWarn",
  "DoorFault",
  "Click1",
  "Click2",
  "SuccessHappy",
  "WarningBeep",
  "ChargingUp",
  "ChargingDown",
  "Notify",
  "BossFight",
};

}  // namespace shutup