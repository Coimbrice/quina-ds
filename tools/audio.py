#!/usr/bin/env python3
"""Convert the downloaded sounds into the files in audio/.

Usage: audio.py downloads_folder audio_folder

The downloads folder has the Gem Waltz files and the Kenney Interface Sounds
pack extracted next to them. Everything becomes mono 16-bit WAV at a lower
rate to fit in memory, and the music gets a loop point so it repeats without
a gap. Needs sox.
"""

import os
import struct
import subprocess
import sys

KENNEY = "kenney_interface-sounds/wav"

# Source file, output name, sample rate, loops.
SOUNDS = [
    ("gem-waltz.wav", "music.wav", 16000, True),
    ("gem-waltz-cleared.wav", "cleared.wav", 16000, False),
    ("gem-waltz-gameover.wav", "gameover.wav", 16000, False),
    (f"{KENNEY}/click_002.wav", "click_002.wav", 22050, False),
    (f"{KENNEY}/click_003.wav", "click_003.wav", 22050, False),
    (f"{KENNEY}/bong_001.wav", "bong_001.wav", 22050, False),
    (f"{KENNEY}/error_004.wav", "error_004.wav", 22050, False),
    (f"{KENNEY}/maximize_008.wav", "maximize_008.wav", 22050, False),
    (f"{KENNEY}/minimize_008.wav", "minimize_008.wav", 22050, False),
    (f"{KENNEY}/select_001.wav", "select_001.wav", 22050, False),
    (f"{KENNEY}/select_002.wav", "select_002.wav", 22050, False),
]


def add_loop(path, rate):
    # A "smpl" chunk with one loop over the whole sound, which mmutil reads.
    with open(path, "rb") as f:
        data = f.read()
    frames = (len(data) - 44) // 2
    smpl = struct.pack("<9I", 0, 0, 1000000000 // rate, 60, 0, 0, 0, 1, 0)
    smpl += struct.pack("<6I", 0, 0, 0, frames - 1, 0, 0)
    data += b"smpl" + struct.pack("<I", len(smpl)) + smpl
    data = data[:4] + struct.pack("<I", len(data) - 8) + data[8:]
    with open(path, "wb") as f:
        f.write(data)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)

    downloads, audio = sys.argv[1:]
    os.makedirs(audio, exist_ok=True)
    for source, name, rate, loops in SOUNDS:
        output = os.path.join(audio, name)
        subprocess.run(
            [
                "sox",
                os.path.join(downloads, source),
                "-c",
                "1",
                "-r",
                str(rate),
                "-b",
                "16",
                output,
            ],
            check=True,
        )
        if loops:
            add_loop(output, rate)


if __name__ == "__main__":
    main()
