import re
from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]
    ROOT = Path(env["PROJECT_DIR"]).resolve().parent  # type: ignore[name-defined]
except NameError:
    ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "shared" / "web"
GENERATED = ROOT / "shared" / "generated"
DEMO = ROOT / "demo"
TONES_HEADER = ROOT / "shared" / "include" / "shutup_sounds.h"

ASSETS = [
    ("config_can.html", "kConfigCanHtml"),
]

CAN_DEVICE_TRANSPORT = """    const shouldPollPairStatus = true;

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
        render({ ...demoConfig, uteColor: $("uteColor").value, doorEnabledMask: mask });
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
    }"""

CAN_DEMO_TRANSPORT = """    const shouldPollPairStatus = false;
    let demoState = JSON.parse(JSON.stringify(demoConfig));

    function readDoorEnabledMask() {
      let mask = 0;
      for (let i = 0; i < 6; i++) if ($("door" + (i + 1)).checked) mask |= (1 << i);
      return mask;
    }

    function readSoundActions() {
      return demoState.soundActions.map((action, index) => ({
        name: action.name,
        cab: $(`action${index}Cab`).value,
        canopy: $(`action${index}Canopy`).value,
        repeat: $(`action${index}Repeat`).checked,
        delay: Number($(`action${index}Delay`).value || 0)
      }));
    }

    function loadConfig() {
      render(demoState);
    }

    function saveConfig() {
      demoState.uteColor = $("uteColor").value;
      demoState.doorEnabledMask = readDoorEnabledMask();
      demoState.soundActions = readSoundActions();
      render(demoState);
    }

    function startPairing() {
      $("pairStatus").textContent = "Pairing started. Keep both devices powered in config mode.";
      updatePairStatus({ active: true, hasPeer: true, peerMac: demoState.peerMac, message: "Demo pairing complete." });
    }

    function pollPairStatus() {}

    function rebootDevice() {}

    function saveOverlay(index) {
      demoState.doorOverlays[index] = {
        name: $(`overlay${index}Name`).value || `Door #${index + 1}`,
        width: Number($(`overlay${index}Width`).value || 0),
        height: Number($(`overlay${index}Height`).value || 0),
        x: Number($(`overlay${index}X`).value || 0),
        y: Number($(`overlay${index}Y`).value || 0),
        closed: $(`overlay${index}Closed`).value,
        open: $(`overlay${index}Open`).value
      };
      render(demoState);
    }

    function demoSound(index) {
      playTonePreview($(`action${index}Canopy`).value);
    }"""


def cpp_raw_string(text):
    marker = "SHUTUP_HTML"
    return f'R"{marker}({text}){marker}"'


def cpp_string(text):
    escaped = text.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def js_string(text):
    escaped = text.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def sound_definitions():
    text = TONES_HEADER.read_text(encoding="utf-8")
    step_arrays = {}
    for match in re.finditer(r"static constexpr ToneStep (\w+)\[\] = \{(.*?)\};", text, re.S):
        steps = []
        for step in re.finditer(r"\{\s*(\d+)\s*,\s*(\d+)\s*\}", match.group(2)):
            steps.append((int(step.group(1)), int(step.group(2))))
        step_arrays[match.group(1)] = steps

    sounds = []
    in_table = False
    for line in text.splitlines():
        if "kToneSounds[]" in line:
            in_table = True
            continue
        if in_table and line.strip().startswith("};"):
            break
        if not in_table:
            continue
        match = re.search(r'\{\s*\d+\s*,\s*"([^"]+)"\s*,\s*(?:true|false)\s*,\s*(\w+)', line)
        if match and match.group(1) != "None":
            sounds.append({"name": match.group(1), "steps": step_arrays.get(match.group(2), [])})
    return sounds


def js_steps(steps):
    return "[" + ", ".join(f"[{frequency},{duration}]" for frequency, duration in steps) + "]"


def inject_sound_options(filename, text, sounds):
    if filename != "config_can.html":
        return text
    names = [sound["name"] for sound in sounds]
    options = ["None"] + names
    replacement = "soundOptions: [" + ", ".join(js_string(name) for name in options) + "],"
    tones = "const demoTones = {\n"
    for sound in sounds:
        tones += f"      {js_string(sound['name'])}: {js_steps(sound['steps'])},\n"
    tones += "    };"
    return text.replace('soundOptions: ["None"],', replacement).replace("const demoTones = {};", tones)


def inject_transport(filename, text, demo):
    if filename == "config_can.html":
        transport = CAN_DEMO_TRANSPORT if demo else CAN_DEVICE_TRANSPORT
    else:
        raise ValueError(f"No transport for {filename}")
    if "/*__SHUTUP_TRANSPORT__*/" not in text:
        raise ValueError(f"Missing transport placeholder in {filename}")
    return text.replace("/*__SHUTUP_TRANSPORT__*/", transport)


def main():
    GENERATED.mkdir(parents=True, exist_ok=True)
    DEMO.mkdir(parents=True, exist_ok=True)

    header = [
        "#pragma once",
        "",
        "namespace shutup {",
        "",
    ]

    sounds = sound_definitions()

    for filename, symbol in ASSETS:
        source = WEB / filename
        template = inject_sound_options(filename, source.read_text(encoding="utf-8"), sounds)
        device_text = inject_transport(filename, template, demo=False)
        demo_text = inject_transport(filename, template, demo=True)
        (DEMO / filename).write_text(demo_text, encoding="utf-8", newline="\n")
        header.append(f"static constexpr const char {symbol}[] = {cpp_raw_string(device_text)};")
        header.append("")

    header.append(f"static constexpr size_t kSoundAssetCount = {len(sounds)};")
    header.append("static constexpr const char *kSoundAssets[] = {")
    for sound in sounds:
        header.append(f"  {cpp_string(sound['name'])},")
    header.append("};")
    header.append("")

    header.append("}  // namespace shutup")
    (GENERATED / "web_assets.h").write_text("\n".join(header), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
