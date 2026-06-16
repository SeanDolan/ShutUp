from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]
    ROOT = Path(env["PROJECT_DIR"]).resolve().parent  # type: ignore[name-defined]
except NameError:
    ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "shared" / "web"
GENERATED = ROOT / "shared" / "generated"
DEMO = ROOT / "demo"

ASSETS = [
    ("config_cab.html", "kConfigCabHtml"),
    ("config_can.html", "kConfigCanHtml"),
]


def cpp_raw_string(text):
    marker = "SHUTUP_HTML"
    return f'R"{marker}({text}){marker}"'


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

    header.append("}  // namespace shutup")
    (GENERATED / "web_assets.h").write_text("\n".join(header), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
