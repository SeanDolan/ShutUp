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
    ("config_cab.html", "kConfigCabHtml"),
    ("config_can.html", "kConfigCanHtml"),
]


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
        text = inject_sound_options(filename, source.read_text(encoding="utf-8"), sounds)
        (DEMO / filename).write_text(text, encoding="utf-8", newline="\n")
        header.append(f"static constexpr const char {symbol}[] = {cpp_raw_string(text)};")
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
