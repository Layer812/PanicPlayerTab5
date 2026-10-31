<p align="center">
  <img src="000000.png" alt="panicplayer" width="100%">
</p>

# panicplayer

[日本語 / Japanese](README.md)

**panicplayer** is a PANIC player for playing **X68000 PANIC data (`.PAN`)** on the M5Stack Tab5.

PANIC was an animation playback system used on the X68000, combining graphics, sprites, text, and audio.  
The original `panic.x` player and `.PAN` data files were used to create and share many different works.

panicplayer is not intended to be a general-purpose X68000 emulator. It provides an **X68000-compatible environment focused on playing PANIC data** as easily as possible.

## M5Stack Tab5

**M5Stack Tab5** is a portable development terminal built around the ESP32-P4 RISC-V SoC, with a 5-inch 1280×720 IPS touchscreen, 32MB PSRAM, a microSD card slot, and built-in audio hardware.

panicplayer uses that hardware to run a dedicated X68000-compatible PANIC playback environment.

- [M5Stack Tab5 official documentation](https://docs.m5stack.com/en/core/Tab5)

## Install with M5Burner

panicplayer is currently available through **M5Burner**.

- [M5Burner official page](https://docs.m5stack.com/en/uiflow/m5burner/intro)
- [M5Stack official download page](https://docs.m5stack.com/en/download)

**M5Burner Share Code**

```text
nsDMO5hHzVw4nB1U
```

Open M5Burner, use **Share Burn**, and enter the share code above.

## Usage

Put your `.PAN` files on a microSD card, start panicplayer, and choose a PAN file from the built-in file selector.

- Tap a `.PAN` file → Play
- **BACK** → Return to the PAN file selector
- **PREV / NEXT** → Move to the previous / next PAN file
- **REPEAT** → Automatically advances PAN data that waits for SPACE
- **TURBO** → Cycle through NORMAL / GREEN / RED host-load profiles
- Tap the screen during playback → `SPACE`

TURBO does not change the X68000-side playback speed itself. It changes the balance of video/audio processing on the Tab5 side. The selected mode remains active for the rest of the current run.

A sample PAN file, [DENKO1.PAN](https://github.com/Layer812/PanicPlayerTab5/blob/main/DENKO1.PAN), is available.

## Z-MUSIC

The music playback configuration uses **Z-MUSIC SYSTEM v2.08**, the X68000 music driver created by **Zenji Nishikawa (西川善司)**.

See the [Z-MUSIC v2.08 copyright and redistribution notice](docs/ZMUSIC_2_08_NOTICE.md) for original documentation and attribution.

## Looking for a fuller X68000 experience?

panicplayer is intentionally focused on PANIC playback. If you would like to explore a more general-purpose X68000 environment on the M5Stack Tab5, also take a look at **X68K Tab**:

- [X68K Tab](https://github.com/Layer812/X68KTab5)

## PANIC V1.38

panicplayer uses the original **PANIC player (`panic.x` V1.38)** binary as part of its playback environment.

The original PANIC V1.38 documentation is also included in this repository under `docs/`.

Original archive / documentation:

- [X68000 LIBRARY - PANIC](http://retropc.net/x68000/software/movie/panic/panic/)

According to the original V1.38 distribution documents:

- PANIC was originally written by **Hideya Nagata (pako / ぱこたん / 永田英哉)**
- `panic.x` V1.38 is based on pako's V1.34, with modifications by **Nashimi (なしみ)**
- the copyright of PANIC remains with **Hideya Nagata (pako)**
- the original author explicitly permitted use, redistribution, modification, and commercial use of PANIC
- rights and distribution conditions for individual `.PAN` data files belong to their respective creators

Many thanks to **pako**, **Nashimi**, and everyone who developed, documented, distributed, and created data for PANIC.

If you still have PANIC data on an old HDD, MO disk, CD-R, or backup, give it a try with panicplayer.  
If you find a PAN file that does not work, a reproduction description and serial log would be very helpful.

## Current tested build (October 24, 2026)

- **R5Y**: PAN playback and the palette correction have been confirmed on an M5Stack Tab5.
- First-time 256×256 display recovery and replay of another PAN selection in the same session are supported.

## Source Code

R5Y-specific PanicPlayerTab5 sources are available in [overlays/src](overlays/src). The shared X68000 base is the pinned [X68K Tab](https://github.com/Layer812/X68KTab5) Git submodule. The PX68K-derived R5Y changes are in [overlays/components/px68k](overlays/components/px68k). See [source provenance and hashes](SOURCE_PROVENANCE.md).

```bash
git clone --recurse-submodules https://github.com/Layer812/PanicPlayerTab5.git
```

On Windows with ESP-IDF 5.5.4, `RUN_THIS_BUILD_PUBLIC_SOURCE.cmd` builds in an isolated work directory, without flashing. FIX4 previously compiled successfully; a fresh post-publication clone and hardware behavior have not yet been revalidated.

## Special Thanks

**Nochi**

## Copyright / Acknowledgements

- **New code in Panic Player Tab5**: © 2026 Layer812.
- **PANIC / `PANIC.X`**: Original author and copyright holder **Hideya Nagata (pako / ぱこたん)**; V1.38 modifications by **Nashimi (なしみ)**. The original documents and conditions take precedence.
- **Z-MUSIC SYSTEM v2.08**: © Zenji Nishikawa (西川善司). See the [Z-MUSIC rights notice](docs/ZMUSIC_2_08_NOTICE.md).
- **Musashi**: © Karl Stenerud and contributors (MIT License).
- **PX68K / WinX68k / xkeropi**: Thanks to all developers, porters, and hardware researchers in this lineage.
- **Individual `.PAN` works**: Rights to animations, music, and data remain with their respective creators and rights holders.
- **SHARP X68000**: Names and trademarks belong to SHARP and their respective rights holders. This is an independent unofficial project, not endorsed by SHARP.

**Permission to distribute PANIC itself and permission to distribute individual PAN files are separate.** For original and related terms, see [THIRD_PARTY_NOTICES.md](https://github.com/Layer812/PanicPlayerWeb/blob/main/THIRD_PARTY_NOTICES.md), [LICENSE_PANIC_X.txt](https://github.com/Layer812/PanicPlayerWeb/blob/main/LICENSE_PANIC_X.txt), [PANIC_V1.38_NOTICE.txt](https://github.com/Layer812/PanicPlayerWeb/blob/main/PANIC_V1.38_NOTICE.txt), and the [original PANIC documents](https://github.com/Layer812/PanicPlayerTab5/tree/main/docs). Also see the [SHARP/FSHARP terms](LICENSE_SHARP_X68000.txt) and [X68K Tab third-party notices](https://github.com/Layer812/X68KTab5/blob/main/THIRD_PARTY_NOTICES.md).

**The Panic Player Web repository's MIT license covers only Layer812-owned new Web code.** It does not automatically apply to this Tab5 repository or to third-party code, binaries, or PAN files. Original terms of each component take precedence.

Thank you to everyone who has created and preserved the PANIC and X68000 culture.

## Denko (working title) — comic

What if the activities surrounding Panic Player were a project of **Den-no Club (電脳倶楽部)**? This comic explores an imaginary world built around that premise. **Episodes -1 through 4** are available, with **5 through 8** planned.

The **“FM音源で歌うPanic” (FM-singing PANIC)** featured throughout the comic can be watched and played in your browser: **[Play FM音源で歌うPanic](https://layer812.github.io/PanicPlayerWeb/?pan=DENKOSN)**.

Click a thumbnail to open the full-size image.

*This is an independent fictional premise, not an official Den-no Club project or publication.*

<table>
<tr>
  <td align="center" width="33%"><a href="./denko-1.png"><img src="./denko-1.png" width="185" alt="Denko (working title), comic -1"></a><p><strong>-1</strong></p></td>
  <td align="center" width="33%"><a href="./denko0.png"><img src="./denko0.png" width="185" alt="Denko (working title), comic 0"></a><p><strong>0</strong></p></td>
  <td align="center" width="33%"><a href="./denko1.png"><img src="./denko1.png" width="185" alt="Denko (working title), comic 1"></a><p><strong>1</strong></p></td>
</tr>
<tr>
  <td align="center" width="33%"><a href="./denko2.png"><img src="./denko2.png" width="185" alt="Denko (working title), comic 2"></a><p><strong>2</strong></p></td>
  <td align="center" width="33%"><a href="./denko3.png"><img src="./denko3.png" width="185" alt="Denko (working title), comic 3"></a><p><strong>3</strong></p></td>
  <td align="center" width="33%"><a href="./denko4.png"><img src="./denko4.png" width="185" alt="Denko (working title), comic 4"></a><p><strong>4</strong></p></td>
</tr>
</table>

The comics are separate works; permissions for PANIC.X and .PAN files do not automatically cover these images.
