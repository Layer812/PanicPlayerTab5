# PanicPlayerTab5 R5Y — source provenance

The R5Y PANIC player source is kept in `overlays/`. The project uses the
pinned [X68K Tab](https://github.com/Layer812/X68KTab5) Git submodule as the
shared emulator/ESP32-P4 base, not a duplicate of its source tree.

- `overlays/src/`: six R5Y PANIC/player/host UI source files.
- `overlays/components/px68k/x68k/`: three **PX68K-derived** files with
  R5Y's HostFS, palette, and system-port changes. Their original copyright
  and redistribution conditions still apply.
- `SHA256_MANIFEST.json`: byte-for-byte fingerprints for all nine files.
- `scripts/build_from_source.py`: reproduces the successful FIX4 native
  integration by copying the pinned base into an isolated build directory,
  replacing exactly nine files, adding two PANIC sources to CMake, and
  invoking ESP-IDF 5.5.4. It does not flash hardware.

The original FIX4 Windows build (2026-10-25) completed 1,275 build steps,
producing an app binary and passing the 8 MiB app partition size check.
That proves the build integration, **not** exact identity with the previously
hardware-tested R5Y firmware and **not** new hardware validation.

PANIC.X embedded source, system software, fonts and third-party code retain
applicable original terms. This project's MIT license, if adopted later,
would not relicense them. See the project's README and
[X68K Tab third-party notices](https://github.com/Layer812/X68KTab5/blob/main/THIRD_PARTY_NOTICES.md).

SHA-256 values and committed X68K Tab pin are declared in `SHA256_MANIFEST.json`.
Do not edit files in the upstream submodule. All integration takes place in
`F:\build\tmp\PanicPlayerTab5_Public` or a path selected by the user.
