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


def sound_names():
    text = TONES_HEADER.read_text(encoding="utf-8")
    names = []
    in_table = False
    for line in text.splitlines():
        if "kToneSounds[]" in line:
            in_table = True
            continue
        if in_table and line.strip().startswith("};"):
            break
        if not in_table:
            continue
        match = re.search(r'\{\s*\d+\s*,\s*"([^"]+)"', line)
        if match and match.group(1) != "None":
            names.append(match.group(1))
    return names


def inject_sound_options(filename, text, sounds):
    if filename != "config_can.html":
        return text
    options = ["None"] + sounds
    replacement = "soundOptions: [" + ", ".join(js_string(name) for name in options) + "],"
    return text.replace('soundOptions: ["None"],', replacement)


def main():
    GENERATED.mkdir(parents=True, exist_ok=True)
    DEMO.mkdir(parents=True, exist_ok=True)

    header = [
        "#pragma once",
        "",
        "namespace shutup {",
        "",
    ]

    sounds = sound_names()

    for filename, symbol in ASSETS:
        source = WEB / filename
        text = inject_sound_options(filename, source.read_text(encoding="utf-8"), sounds)
        (DEMO / filename).write_text(text, encoding="utf-8", newline="\n")
        header.append(f"static constexpr const char {symbol}[] = {cpp_raw_string(text)};")
        header.append("")

    header.append(f"static constexpr size_t kSoundAssetCount = {len(sounds)};")
    header.append("static constexpr const char *kSoundAssets[] = {")
    for name in sounds:
        header.append(f"  {cpp_string(name)},")
    header.append("};")
    header.append("")

    header.append("}  // namespace shutup")
    (GENERATED / "web_assets.h").write_text("\n".join(header), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
