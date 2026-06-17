import struct
import zlib
from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]
    ROOT = Path(env["PROJECT_DIR"]).resolve().parent  # type: ignore[name-defined]
except NameError:
    ROOT = Path(__file__).resolve().parents[1]

IMAGE_DIR = ROOT / "Cab" / "data" / "images"
GENERATED = ROOT / "Cab" / "generated"
HEADER = GENERATED / "cab_images.h"

DISPLAY_WIDTH = 170
DISPLAY_HEIGHT = 320

IMAGES = [
    ("startup.png", "kCabStartupImage"),
    ("connecting.png", "kCabConnectingImage"),
    ("pairing.png", "kCabPairingImage"),
    ("ute_black.png", "kCabUteBlackImage"),
    ("ute_white.png", "kCabUteWhiteImage"),
    ("ute_gray.png", "kCabUteGrayImage"),
    ("ute_red.png", "kCabUteRedImage"),
    ("ute_blue.png", "kCabUteBlueImage"),
    ("ute_green.png", "kCabUteGreenImage"),
    ("ute_yellow.png", "kCabUteYellowImage"),
]


def paeth(a, b, c):
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def decode_png(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")

    pos = 8
    width = height = bit_depth = color_type = None
    compressed = bytearray()
    palette = None
    transparency = None

    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        chunk_type = data[pos + 4:pos + 8]
        chunk_data = data[pos + 8:pos + 8 + length]
        pos += 12 + length

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(">IIBBBBB", chunk_data)
            if bit_depth != 8 or compression != 0 or filter_method != 0 or interlace != 0:
                raise ValueError(f"{path} must be non-interlaced 8-bit PNG")
        elif chunk_type == b"PLTE":
            palette = [tuple(chunk_data[i:i + 3]) for i in range(0, len(chunk_data), 3)]
        elif chunk_type == b"tRNS":
            transparency = chunk_data
        elif chunk_type == b"IDAT":
            compressed.extend(chunk_data)
        elif chunk_type == b"IEND":
            break

    if width != DISPLAY_WIDTH or height != DISPLAY_HEIGHT:
        raise ValueError(f"{path.name} is {width}x{height}; expected {DISPLAY_WIDTH}x{DISPLAY_HEIGHT}")

    channels_by_type = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}
    if color_type not in channels_by_type:
        raise ValueError(f"{path} has unsupported PNG color type {color_type}")
    channels = channels_by_type[color_type]
    stride = width * channels
    raw = zlib.decompress(bytes(compressed))
    rows = []
    offset = 0
    previous = bytearray(stride)

    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset:offset + stride])
        offset += stride

        for i in range(stride):
            left = row[i - channels] if i >= channels else 0
            up = previous[i]
            up_left = previous[i - channels] if i >= channels else 0
            if filter_type == 1:
                row[i] = (row[i] + left) & 0xFF
            elif filter_type == 2:
                row[i] = (row[i] + up) & 0xFF
            elif filter_type == 3:
                row[i] = (row[i] + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                row[i] = (row[i] + paeth(left, up, up_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"{path} has unsupported PNG filter {filter_type}")
        rows.append(row)
        previous = row

    pixels = []
    for row in rows:
        for x in range(width):
            base = x * channels
            if color_type == 0:
                r = g = b = row[base]
            elif color_type == 2:
                r, g, b = row[base], row[base + 1], row[base + 2]
            elif color_type == 3:
                if palette is None:
                    raise ValueError(f"{path} uses indexed color without palette")
                r, g, b = palette[row[base]]
                if transparency and row[base] < len(transparency) and transparency[row[base]] == 0:
                    r = g = b = 0
            elif color_type == 4:
                r = g = b = row[base]
                if row[base + 1] == 0:
                    r = g = b = 0
            else:
                r, g, b = row[base], row[base + 1], row[base + 2]
                if row[base + 3] == 0:
                    r = g = b = 0

            pixels.append(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

    return pixels


def emit_array(lines, symbol, pixels):
    lines.append(f"static const uint16_t {symbol}[kCabImagePixelCount] PROGMEM = {{")
    for i in range(0, len(pixels), 12):
        chunk = ", ".join(f"0x{value:04X}" for value in pixels[i:i + 12])
        lines.append(f"  {chunk},")
    lines.append("};")
    lines.append("")


def main():
    GENERATED.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "#include <pgmspace.h>",
        "",
        "namespace shutup {",
        "",
        f"static constexpr uint16_t kCabImageWidth = {DISPLAY_WIDTH};",
        f"static constexpr uint16_t kCabImageHeight = {DISPLAY_HEIGHT};",
        "static constexpr uint32_t kCabImagePixelCount = static_cast<uint32_t>(kCabImageWidth) * kCabImageHeight;",
        "",
    ]

    for filename, symbol in IMAGES:
        emit_array(lines, symbol, decode_png(IMAGE_DIR / filename))

    lines.extend([
        "}  // namespace shutup",
        "",
    ])
    HEADER.write_text("\n".join(lines), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
