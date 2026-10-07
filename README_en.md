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
Kr4WZqevzTJSB3TF
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

## Source Code

Source code will be published later.

Some parts are still being tuned, so the M5Burner build is currently the easiest way to try panicplayer.

## Special Thanks

**Nochi**

## Copyright / Acknowledgements

### SHARP X68000

The X68000 computer platform was developed by **Sharp Corporation**.

panicplayer uses X68000 system software made available through the **SHARP PRODUCTS USERS FORUM (FSHARP)** under its original distribution terms. The applicable original license text is included with this repository as:

[`LICENSE_SHARP_X68000.txt`](LICENSE_SHARP_X68000.txt)

Please refer to that document for the original terms and conditions. panicplayer is distributed free of charge.

Original SHARP software library:

- [X68000 LIBRARY - SHARP software](http://retropc.net/x68000/software/sharp/)

SHARP, X68000, and related software, names, and trademarks remain the property of Sharp Corporation and/or their respective rights holders.

panicplayer is an unofficial personal project and is **not affiliated with, sponsored by, or endorsed by Sharp Corporation**.

### PANIC

PANIC / `panic.x` copyright remains with **Hideya Nagata (pako / 永田英哉)** as stated in the original distribution documents.

`panic.x` V1.38 includes modifications by **Nashimi (なしみ)**.

The original PANIC documentation distributed with V1.38 is included in this repository so that the original authors' notes and distribution conditions remain available alongside the project.

### Musashi

panicplayer uses **Musashi**, Karl Stenerud's portable Motorola 680x0 emulation engine, for 68000 CPU emulation.

Many thanks to **Karl Stenerud** and the Musashi contributors.

- [Musashi official repository](https://github.com/kstenerud/Musashi)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

The original Musashi copyright and permission notice is reproduced in `THIRD_PARTY_NOTICES.md`.

### PX68K

Parts of panicplayer's X68000 compatibility implementation were developed with reference to **PX68K** and its documented hardware behavior.

Many thanks to **hissorii** and all of the authors and contributors in the WinX68k / xkeropi / PX68K lineage whose work made X68000 emulation and documentation available to later projects.

- [PX68K official repository](https://github.com/hissorii/px68k)
- [Third-party notices](THIRD_PARTY_NOTICES.md)

panicplayer is an independent project and is not an official PX68K port.

### M5Stack

M5Stack, M5Stack Tab5, and M5Burner are products/services of **M5Stack Technology Co., Ltd.**  
panicplayer is an independent project and is not affiliated with M5Stack.
