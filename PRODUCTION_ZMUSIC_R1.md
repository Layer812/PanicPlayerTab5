# PanicPlayerTab5 — Z-MUSIC Production R1 release policy / 引継ぎ・リリース基準

Updated: 2026-11-01 JST

> **Status: production candidate / NOT a published firmware release.** This document freezes the implementation policy and reproducible-source requirements. The current repository `main` and existing M5Burner firmware remain unchanged. Do not advertise a new firmware as installed, hardware-qualified, or generally available based on this document.

## 方針 / Scope

- Purpose: play X68000 PANIC movies on M5Stack Tab5 through the authentic PANIC.X v1.38 and Human68k.
- The default music path is the authentic **Z-MUSIC 2.08 resident driver**; run PANIC after detecting the real TRAP #3 `ZmuSiC` signature. Do not substitute a fake register interpreter or special-case `DENKOFM.PAN`.
- The R7 screen-continuity rescue is the visual basis: a moving PAN movie was confirmed on hardware. Preserve the working PAN filer, BACK, PREV/NEXT, REPEAT, and SPACE interactions.
- NORMAL / 44,100 Hz is the release recommendation. GREEN / RED TURBO are optional and not an audio-quality claim.
- Stop FM-direct special-case development and speculative YM2151 / PCM scheduler tuning for this release. R8–R11 audio changes are excluded from the proposed frozen baseline pending independent validation.
- The audible gritty / buzzing FM noise observed in later tests is a **known limitation**, not described as fixed. Works using Z-MUSIC may not all produce clean audio.
- Preserve all PAN authors' distribution rights separately from PANIC.X permissions, the original Z-MUSIC license, and PX68K-derived code rights. The PANIC Web MIT license does not automatically cover this firmware.

## Verified observations versus unverified

| Evidence | Status |
| --- | --- |
| R4 `DENKOZM.PAN`: actual Z-MUSIC 2.08 resident TRAP #3, YM2151 writes and key-ons; NORMAL-mode tune heard | Observed on hardware |
| R7: previously static 256x256 movie started moving | Observed on hardware |
| R7 / R8 / R9: FM melody accompanied by gritty or buzzing noise | Observed on hardware |
| R10: melody vanished, noise only | Observed on hardware — regression, excluded |
| R11: priority-only audio candidate | Build script supplied; hardware outcome not established here |
| Fully reproducible public source tree and Production R1 BIN | **Not yet established** |

These results were observed at different intermediate stages; they **do not** prove that one immutable R7 production binary passes every criterion.

## Fixed source candidate fingerprints

These fingerprints identify the R6/R7 **development source patch files**, not a complete public source release or hardware-tested production binary.

| File | SHA-256 |
| --- | --- |
| R7 `src/main.c` | `bc902883209472fa7b4bcff55886cd0798bc987d01b247b6a13be39031b567cf` |
| R6/R7 `src/tab5_video.cpp` | `3c80c882bb243b4d74564f54ad6c80ec696ff9b8b4bce95110fd15c37adbd871` |
| Intermediate-stage `components/px68k/x68k/hostfs.c` (expected in R6/R7 manifests) | `0f5b9b7ae3b333e044661f990aaf6bbb107a7a2ee4fc948dd0e7948016f9e13e` |
| Original `ZMUSIC208.X` content | SHA-256 `7f066d4dbbe6e07e91463e78005b2cc01a23b3453ba3c3da178474ef1244efdf`; 56,310 bytes |
| `ZMUSIC208.X` source blob | upstream `toyoshim/z-music.js` commit `fd6ad1c1db9edcf973283597e2c95cedf17eb701`, blob `4a4d88b3a620694cad3f27739e5b6a4d70cfbbbf` |
| Base `X68KTab5` submodule pin currently in public R5Y | `b8a5f29c747f4345060b32a34f190f9339d5b577` |

**Important source gap:** Public `main` currently contains the R5Y overlays and pinned X68KTab5 submodule, whereas the R6/R7 ZIPs are *incremental* patches against a later private Z-MUSIC-embedded development stage. That stage also changes other files, including `tab5_panic.cpp/.h`, HostFS, and the embedded driver source. Copying the two R7 source files into `main` alone would be incomplete and potentially break the build. The source must be reconstructed from the full validated integration stage, with legal provenance and build hashes, **before** code merge.

## Production promotion gate / 昇格条件

1. Reconstruct the complete pinned-source integration, including the authentic embedded Z-MUSIC source and modified HostFS/PANIC files. Identify and document every changed source and third-party license; do not include unauthorized PAN works.
2. Perform a reproducible ESP-IDF 5.5.4 native build from that immutable source; record the ELF/BIN SHA-256 and partition layout.
3. One physical confirmation of Z-MUSIC resident + PANIC movie continuous playback + NORMAL audio. Record that buzz is a known limitation if it remains; no further speculative audio experiments are release blockers.
4. Once the above are satisfied, update README JP/EN, release notes, and M5Burner binary only with validated images. Flash method must remain APP-ONLY (0x10000), preserve HumanXDF, bootloader and partition table; no automatic full-chip backup.
5. Keep the existing public R5Y/M5Burner production image intact until a positively identified candidate passes the gate. Do not silently retag it as Z-MUSIC R1.

## Development decision

Release focus is authentic Z-MUSIC-based PAN playback, not an endless pursuit of FM-register direct playback. The user explicitly elected to stop the FM noise optimization cycle, not to claim that noise has been fixed.

**Special Thanks: Nochi.**
