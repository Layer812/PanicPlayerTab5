/*
 * PanicPlayer NEXT R5 shell on current PX68K Production authority.
 * Tab5 PANIC launcher/runtime bridge.
 * Intent: Run the original PANIC.X as a thin shell on the current PX68K
 * Production guest/host authority while keeping PAN selection in native UI.
 * Layer8 Aug/17/2026
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Select a random .PAN anywhere below /sdcard. */
int tab5_panic_select_random(char *out_path, size_t out_size);

/* Native touch filer: recursively scans SD and returns one selected .PAN. */
int tab5_panic_select_file(char *out_path, size_t out_size);
/* Same-folder playback navigation; wraps at folder ends. */
int tab5_panic_adjacent_file(const char *current_path, int delta,
                             char *out_path, size_t out_size);
/* Host-side Repeat UI latch; ON asks CPU1 to pulse SPACE once per second. */
int tab5_panic_repeat_enabled(void);

/* Remove only temporary files created by PANIC integration. Safe before the launcher and before restart. */
void tab5_panic_cleanup_staging(void);

/* Legacy reboot handoff (kept for compatibility with an interrupted older build). */
int tab5_panic_schedule_random_restart(void);
int tab5_panic_take_pending(char *out_path, size_t out_size);

/* Game-screen PANIC button: choose + stage one random PAN without rebooting ESP32-P4. */
int tab5_panic_prepare_random_runtime(char *out_path, size_t out_size);

/* Prepare the embedded player and 8.3-safe staged PAN in the HostFS root. */
int tab5_panic_prepare_runtime(const char *selected_pan_path);

/* Create the short HostFS batch command after the runtime drive letter is known. */
int tab5_panic_prepare_batch(char drive_letter);

/* Playback touch result: 0=none, 1=SPACE, 2=BACK, 3=PREV, 4=NEXT, 5=PANIC QUEST, 6=REPEAT latch changed, 7=TURBO. */
int tab5_panic_poll_playback_touch(void); /* legacy ABI; NEXT uses sampled API below */
int tab5_panic_process_playback_touch_sample(int x, int y, int pressed);
void tab5_panic_reset_touch(void);

/* Called only by the Production CPU0 LCD presenter while it owns display mutex. */
void tab5_panic_present_playback_controls(void);
int tab5_panic_playback_controls_dirty(void);

/* Pointless native 8-bit RPG easter egg. Blocks CPU1 intentionally, pausing guest playback. */
void tab5_panic_run_rpg_interlude(void);

#ifdef __cplusplus
}
#endif
