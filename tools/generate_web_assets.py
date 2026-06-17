from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]
    ROOT = Path(env["PROJECT_DIR"]).resolve().parent  # type: ignore[name-defined]
except NameError:
    ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "shared" / "web"
GENERATED = ROOT / "shared" / "generated"
DEMO = ROOT / "demo"
SOUND_DIRS = [
    ROOT / "Cab" / "data" / "sounds",
    ROOT / "Canopy" / "data" / "sounds",
]

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


def sound_names():
    names = set()
    for sound_dir in SOUND_DIRS:
        if not sound_dir.exists():
            continue
        for path in sound_dir.glob("*.mp3"):
            names.add(path.stem)
    return sorted(names, key=str.casefold)


def main():
    GENERATED.mkdir(parents=True, exist_ok=True)
    DEMO.mkdir(parents=True, exist_ok=True)

    header = [
        "#pragma once",
        "",
        "namespace shutup {",
        "",
    ]

    for filename, symbol in ASSETS:
        source = WEB / filename
        text = source.read_text(encoding="utf-8")
        (DEMO / filename).write_text(text, encoding="utf-8", newline="\n")
        header.append(f"static constexpr const char {symbol}[] = {cpp_raw_string(text)};")
        header.append("")

    sounds = sound_names()
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
