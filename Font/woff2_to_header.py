import sys
from pathlib import Path

def main():
    if len(sys.argv) < 3:
        print("Usage: python woff2_to_header.py <input.woff2> <output.h> [varname]")
        sys.exit(1)

    inp = Path(sys.argv[1])
    out = Path(sys.argv[2])
    var = sys.argv[3] if len(sys.argv) >= 4 else "wordclock_woff2"

    data = inp.read_bytes()
    n = len(data)

    lines = []
    lines.append("#pragma once")
    lines.append("#include <Arduino.h>")
    lines.append("")
    lines.append(f"// Generated from: {inp.name} ({n} bytes)")
    lines.append(f"const uint8_t {var}[] PROGMEM = {{")

    # 16 bytes per line
    for i in range(0, n, 16):
        chunk = data[i:i+16]
        hexes = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"  {hexes},")

    lines.append("};")
    lines.append(f"const size_t {var}_len = {n};")
    lines.append("")

    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"OK: wrote {out} with {n} bytes as {var} / {var}_len")

if __name__ == "__main__":
    main()
