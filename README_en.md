<p align="center">
  <img src="000000.png" alt="panicplayer" width="100%">
</p>

# panicplayer

[日本語 / Japanese](README.md)

I suddenly felt like watching some old **X68000 PANIC** animations again, so I made a player.

I've heard the retro-PC scene can be a slightly scary place sometimes... 😅  
so, just to be clear:

**This is only a PANIC player!**

It's not intended to be a general-purpose X68000 emulator.  
It implements just enough of an X68000-compatible environment to play `.PAN` files on an **M5Stack Tab5**.

There is, however, one small problem.

**I only have one PANIC file.**

I'm pretty sure there used to be tons of them...

So if you have some old PANIC data hiding on an HDD, MO disk, CD-R, or backup somewhere, please give it a try.

And if you happen to have a PANIC file that **doesn't work** with panicplayer...

**I'd be very happy if you quietly sent it my way. :)**

I'll see if I can make it work.

Old PANIC files you made yourself, files downloaded from some long-forgotten BBS, mysterious files found on an old MO disk — even just information about them would be very welcome.

It would be fun if this little player helped rediscover a few PANIC files that have been hiding for the last 30 years.

## What is M5Stack Tab5?

**M5Stack Tab5** is a portable development terminal built around the **ESP32-P4** RISC-V SoC. It has a **5-inch 1280×720 IPS touchscreen**, **32MB PSRAM**, a **microSD card slot**, and built-in audio hardware including a speaker.

That combination turned out to be a rather nice fit for running a small, dedicated X68000-compatible PANIC playback environment.

- [M5Stack Tab5 official documentation](https://docs.m5stack.com/en/core/Tab5)

## Install with M5Burner

panicplayer is currently available through **M5Burner**.

- [M5Burner official page](https://docs.m5stack.com/en/uiflow/m5burner/intro)
- [M5Stack official download page](https://docs.m5stack.com/en/download)

**M5Burner Share Code**

```text
PDax1LcxZRfUhJyp
```

Open M5Burner, use **Share Burn**, and enter the share code above.

## Usage

Put your `.PAN` files on an SD card and start panicplayer.

Choose a PANIC file from the built-in file selector to play it.

- Tap a `.PAN` file → Play
- Tap the screen during playback → `SPACE`
- Long-press the upper-left corner → Return to the file selector

## Source Code

Source code will be published later.

I'm still changing things quite a bit, so for now the M5Burner build is the easiest way to try it.

## Looking for PANIC Data

Seriously, this is the part I need help with.

If you have old X68000 media lying around, I'd love to hear about:

- `.PAN` files
- LZH/ZIP archives containing PANIC data
- old MO / HDD / CD-R backups
- BBS file lists
- README or DOC files
- filenames you remember
- even "I think I saw that on some BBS..." stories

If redistribution is difficult because of copyright, **a filename or directory listing alone is still very useful**.

PANIC data itself belongs to whoever created it, so please respect the original author's wishes.

## PANIC V1.38

panicplayer uses the original **PANIC player (`panic.x` V1.38)** binary as part of its playback environment.

The original PANIC V1.38 documentation is included in this repository for reference.  
Please read the original documents for the full history, usage notes, and distribution terms.

Original archive / documentation:

- [X68000 LIBRARY - PANIC](http://retropc.net/x68000/software/movie/panic/panic/)

According to the original V1.38 distribution documents:

- PANIC was originally written by **Hideya Nagata (pako / ぱこたん / 永田英哉)**
- `panic.x` V1.38 is based on pako's `panic.x` V1.34, with modifications by **Nashimi (なしみ)**
- the copyright of PANIC remains with **Hideya Nagata (pako)**
- the original author explicitly permitted use, redistribution, modification, and commercial use of PANIC
- rights and distribution conditions for individual `.PAN` data files belong to their respective creators

Many thanks to **pako**, **Nashimi**, and everyone who developed, documented, distributed, and created data for PANIC.

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

---

**Somewhere out there, an old MO disk is probably still hiding a few `.PAN` files.**
