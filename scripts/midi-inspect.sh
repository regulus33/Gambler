#!/usr/bin/env bash
# midi-inspect.sh — pretty-print a MIDI file using Gambler's note names + colours.
#
# Usage:   scripts/midi-inspect.sh <path-to-.mid>
# Source of truth for names + colours: source/ui/MusicConstants.h
#
# Output:
#   <name1> <name2> ...      ← MIDI note names (Gambler convention, e.g. C4)
#   [] [] ...                ← coloured squares using NOTE_COLOURS hex values
#   length: <steps> steps · <ticks> ticks · <beats> beats @ <PPQ> PPQ

set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <midi-file>" >&2
    exit 64
fi

MIDI_FILE="$1"
if [[ ! -f "$MIDI_FILE" ]]; then
    echo "error: not a file: $MIDI_FILE" >&2
    exit 66
fi

exec python3 - "$MIDI_FILE" <<'PY'
import struct
import sys

# Mirrors source/ui/MusicConstants.h (must stay in sync).
CHROMATIC_NOTE_NAMES = ["C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"]
NOTE_COLOURS_HEX = [
    0xFF6B6B,  # C
    0xFF9F6B,  # Db
    0xFFD66B,  # D
    0xFFE66B,  # Eb
    0xE6FF6B,  # E
    0x6BFFB4,  # F
    0x6BFFD6,  # Gb
    0x6BD6FF,  # G
    0x6BB4FF,  # Ab
    0x9F6BFF,  # A
    0xD66BFF,  # Bb
    0xFF6BD6,  # B
]

def midi_to_name(n):
    if n < 0 or n > 127:
        return "-"
    return CHROMATIC_NOTE_NAMES[n % 12] + str((n // 12) - 1)

def colour_block(n):
    rgb = NOTE_COLOURS_HEX[n % 12]
    r, g, b = (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF
    return f"\033[48;2;{r};{g};{b}m  \033[0m"

def read_vlq(buf, i):
    val = 0
    while True:
        byte = buf[i]
        i += 1
        val = (val << 7) | (byte & 0x7F)
        if not (byte & 0x80):
            return val, i

def parse_midi(path):
    with open(path, "rb") as f:
        data = f.read()

    if data[:4] != b"MThd":
        raise SystemExit("error: not a Standard MIDI File (missing MThd)")

    hlen = struct.unpack(">I", data[4:8])[0]
    fmt, ntracks, division = struct.unpack(">HHH", data[8:8 + hlen][:6])
    i = 8 + hlen

    note_ons = []     # (tick, note)
    last_tick = 0     # max tick seen anywhere (for sequence length)

    for _ in range(ntracks):
        if data[i:i+4] != b"MTrk":
            raise SystemExit("error: malformed track header")
        tlen = struct.unpack(">I", data[i+4:i+8])[0]
        i += 8
        end = i + tlen
        tick = 0
        running_status = None

        while i < end:
            delta, i = read_vlq(data, i)
            tick += delta
            status = data[i]
            if status < 0x80:
                # running status
                if running_status is None:
                    raise SystemExit("error: running status with no prior status")
                status = running_status
            else:
                i += 1
                running_status = status

            high = status & 0xF0
            if high in (0x80, 0x90, 0xA0, 0xB0, 0xE0):
                d1, d2 = data[i], data[i+1]
                i += 2
                if high == 0x90 and d2 > 0:
                    note_ons.append((tick, d1))
            elif high in (0xC0, 0xD0):
                i += 1
            elif status == 0xFF:
                meta_type = data[i]; i += 1
                mlen, i = read_vlq(data, i)
                i += mlen
                if meta_type == 0x2F:  # End of Track
                    break
            elif status in (0xF0, 0xF7):
                slen, i = read_vlq(data, i)
                i += slen
            else:
                raise SystemExit(f"error: unknown status byte 0x{status:02X}")

            if tick > last_tick:
                last_tick = tick

    return note_ons, last_tick, division

def main():
    path = sys.argv[1]
    note_ons, last_tick, division = parse_midi(path)
    note_ons.sort(key=lambda x: x[0])

    if not note_ons:
        print("(no note-on events)")
    else:
        names = [midi_to_name(n) for _, n in note_ons]
        widths = [max(2, len(name)) for name in names]
        name_row = " ".join(name.ljust(w) for name, w in zip(names, widths))
        block_row = " ".join(
            colour_block(n) + (" " * (w - 2)) for (_, n), w in zip(note_ons, widths)
        )
        print(name_row)
        print(block_row)

    ppq = division if division & 0x8000 == 0 else 480
    beats = last_tick / ppq if ppq else 0
    # Step length: assume 16th-note grid (ppq/4) — matches Gambler's default subdivision.
    step_ticks = max(1, ppq // 4)
    steps = (last_tick + step_ticks - 1) // step_ticks if last_tick else 0
    print(f"length: {steps} steps · {last_tick} ticks · {beats:.2f} beats @ {ppq} PPQ")

main()
PY
