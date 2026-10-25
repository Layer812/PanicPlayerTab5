/*
 * Tab5 port-specific PANIC integration.
 * Intent: Keep the original PanicPlayer user experience (PAN-only recursive
 * filer, tap=SPACE, upper-left hold=menu) while executing PANIC.X on the
 * already validated PX68K 6.00 X68000 runtime instead of carrying a second
 * emulator/audio/video stack.
 * Layer8 Aug/17/2026
 */
#include "tab5_panic.h"

#include <M5Unified.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panic_x_bin.h"
#include "tab5_audio.h"
#include "tab5_video.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <unistd.h>

static const char *TAG = "TAB5_PANIC";
static const char *const kPanicNextMarker =
    "PANICPLAYER_NEXT_R5_PRODUCTION_SHELL_20261007";
static const char *const kPanScanV4Marker = "PANICPLAYER_R1_PANSCAN_V4_EXACT_20261007";

namespace {
constexpr const char *kSdRoot = "/sdcard";
constexpr const char *kPanRoot = "/sdcard/pan"; // PANICPLAYER_R1_PRODUCTION_FINAL_20261007
constexpr const char *kPlayerHostPath = "/sdcard/P68K.X";
constexpr const char *kPanStageHostPath = "/sdcard/P68K.PAN";
constexpr const char *kBatchHostPath = "/sdcard/P68K.BAT";
constexpr const char *kLegacyRootPlayPath = "/sdcard/PLAY.PAN";
constexpr const char *kLegacyPlayPath = "/sdcard/PX68K/PLAY.PAN";
constexpr const char *kLegacyPlayerPath = "/sdcard/PX68K/PANIC.X";
constexpr const char *kPendingRestartPath = "/sdcard/P68K.NXT";
// PANICPLAYER_R1_20261006: persistent PAN library cache.
// PANICPLAYER_R1_FINAL: use an 8.3-safe cache name so cache creation also
// works when FAT long filenames are unavailable; the former hidden long name
// produced errno=EINVAL on this device.
constexpr const char *kCachePath = "/sdcard/PANCACHE.DAT";
constexpr const char *kCacheMagic = "PANICPLAYER_CACHE_V5_VIRTUAL_ROOT";
constexpr size_t kMaxEntries = 2048;
constexpr size_t kDirQueueEntries = 2048; // PANSCAN V4: PSRAM-backed BFS queue.
constexpr int kPaneRows = 6;
constexpr int kPaneTop = 116;
constexpr int kPaneRowH = 76;
constexpr int kPaneBottom = kPaneTop + kPaneRows * kPaneRowH;

constexpr int kPaneMargin = 12;
constexpr int kPaneGap = 12;
constexpr int kFolderPaneX = kPaneMargin;
constexpr int kFolderPaneW = 622;
constexpr int kFilePaneX = kFolderPaneX + kFolderPaneW + kPaneGap;
constexpr int kFilePaneW = 622;

constexpr int kPagerY = 590;
constexpr int kPagerH = 112;

constexpr int kTopButtonY = 12;
constexpr int kTopButtonH = 58;
constexpr int kRefreshX = 770;
constexpr int kRefreshW = 200;
constexpr int kTopVolMinusX = 980;
constexpr int kTopVolMinusW = 135;
constexpr int kTopVolPlusX = 1125;
constexpr int kTopVolPlusW = 135;

constexpr int kPlaybackSideW = 160;
constexpr int kPlaybackLeftX = 12;
constexpr int kPlaybackRightX = 1132;
constexpr int kPlaybackButtonW = 136;
constexpr int kPlaybackBackY = 18;
constexpr int kPlaybackBackH = 60;
constexpr int kPlaybackVolPlusY = 94;
constexpr int kPlaybackVolMinusY = 164;
constexpr int kPlaybackRepeatY = 234;
constexpr int kPlaybackTurboY = 146;
constexpr int kPlaybackSmallH = 58;
constexpr int kPlaybackPrevNextY = 628;
constexpr int kPlaybackPrevNextH = 70;
constexpr int kPlaybackLogoY = 18;
constexpr int kPlaybackLogoH = 112;

struct PanEntry {
    char full_path[512];
    char rel_path[384];
    uint32_t size;
};

struct FolderEntry {
    char rel_dir[384];   /* Immediate child name, or ".." when not at root. */
};

static bool s_touch_active = false;
static bool s_touch_started_in_corner = false;
static bool s_touch_started_volm = false;
static bool s_touch_started_volp = false;
static bool s_touch_started_prev = false;
static bool s_touch_started_next = false;
static bool s_touch_started_repeat = false;
static bool s_touch_started_turbo = false;
static bool s_touch_started_logo = false;
static bool s_touch_action_fired = false;
static bool s_touch_started_side_chrome = false;
static bool s_playback_repeat_enabled = false;
static int64_t s_touch_start_us = 0;
static int64_t s_playback_controls_last_draw_us = 0;

/* PANFS readiness is proven by real VFS/FatFS read+write, not by elapsed time. */
static bool s_panfs_gate_passed = false;
static char s_panfs_gate_root[512] = {};
static bool s_panfs_entry_probe_done = false;

static bool point_in_play_rect(int x,int y,int rx,int ry,int rw,int rh)
{
    return x>=rx && x<rx+rw && y>=ry && y<ry+rh;
}

static void draw_panic_player_logo(int x, int y, int w, int h)
{
    auto &d = M5.Display;
    constexpr uint16_t kLogoRed = 0xD800;
    d.fillRect(x, y, w, h, kLogoRed);
    d.drawRect(x, y, w, h, TFT_WHITE);
    d.drawRect(x + 2, y + 2, w - 4, h - 4, TFT_WHITE);
    d.setFont(&fonts::efontJA_16);
    d.setTextDatum(textdatum_t::middle_center);
    d.setTextColor(TFT_WHITE, kLogoRed);
    d.setTextSize(2);
    d.drawString("PANIC", x + w / 2, y + h / 2 - 24);
    d.drawString("PLAYER", x + w / 2, y + h / 2 + 16);
}

static void draw_play_button(int x, int y, int w, int h,
                             const char *label, bool inverted)
{
    auto &d = M5.Display;
    const uint16_t bg = inverted ? TFT_WHITE : TFT_BLACK;
    const uint16_t fg = inverted ? TFT_BLACK : TFT_WHITE;
    d.fillRect(x, y, w, h, bg);
    d.drawRect(x, y, w, h, TFT_WHITE);
    /* Playback controls are ASCII: use the compact built-in font at 3x
     * instead of efontJA_16 at 1x so BACK/VOL/REPEAT/PREV/NEXT are legible. */
    d.setFont(nullptr);
    d.setTextDatum(textdatum_t::middle_center);
    d.setTextColor(fg, bg);
    d.setTextSize(3);
    d.drawString(label, x + w / 2, y + h / 2);
}

static void draw_playback_turbo_button()
{
    auto &d = M5.Display;
    const uint32_t mode = tab5_video_turbo_mode();
    const uint16_t bg = mode >= 2u ? 0x7800 : (mode == 1u ? 0x03E0 : TFT_BLACK);
    const uint16_t edge = mode >= 2u ? 0xF800 : (mode == 1u ? 0x07E0 : TFT_WHITE);
    const char *state = mode >= 2u ? "RED" : (mode == 1u ? "GREEN" : "NORMAL");
    d.fillRect(kPlaybackLeftX, kPlaybackTurboY, kPlaybackButtonW, kPlaybackSmallH, bg);
    d.drawRect(kPlaybackLeftX, kPlaybackTurboY, kPlaybackButtonW, kPlaybackSmallH, edge);
    d.setFont(nullptr);
    d.setTextDatum(textdatum_t::middle_center);
    d.setTextColor(TFT_WHITE, bg);
    d.setTextSize(2);
    d.drawString("TURBO", kPlaybackLeftX + kPlaybackButtonW / 2, kPlaybackTurboY + 19);
    d.setTextSize(1);
    d.drawString(state, kPlaybackLeftX + kPlaybackButtonW / 2, kPlaybackTurboY + 43);
}

static void draw_playback_controls_if_due()
{
    /* AVFAST R1H: side bars persist outside the 960px PAN viewport. Redraw
     * only after reset/state change; the old 500 ms refresh performed large
     * M5GFX writes on CPU1 and periodically stole guest/audio time. */
    if (s_playback_controls_last_draw_us) return;
    const int64_t now = esp_timer_get_time();

    auto &d = M5.Display;

    /* PANIC owns both 160-pixel side bars.  Clearing both here removes stale
     * normal-game chrome, which previously made controls appear on both sides. */
    d.fillRect(0, 0, kPlaybackSideW, 720, TFT_BLACK);
    d.fillRect(1120, 0, kPlaybackSideW, 720, TFT_BLACK);

    draw_panic_player_logo(kPlaybackLeftX, kPlaybackLogoY,
                           kPlaybackButtonW, kPlaybackLogoH);

    draw_play_button(kPlaybackRightX, kPlaybackBackY,
                     kPlaybackButtonW, kPlaybackBackH, "BACK", false);
    draw_play_button(kPlaybackRightX, kPlaybackVolPlusY,
                     kPlaybackButtonW, kPlaybackSmallH, "VOL+", false);
    draw_play_button(kPlaybackRightX, kPlaybackVolMinusY,
                     kPlaybackButtonW, kPlaybackSmallH, "VOL-", false);
    draw_play_button(kPlaybackRightX, kPlaybackRepeatY,
                     kPlaybackButtonW, kPlaybackSmallH, "REPEAT",
                     __atomic_load_n(&s_playback_repeat_enabled, __ATOMIC_ACQUIRE));
    draw_playback_turbo_button();

    draw_play_button(kPlaybackLeftX, kPlaybackPrevNextY,
                     kPlaybackButtonW, kPlaybackPrevNextH, "PREV", false);
    draw_play_button(kPlaybackRightX, kPlaybackPrevNextY,
                     kPlaybackButtonW, kPlaybackPrevNextH, "NEXT", false);

    d.setTextDatum(textdatum_t::top_left);
    s_playback_controls_last_draw_us = now;
}

static bool ends_with_pan(const char *name)
{
    if (!name) return false;
    const size_t n = std::strlen(name);
    return n >= 4 && strcasecmp(name + n - 4, ".PAN") == 0;
}

static int entry_compare(const void *aa, const void *bb)
{
    const auto *a = static_cast<const PanEntry *>(aa);
    const auto *b = static_cast<const PanEntry *>(bb);
    return strcasecmp(a->rel_path, b->rel_path);
}

static PanEntry *alloc_entries(size_t cap = kMaxEntries)
{
    if (!cap || cap > kMaxEntries) return nullptr;
    auto *p = static_cast<PanEntry *>(heap_caps_calloc(
        cap, sizeof(PanEntry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!p)
        p = static_cast<PanEntry *>(std::calloc(cap, sizeof(PanEntry)));
    return p;
}

static bool resolve_pan_root(char *out, size_t out_size)
{
    if (!out || out_size == 0) return false;
    out[0] = 0;

    DIR *root = opendir(kSdRoot);
    if (!root) {
        ESP_LOGE(TAG, "PANSCAN_V4: opendir(%s) failed errno=%d", kSdRoot, errno);
        return false;
    }

    struct dirent *de = nullptr;
    while ((de = readdir(root)) != nullptr) {
        if (strcasecmp(de->d_name, "pan") != 0) continue;

        char full[512];
        const int n = std::snprintf(full, sizeof(full), "%s/%s", kSdRoot, de->d_name);
        if (n <= 0 || n >= (int)sizeof(full)) continue;

        DIR *probe = opendir(full);
        if (!probe) continue;
        closedir(probe);

        std::snprintf(out, out_size, "%s", full);
        closedir(root);
        ESP_LOGI(TAG, "PANSCAN_V4: library root resolved=%s", out);
        return true;
    }
    closedir(root);

    /* Also try the configured spelling directly. */
    DIR *probe = opendir(kPanRoot);
    if (probe) {
        closedir(probe);
        std::snprintf(out, out_size, "%s", kPanRoot);
        ESP_LOGI(TAG, "PANSCAN_V4: library root direct=%s", out);
        return true;
    }

    ESP_LOGE(TAG, "PANSCAN_V4: no root-level PAN/pan directory under %s", kSdRoot);
    return false;
}

static bool panfs_ready_gate(char *out_root, size_t out_size)
{
    if (!out_root || out_size == 0) return false;
    out_root[0] = 0;

    if (s_panfs_gate_passed) {
        std::snprintf(out_root, out_size, "%s", s_panfs_gate_root);
        return true;
    }

    constexpr const char *kReadyProbe = "/sdcard/PPRDY.TMP";
    constexpr uint8_t kMagic[4] = { 'P', 'P', 'R', '1' };

    for (unsigned attempt = 1; attempt <= 40; ++attempt) {
        bool root_ok = false;
        bool rw_ok = false;
        bool pan_ok = false;
        int root_errno = 0;
        int write_errno = 0;
        int read_errno = 0;

        errno = 0;
        DIR *root = opendir(kSdRoot);
        root_errno = errno;
        if (root) {
            root_ok = true;
            closedir(root);
        }

        unlink(kReadyProbe);
        errno = 0;
        FILE *wf = std::fopen(kReadyProbe, "wb");
        write_errno = errno;
        if (wf) {
            const bool wrote = std::fwrite(kMagic, 1, sizeof(kMagic), wf) == sizeof(kMagic);
            const int close_rc = std::fclose(wf);

            if (wrote && close_rc == 0) {
                uint8_t got[sizeof(kMagic)] = {};
                errno = 0;
                FILE *rf = std::fopen(kReadyProbe, "rb");
                read_errno = errno;
                if (rf) {
                    const size_t nr = std::fread(got, 1, sizeof(got), rf);
                    const int rclose = std::fclose(rf);
                    rw_ok = nr == sizeof(got) && rclose == 0 &&
                            std::memcmp(got, kMagic, sizeof(got)) == 0;
                }
            }
        }
        unlink(kReadyProbe);

        char resolved[512] = {};
        if (root_ok && rw_ok && resolve_pan_root(resolved, sizeof(resolved))) {
            errno = 0;
            DIR *pd = opendir(resolved);
            if (pd) {
                pan_ok = true;
                closedir(pd);
            }
        }

        if (root_ok && rw_ok && pan_ok) {
            s_panfs_gate_passed = true;
            std::snprintf(s_panfs_gate_root, sizeof(s_panfs_gate_root), "%s", resolved);
            std::snprintf(out_root, out_size, "%s", resolved);
            ESP_LOGI(TAG,
                     "PANFS_GATE PASS attempt=%u root=OK rw=OK pan=OK resolved=%s marker=PANICPLAYER_R1_PANFS_GATE_DIAG_20261007",
                     attempt, resolved);
            return true;
        }

        ESP_LOGW(TAG,
                 "PANFS_GATE WAIT attempt=%u root=%s(root_errno=%d) rw=%s(write_errno=%d read_errno=%d) pan=%s",
                 attempt,
                 root_ok ? "OK" : "NO", root_errno,
                 rw_ok ? "OK" : "NO", write_errno, read_errno,
                 pan_ok ? "OK" : "NO");

        vTaskDelay(pdMS_TO_TICKS(50) ? pdMS_TO_TICKS(50) : 1);
    }

    ESP_LOGE(TAG, "PANFS_GATE FAIL after 40 readiness probes");
    return false;
}

static void panfs_probe_entries_once(const char *scan_root)
{
    if (s_panfs_entry_probe_done || !scan_root || !scan_root[0]) return;
    s_panfs_entry_probe_done = true;

    DIR *dir = opendir(scan_root);
    if (!dir) {
        ESP_LOGE(TAG, "PANFS_PROBE: opendir root failed path=%s errno=%d",
                 scan_root, errno);
        return;
    }

    unsigned index = 0;
    struct dirent *de = nullptr;
    while ((de = readdir(dir)) != nullptr) {
        if (!std::strcmp(de->d_name, ".") || !std::strcmp(de->d_name, ".."))
            continue;

        ++index;
        char full[512];
        const int n = std::snprintf(full, sizeof(full), "%s/%s",
                                    scan_root, de->d_name);
        if (n <= 0 || n >= (int)sizeof(full)) {
            ESP_LOGW(TAG, "PANFS_PROBE #%u name='%s' PATH_TOO_LONG",
                     index, de->d_name);
            continue;
        }

        errno = 0;
        struct stat st = {};
        const int stat_rc = stat(full, &st);
        const int stat_errno = errno;
        const unsigned long mode = stat_rc == 0 ? (unsigned long)st.st_mode : 0ul;
        const int is_dir = stat_rc == 0 && S_ISDIR(st.st_mode);
        const int is_reg = stat_rc == 0 && S_ISREG(st.st_mode);

        errno = 0;
        DIR *sub = opendir(full);
        const int opendir_errno = errno;
        const int opendir_ok = sub != nullptr;
        if (sub) closedir(sub);

        errno = 0;
        FILE *fp = std::fopen(full, "rb");
        const int fopen_errno = errno;
        const int fopen_ok = fp != nullptr;
        if (fp) std::fclose(fp);

        ESP_LOGI(TAG,
                 "PANFS_PROBE #%u name='%s' dtype=%u panext=%d stat=%d stat_errno=%d mode=0x%lx isdir=%d isreg=%d opendir=%d opendir_errno=%d fopen=%d fopen_errno=%d",
                 index, de->d_name, (unsigned)de->d_type,
                 ends_with_pan(de->d_name) ? 1 : 0,
                 stat_rc, stat_errno, mode, is_dir, is_reg,
                 opendir_ok, opendir_errno, fopen_ok, fopen_errno);
    }
    closedir(dir);

    ESP_LOGI(TAG, "PANFS_PROBE complete root=%s entries=%u",
             scan_root, index);
}

static bool pan_entry_rel_exists(const PanEntry *entries, size_t count,
                                 const char *rel)
{
    for (size_t i=0; i<count; ++i)
        if (!strcasecmp(entries[i].rel_path, rel)) return true;
    return false;
}

static bool add_pan_entry(PanEntry *entries, size_t cap, size_t &count,
                          const char *full, const char *rel)
{
    if (!entries || count >= cap || !full || !rel || !rel[0]) return false;
    if (pan_entry_rel_exists(entries, count, rel)) {
        ESP_LOGW(TAG, "PANSCAN_V5: duplicate virtual path skipped rel=%s full=%s",
                 rel, full);
        return false;
    }

    FILE *fp = std::fopen(full, "rb");
    if (!fp) {
        ESP_LOGW(TAG, "PANSCAN_V5: PAN fopen failed path=%s errno=%d", full, errno);
        return false;
    }

    uint32_t fsize = 0;
    if (std::fseek(fp, 0, SEEK_END) == 0) {
        const long sz = std::ftell(fp);
        if (sz > 0) fsize = (uint32_t)sz;
    }
    std::fclose(fp);

    PanEntry &e = entries[count++];
    std::snprintf(e.full_path, sizeof(e.full_path), "%s", full);
    std::snprintf(e.rel_path, sizeof(e.rel_path), "%s", rel);
    e.size = fsize;

    if (count <= 12) {
        ESP_LOGI(TAG, "PANSCAN_V5: found #%u rel=%s full=%s (%u bytes)",
                 (unsigned)count, e.rel_path, e.full_path, (unsigned)e.size);
    }
    return true;
}

static size_t scan_pan_library(PanEntry *entries, size_t cap)
{
    if (!entries || !cap) return 0;

    char scan_root[512] = {};
    if (!panfs_ready_gate(scan_root, sizeof(scan_root))) return 0;
    panfs_probe_entries_once(scan_root);
    const size_t pan_root_len = std::strlen(scan_root);

    size_t count = 0;
    unsigned sd_root_pan = 0;

    /* Virtual "/" merges:
     *   - .PAN files directly under /sdcard
     *   - the contents of /sdcard/PAN (or actual-case equivalent)
     * This preserves the user's three historical root PAN files while keeping
     * category folders below PAN as immediate virtual-root children. */
    DIR *sd = opendir(kSdRoot);
    if (sd) {
        struct dirent *de = nullptr;
        while ((de = readdir(sd)) != nullptr && count < cap) {
            if (!std::strcmp(de->d_name, ".") || !std::strcmp(de->d_name, ".."))
                continue;
            if (!ends_with_pan(de->d_name)) continue;

            char full[512];
            const int n = std::snprintf(full, sizeof(full), "%s/%s",
                                        kSdRoot, de->d_name);
            if (n <= 0 || n >= (int)sizeof(full)) continue;
            if (!strcasecmp(full, kPanStageHostPath) ||
                !strcasecmp(full, kLegacyRootPlayPath)) continue;

            if (add_pan_entry(entries, cap, count, full, de->d_name))
                ++sd_root_pan;
        }
        closedir(sd);
    } else {
        ESP_LOGW(TAG, "PANSCAN_V5: opendir(%s) failed errno=%d", kSdRoot, errno);
    }

    using DirPath = char[512];
    auto *queue = static_cast<DirPath *>(heap_caps_malloc(
        kDirQueueEntries * sizeof(DirPath), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!queue)
        queue = static_cast<DirPath *>(std::malloc(kDirQueueEntries * sizeof(DirPath)));
    if (!queue) {
        ESP_LOGE(TAG, "PANSCAN_V5: directory queue allocation failed");
        return count;
    }

    size_t q_head = 0;
    size_t q_tail = 0;
    std::snprintf(queue[q_tail++], sizeof(queue[0]), "%s", scan_root);

    unsigned dirs_opened = 0;
    unsigned entries_seen = 0;
    unsigned queue_drop = 0;

    ESP_LOGI(TAG,
             "PANSCAN_V5: virtual-root begin sd_root_pan=%u pan_root=%s dir_cap=%u pan_cap=%u",
             sd_root_pan, scan_root,
             (unsigned)kDirQueueEntries, (unsigned)cap);

    while (q_head < q_tail && count < cap) {
        char dir_path[512];
        std::snprintf(dir_path, sizeof(dir_path), "%s", queue[q_head++]);

        DIR *dir = opendir(dir_path);
        if (!dir) {
            ESP_LOGW(TAG, "PANSCAN_V5: opendir failed path=%s errno=%d",
                     dir_path, errno);
            continue;
        }
        ++dirs_opened;

        struct dirent *de = nullptr;
        while ((de = readdir(dir)) != nullptr && count < cap) {
            if (!std::strcmp(de->d_name, ".") ||
                !std::strcmp(de->d_name, "..") ||
                de->d_name[0] == '.') continue;
            ++entries_seen;

            char full[512];
            const int n = std::snprintf(full, sizeof(full), "%s/%s",
                                        dir_path, de->d_name);
            if (n <= 0 || n >= (int)sizeof(full)) continue;

            DIR *sub = opendir(full);
            if (sub) {
                closedir(sub);
                if (q_tail < kDirQueueEntries) {
                    std::snprintf(queue[q_tail++], sizeof(queue[0]), "%s", full);
                } else {
                    ++queue_drop;
                    ESP_LOGW(TAG, "PANSCAN_V5: directory queue full skipped=%s", full);
                }
                continue;
            }

            if (!ends_with_pan(de->d_name)) continue;
            if (!strcasecmp(full, kPanStageHostPath) ||
                !strcasecmp(full, kLegacyRootPlayPath) ||
                !strcasecmp(full, kLegacyPlayPath)) continue;

            const char *rel = full;
            if (!std::strncmp(full, scan_root, pan_root_len)) {
                rel = full + pan_root_len;
                if (*rel == '/') ++rel;
            }
            const char *rel_src = *rel ? rel : de->d_name;
            (void)add_pan_entry(entries, cap, count, full, rel_src);
        }
        closedir(dir);

        if (((dirs_opened + entries_seen) & 31u) == 0u) vTaskDelay(1);
    }

    qsort(entries, count, sizeof(entries[0]), entry_compare);
    std::free(queue);

    ESP_LOGI(TAG,
             "PANSCAN_V5: complete pan=%u sd_root_pan=%u dirs=%u entries=%u queue_drop=%u",
             (unsigned)count, sd_root_pan, dirs_opened, entries_seen, queue_drop);
    return count;
}

static bool cache_rel_path_safe(const char *rel)
{
    if (!rel || !rel[0] || rel[0] == '/' || std::strchr(rel, '\t') ||
        std::strchr(rel, '\r') || std::strchr(rel, '\n')) return false;
    if (!std::strcmp(rel, "..") || !std::strncmp(rel, "../", 3) ||
        std::strstr(rel, "/../")) return false;
    return ends_with_pan(rel);
}

static bool cache_full_path_safe(const char *full)
{
    if (!full || std::strncmp(full, "/sdcard/", 8) ||
        std::strchr(full, '\t') || std::strchr(full, '\r') ||
        std::strchr(full, '\n') || !ends_with_pan(full)) return false;
    if (!strcasecmp(full, kPanStageHostPath) ||
        !strcasecmp(full, kLegacyRootPlayPath) ||
        !strcasecmp(full, kLegacyPlayPath)) return false;
    return true;
}

static bool save_pan_cache(const PanEntry *entries, size_t count)
{
    FILE *fp = std::fopen(kCachePath, "wb");
    if (!fp) {
        ESP_LOGW(TAG, "PAN cache create failed errno=%d", errno);
        return false;
    }

    bool ok = std::fprintf(fp, "%s\n", kCacheMagic) > 0;
    for (size_t i=0; ok && i<count; ++i) {
        if (!cache_full_path_safe(entries[i].full_path) ||
            !cache_rel_path_safe(entries[i].rel_path)) continue;
        ok = std::fprintf(fp, "%lu\t%s\t%s\n",
                          (unsigned long)entries[i].size,
                          entries[i].full_path,
                          entries[i].rel_path) > 0;
    }

    if (std::fclose(fp) != 0) ok=false;
    if (!ok) unlink(kCachePath);
    ESP_LOGI(TAG, "PAN cache %s V5: %u entries",
             ok ? "saved" : "FAILED", (unsigned)count);
    return ok;
}

static size_t load_pan_cache(PanEntry *entries, size_t cap)
{
    FILE *fp=std::fopen(kCachePath,"rb");
    if (!fp) return 0;

    char line[1200]={};
    if (!std::fgets(line,sizeof(line),fp)) {
        std::fclose(fp);
        return 0;
    }
    line[std::strcspn(line,"\r\n")]=0;
    if (std::strcmp(line,kCacheMagic)!=0) {
        std::fclose(fp);
        ESP_LOGI(TAG,"PANSCAN_V5: stale cache ignored");
        return 0;
    }

    size_t count=0;
    while (count<cap && std::fgets(line,sizeof(line),fp)) {
        line[std::strcspn(line,"\r\n")]=0;

        char *tab1=std::strchr(line,'\t');
        if (!tab1) continue;
        *tab1++=0;

        char *tab2=std::strchr(tab1,'\t');
        if (!tab2) continue;
        *tab2++=0;

        char *tail=nullptr;
        unsigned long sz=std::strtoul(line,&tail,10);
        if (!tail || *tail) continue;

        const char *full=tab1;
        const char *rel=tab2;
        if (!cache_full_path_safe(full) || !cache_rel_path_safe(rel)) continue;

        PanEntry &e=entries[count++];
        std::snprintf(e.full_path,sizeof(e.full_path),"%s",full);
        std::snprintf(e.rel_path,sizeof(e.rel_path),"%s",rel);
        e.size=(uint32_t)sz;
    }
    std::fclose(fp);

    qsort(entries,count,sizeof(entries[0]),entry_compare);
    ESP_LOGI(TAG,"PANSCAN_V5: cache loaded=%u virtual_root=/",
             (unsigned)count);
    return count;
}

static size_t rebuild_pan_cache(PanEntry *entries,size_t cap)
{
    ESP_LOGI(TAG,"PANSCAN_V5: REBUILD requested");
    size_t count=scan_pan_library(entries,cap);
    (void)save_pan_cache(entries,count);
    ESP_LOGI(TAG,"PANSCAN_V5: REBUILD complete=%u",(unsigned)count);
    return count;
}

static size_t load_or_rebuild_pan_cache(PanEntry *entries,size_t cap)
{
    char ready_root[512] = {};
    if (!panfs_ready_gate(ready_root, sizeof(ready_root))) {
        ESP_LOGE(TAG, "PANFS_GATE: cache/scan start blocked because FatFS is not ready");
        return 0;
    }
    panfs_probe_entries_once(ready_root);
    size_t count=load_pan_cache(entries,cap);
    return count ? count : rebuild_pan_cache(entries,cap);
}

/* R5H: playback BACK opens the same native filer after PX68K has consumed
 * most of the large runtime arenas.  R5G always allocated kMaxEntries for
 * both PanEntry and FolderEntry (about 2.6 MiB) even when PANCACHE.DAT held
 * only a small library.  Cold boot had enough memory, runtime BACK often did
 * not.  Count the validated cache rows first and allocate only the bounded
 * amount actually needed.  No PAN path/order semantics change. */
static size_t pan_cache_entry_count_hint()
{
    FILE *fp = std::fopen(kCachePath, "rb");
    if (!fp) return 0;

    char line[1200] = {};
    if (!std::fgets(line, sizeof(line), fp)) {
        std::fclose(fp);
        return 0;
    }
    line[std::strcspn(line, "\r\n")] = 0;
    if (std::strcmp(line, kCacheMagic) != 0) {
        std::fclose(fp);
        return 0;
    }

    size_t count = 0;
    while (count < kMaxEntries && std::fgets(line, sizeof(line), fp)) {
        line[std::strcspn(line, "\r\n")] = 0;
        char *tab1 = std::strchr(line, '\t');
        if (!tab1) continue;
        *tab1++ = 0;
        char *tab2 = std::strchr(tab1, '\t');
        if (!tab2) continue;
        *tab2++ = 0;

        char *tail = nullptr;
        (void)std::strtoul(line, &tail, 10);
        if (!tail || *tail) continue;
        if (!cache_full_path_safe(tab1) || !cache_rel_path_safe(tab2)) continue;
        ++count;
    }
    std::fclose(fp);
    return count;
}

static size_t panic_filer_work_cap()
{
    const size_t hint = pan_cache_entry_count_hint();
    if (!hint) return kMaxEntries;  /* first boot/stale cache: preserve R5G path */
    size_t cap = hint + 8u;         /* bounded headroom for cache churn */
    if (cap < 32u) cap = 32u;
    if (cap > kMaxEntries) cap = kMaxEntries;
    return cap;
}

static const char *base_name(const char *path)
{
    const char *p = path ? std::strrchr(path, '/') : nullptr;
    return p ? p + 1 : (path ? path : "");
}

static void parent_path(const char *rel, char *out, size_t cap)
{
    if (!out || !cap) return;
    std::snprintf(out, cap, "%s", rel ? rel : "");
    char *slash = std::strrchr(out, '/');
    if (!slash) std::snprintf(out, cap, "/");
    else { *slash = 0; if (!out[0]) std::snprintf(out, cap, "/"); }
}

static int folder_compare(const void *aa, const void *bb)
{
    const auto *a = static_cast<const FolderEntry *>(aa);
    const auto *b = static_cast<const FolderEntry *>(bb);
    if (!std::strcmp(a->rel_dir, "..")) return std::strcmp(b->rel_dir, "..") ? -1 : 0;
    if (!std::strcmp(b->rel_dir, "..")) return 1;
    return strcasecmp(a->rel_dir, b->rel_dir);
}

static FolderEntry *alloc_folders(size_t cap = kMaxEntries)
{
    if (!cap || cap > kMaxEntries) return nullptr;
    auto *p = static_cast<FolderEntry *>(heap_caps_calloc(
        cap, sizeof(FolderEntry), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!p)
        p = static_cast<FolderEntry *>(std::calloc(cap, sizeof(FolderEntry)));
    return p;
}

static void copy_text_bounded(char *dst, size_t cap, const char *src)
{
    if (!dst || !cap) return;
    if (!src) src = "";
    size_t n = std::strlen(src);
    if (n >= cap) n = cap - 1;
    if (n) std::memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool path_is_root(const char *dir)
{
    return !dir || !dir[0] || !std::strcmp(dir, "/");
}

static bool rel_under_current(const char *rel_path, const char *current_dir,
                              const char **out_remaining)
{
    if (!rel_path || !out_remaining) return false;
    if (path_is_root(current_dir)) {
        *out_remaining = rel_path;
        return true;
    }

    const size_t dlen = std::strlen(current_dir);
    if (strncasecmp(rel_path, current_dir, dlen) != 0) return false;
    if (rel_path[dlen] != '/') return false;
    *out_remaining = rel_path + dlen + 1;
    return true;
}

static size_t build_child_folder_list(const PanEntry *entries, size_t count,
                                      const char *current_dir,
                                      FolderEntry *folders, size_t cap)
{
    if (!entries || !folders || !cap) return 0;
    size_t folder_count = 0;

    if (!path_is_root(current_dir)) {
        copy_text_bounded(folders[folder_count++].rel_dir,
                          sizeof(folders[0].rel_dir), "..");
    }

    for (size_t i=0; i<count && folder_count<cap; ++i) {
        const char *remaining = nullptr;
        if (!rel_under_current(entries[i].rel_path, current_dir, &remaining))
            continue;

        const char *slash = std::strchr(remaining, '/');
        if (!slash) continue;  /* PAN directly in current directory, not a child folder. */

        const size_t child_len = (size_t)(slash - remaining);
        if (!child_len || child_len >= sizeof(folders[0].rel_dir)) continue;

        bool seen = false;
        for (size_t j=0; j<folder_count; ++j) {
            if (std::strlen(folders[j].rel_dir) == child_len &&
                !strncasecmp(folders[j].rel_dir, remaining, child_len)) {
                seen = true;
                break;
            }
        }
        if (seen) continue;

        std::memcpy(folders[folder_count].rel_dir, remaining, child_len);
        folders[folder_count].rel_dir[child_len] = '\0';
        ++folder_count;
    }

    qsort(folders, folder_count, sizeof(folders[0]), folder_compare);
    return folder_count;
}

static bool entry_in_current_folder(const PanEntry &entry, const char *current_dir)
{
    char parent[384] = {};
    parent_path(entry.rel_path, parent, sizeof(parent));
    const char *want = path_is_root(current_dir) ? "/" : current_dir;
    return !strcasecmp(parent, want);
}

static size_t file_count_in_current_folder(const PanEntry *entries, size_t count,
                                           const char *current_dir)
{
    size_t n = 0;
    for (size_t i=0; i<count; ++i)
        if (entry_in_current_folder(entries[i], current_dir)) ++n;
    return n;
}

static const PanEntry *file_in_current_folder_at(const PanEntry *entries, size_t count,
                                                 const char *current_dir,
                                                 size_t ordinal)
{
    size_t n = 0;
    for (size_t i=0; i<count; ++i) {
        if (!entry_in_current_folder(entries[i], current_dir)) continue;
        if (n++ == ordinal) return &entries[i];
    }
    return nullptr;
}

static void parent_current_dir(char *dir, size_t cap)
{
    if (!dir || !cap || path_is_root(dir)) {
        if (dir && cap) copy_text_bounded(dir, cap, "/");
        return;
    }

    char *slash = std::strrchr(dir, '/');
    if (!slash) {
        copy_text_bounded(dir, cap, "/");
        return;
    }
    *slash = '\0';
    if (!dir[0]) copy_text_bounded(dir, cap, "/");
}

static bool enter_child_dir(char *dir, size_t cap, const char *child)
{
    if (!dir || !cap || !child || !child[0] || !std::strcmp(child, ".."))
        return false;

    const size_t dlen = path_is_root(dir) ? 0 : std::strlen(dir);
    const size_t clen = std::strlen(child);
    const size_t need = dlen + (dlen ? 1 : 0) + clen + 1;
    if (need > cap) return false;

    char next[384] = {};
    size_t pos = 0;
    if (dlen) {
        std::memcpy(next, dir, dlen);
        pos = dlen;
        next[pos++] = '/';
    }
    std::memcpy(next + pos, child, clen);
    next[pos + clen] = '\0';
    copy_text_bounded(dir, cap, next);
    return true;
}

static size_t utf8_seq_len(unsigned char c)
{
    if ((c & 0x80u) == 0u) return 1;
    if ((c & 0xE0u) == 0xC0u) return 2;
    if ((c & 0xF0u) == 0xE0u) return 3;
    if ((c & 0xF8u) == 0xF0u) return 4;
    return 1;
}

static void fit_utf8_label(const char *src, char *dst, size_t cap, size_t max_chars)
{
    if (!dst || !cap) return;
    if (!src) src = "";

    size_t si = 0, di = 0, chars = 0;
    const size_t src_len = std::strlen(src);

    while (si < src_len && chars < max_chars) {
        const size_t seq = utf8_seq_len((unsigned char)src[si]);
        if (si + seq > src_len) break;
        if (di + seq + 1 >= cap) break;

        bool valid = true;
        for (size_t j=1; j<seq; ++j) {
            if (((unsigned char)src[si+j] & 0xC0u) != 0x80u) {
                valid = false;
                break;
            }
        }
        const size_t copy_n = valid ? seq : 1;
        if (di + copy_n + 1 >= cap) break;
        std::memcpy(dst + di, src + si, copy_n);
        di += copy_n;
        si += copy_n;
        ++chars;
    }

    const bool truncated = si < src_len;
    if (truncated && di + 4 < cap) {
        dst[di++]='.';
        dst[di++]='.';
        dst[di++]='.';
    }
    dst[di]='\0';
}

static void wait_release()
{
    for (int i = 0; i < 160; ++i) {
        M5.update();
        if (!M5.Touch.getCount()) break;
        vTaskDelay(pdMS_TO_TICKS(8) ? pdMS_TO_TICKS(8) : 1);
    }
}

static bool get_click(int &x, int &y)
{
    M5.update();
    auto t = M5.Touch.getDetail();
    if (!t.wasClicked()) return false;
    x = t.x; y = t.y; return true;
}

static void draw_two_pane(const PanEntry *entries, size_t count,
                          const FolderEntry *folders, size_t folder_count,
                          const char *current_dir,
                          size_t folder_page, size_t file_page)
{
    auto &d = M5.Display;
    d.fillScreen(TFT_BLACK);

    /* Filer header: simple two-line typography, no logo box/captions. */
    d.setFont(nullptr);
    d.setTextDatum(textdatum_t::top_left);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.setTextSize(2);
    d.drawString("PANIC Player for M5Tab : layer8", 18, 10);

    const char *folder_name = path_is_root(current_dir) ? "/" : base_name(current_dir);
    char current_label[160] = {};
    fit_utf8_label(folder_name, current_label, sizeof(current_label), 22);

    /* Second line is only the current folder name. */
    d.setFont(&fonts::efontJA_16);
    d.setTextDatum(textdatum_t::top_left);
    d.setTextSize(2);
    d.setTextColor(0xFFE0, TFT_BLACK);
    d.drawString(current_label, 18, 42);

    /* Right-upper controls: larger, high-contrast ASCII labels. */
    const struct TopButton {
        int x;
        int w;
        const char *label;
    } top_buttons[] = {
        { kRefreshX,      kRefreshW,      "REFRESH" },
        { kTopVolMinusX,  kTopVolMinusW,  "VOL-"    },
        { kTopVolPlusX,   kTopVolPlusW,   "VOL+"    },
    };
    d.setFont(nullptr);
    d.setTextSize(3);
    for (const auto &bt : top_buttons) {
        d.drawRect(bt.x, kTopButtonY, bt.w, kTopButtonH, 0x8410);
        d.setTextDatum(textdatum_t::middle_center);
        d.setTextColor(TFT_WHITE, TFT_BLACK);
        d.drawString(bt.label, bt.x + bt.w/2, kTopButtonY + kTopButtonH/2);
    }

    /* Restore Japanese-capable font for folder/file rows. */
    d.setFont(&fonts::efontJA_16);

    d.drawRect(kFolderPaneX, kPaneTop,
               kFolderPaneW, kPaneRows * kPaneRowH, 0x8410);
    d.drawRect(kFilePaneX, kPaneTop,
               kFilePaneW, kPaneRows * kPaneRowH, 0x8410);

    const size_t folder_first = folder_page * kPaneRows;
    for (int row=0; row<kPaneRows; ++row) {
        const size_t idx = folder_first + (size_t)row;
        const int y = kPaneTop + row * kPaneRowH;
        const uint16_t bg = (row & 1) ? 0x1082 : 0x0841;

        /* Every row is a full-pane-width touch button. */
        d.fillRect(kFolderPaneX + 2, y + 2,
                   kFolderPaneW - 4, kPaneRowH - 4, bg);
        d.drawRect(kFolderPaneX + 2, y + 2,
                   kFolderPaneW - 4, kPaneRowH - 4, 0x4208);

        if (idx >= folder_count) continue;

        char label[160] = {};
        fit_utf8_label(folders[idx].rel_dir, label, sizeof(label), 17);
        d.setTextDatum(textdatum_t::middle_left);
        d.setTextSize(2);
        d.setTextColor(!std::strcmp(folders[idx].rel_dir, "..")
                       ? 0xFFE0 : TFT_WHITE, bg);
        d.drawString(label, kFolderPaneX + 18, y + kPaneRowH/2);
    }

    const size_t file_first = file_page * kPaneRows;
    for (int row=0; row<kPaneRows; ++row) {
        const size_t ord = file_first + (size_t)row;
        const int y = kPaneTop + row * kPaneRowH;
        const uint16_t bg = (row & 1) ? 0x1082 : 0x0841;

        d.fillRect(kFilePaneX + 2, y + 2,
                   kFilePaneW - 4, kPaneRowH - 4, bg);
        d.drawRect(kFilePaneX + 2, y + 2,
                   kFilePaneW - 4, kPaneRowH - 4, 0x4208);

        const PanEntry *e = file_in_current_folder_at(
            entries, count, current_dir, ord);
        if (!e) continue;

        char name[192] = {};
        fit_utf8_label(base_name(e->rel_path), name, sizeof(name), 16);
        d.setTextDatum(textdatum_t::middle_left);
        d.setTextSize(2);
        d.setTextColor(TFT_WHITE, bg);
        d.drawString(name, kFilePaneX + 18, y + kPaneRowH/2);

        char sz[48];
        if (e->size >= 1024u*1024u)
            std::snprintf(sz, sizeof(sz), "%lu.%02lu MB",
                          (unsigned long)(e->size/(1024u*1024u)),
                          (unsigned long)((e->size%(1024u*1024u))*100u/(1024u*1024u)));
        else
            std::snprintf(sz, sizeof(sz), "%lu KB",
                          (unsigned long)((e->size + 1023u)/1024u));

        d.setTextDatum(textdatum_t::middle_right);
        d.setTextSize(1);
        d.setTextColor(0xC618, bg);
        d.drawString(sz, kFilePaneX + kFilePaneW - 16, y + kPaneRowH/2);
    }

    /* Only pane paging remains at the bottom. Each pair exactly follows its pane. */
    const int folder_half = kFolderPaneW / 2;
    const int file_half = kFilePaneW / 2;

    d.setTextSize(2);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.setTextDatum(textdatum_t::middle_center);

    d.drawRect(kFolderPaneX, kPagerY, folder_half - 4, kPagerH, 0x8410);
    d.drawRect(kFolderPaneX + folder_half + 4, kPagerY,
               kFolderPaneW - folder_half - 4, kPagerH, 0x8410);
    d.drawString("<", kFolderPaneX + (folder_half - 4)/2, kPagerY + kPagerH/2);
    d.drawString(">", kFolderPaneX + folder_half + 4 +
                 (kFolderPaneW - folder_half - 4)/2,
                 kPagerY + kPagerH/2);

    d.drawRect(kFilePaneX, kPagerY, file_half - 4, kPagerH, 0x8410);
    d.drawRect(kFilePaneX + file_half + 4, kPagerY,
               kFilePaneW - file_half - 4, kPagerH, 0x8410);
    d.drawString("<", kFilePaneX + (file_half - 4)/2, kPagerY + kPagerH/2);
    d.drawString(">", kFilePaneX + file_half + 4 +
                 (kFilePaneW - file_half - 4)/2,
                 kPagerY + kPagerH/2);

    d.setTextDatum(textdatum_t::top_left);
}

static void draw_loading(const char *path, const char *mode)
{
    auto &d=M5.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextDatum(textdatum_t::middle_center);
    d.setTextColor(0xFBE0, TFT_BLACK); d.setTextSize(3); d.drawString("LOADING PAN DATA", d.width()/2, 260);
    d.setTextColor(TFT_WHITE, TFT_BLACK); d.setTextSize(2); d.drawString(mode ? mode : "PLEASE WAIT", d.width()/2, 330);
    d.setTextColor(0xBDF7, TFT_BLACK); d.setTextSize(2); d.drawString(base_name(path), d.width()/2, 405);
}

static bool ensure_player_file()
{
    /*
     * Intent: P68K.X is only 15 KiB; rewrite it every PANIC launch so a stale
     * same-sized file from an older experiment can never shadow the embedded
     * V1.38 payload.
     * Layer8 Aug/17/2026
     */
    FILE *fp = std::fopen(kPlayerHostPath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Cannot materialize PANIC.X at %s errno=%d", kPlayerHostPath, errno);
        return false;
    }
    const size_t n = std::fwrite(panic_x_bin, 1, panic_x_bin_len, fp);
    const int close_rc = std::fclose(fp);
    if (n != panic_x_bin_len || close_rc != 0) {
        ESP_LOGE(TAG, "PANIC.X write failed %u/%u", (unsigned)n, (unsigned)panic_x_bin_len);
        return false;
    }
    ESP_LOGI(TAG, "Embedded PANIC.X materialized: %s (%u bytes)", kPlayerHostPath, (unsigned)panic_x_bin_len);
    return true;
}

static bool stage_pan_file(const char *selected_pan_path)
{
    if (!selected_pan_path || !selected_pan_path[0]) return false;
    if (!std::strcmp(selected_pan_path, kPanStageHostPath)) return true;

    FILE *src = std::fopen(selected_pan_path, "rb");
    if (!src) {
        ESP_LOGE(TAG, "Cannot open selected PAN for staging: %s errno=%d", selected_pan_path, errno);
        return false;
    }
    FILE *dst = std::fopen(kPanStageHostPath, "wb");
    if (!dst) {
        ESP_LOGE(TAG, "Cannot create %s errno=%d", kPanStageHostPath, errno);
        std::fclose(src);
        return false;
    }

    /* AVFAST R1: stage in the same 64 KiB granularity used by the
     * Tab5 Media Player read-ahead design. This runs before playback, so use
     * a larger sequential transfer without changing guest-visible semantics. */
    size_t chunk = 64 * 1024;
    uint8_t *buf = static_cast<uint8_t *>(
        heap_caps_malloc(chunk, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buf) {
        chunk = 16 * 1024;
        buf = static_cast<uint8_t *>(std::malloc(chunk));
    }
    if (!buf) {
        chunk = 4096;
        buf = static_cast<uint8_t *>(std::malloc(chunk));
    }
    if (!buf) {
        ESP_LOGE(TAG, "PAN staging buffer allocation failed");
        if (buf) std::free(buf);
        std::fclose(src);
        std::fclose(dst);
        unlink(kPanStageHostPath);
        return false;
    }

    bool ok = true;
    size_t total = 0;
    for (;;) {
        const size_t n = std::fread(buf, 1, chunk, src);
        if (n) {
            if (std::fwrite(buf, 1, n, dst) != n) { ok = false; break; }
            total += n;
        }
        if (n < chunk) {
            if (std::ferror(src)) ok = false;
            break;
        }
        if ((total & 0xFFFFu) == 0u) vTaskDelay(1);
    }
    const int dst_err = std::fclose(dst);
    std::fclose(src);
    std::free(buf);
    if (!ok || dst_err != 0) {
        ESP_LOGE(TAG, "PAN staging failed after %u bytes", (unsigned)total);
        unlink(kPanStageHostPath);
        return false;
    }
    ESP_LOGI(TAG, "PAN staged for Human68k: %s -> %s (%u bytes)",
             selected_pan_path, kPanStageHostPath, (unsigned)total);
    ESP_LOGI(TAG,
             "PANIC_AVFAST staging chunk=%u marker=PANICPLAYER_R1_AVFAST_R1_20261007",
             (unsigned)chunk);
    return true;
}

static const PanEntry *adjacent_in_same_folder(const PanEntry *entries,
                                                size_t count,
                                                const char *current_full,
                                                int delta)
{
    if (!entries || !count || !current_full || !current_full[0] || !delta)
        return nullptr;

    const PanEntry *current = nullptr;
    char parent[384] = {};

    for (size_t i=0; i<count; ++i) {
        if (!strcasecmp(entries[i].full_path, current_full)) {
            current = &entries[i];
            parent_path(entries[i].rel_path, parent, sizeof(parent));
            break;
        }
    }
    if (!current) return nullptr;

    size_t sibling_count = 0;
    size_t current_ordinal = 0;
    bool ordinal_found = false;

    for (size_t i=0; i<count; ++i) {
        char p[384] = {};
        parent_path(entries[i].rel_path, p, sizeof(p));
        if (strcasecmp(p, parent)) continue;
        if (&entries[i] == current) {
            current_ordinal = sibling_count;
            ordinal_found = true;
        }
        ++sibling_count;
    }

    if (!ordinal_found || !sibling_count) return nullptr;
    if (sibling_count == 1) return current;

    size_t target = current_ordinal;
    if (delta < 0)
        target = current_ordinal ? current_ordinal - 1 : sibling_count - 1;
    else
        target = (current_ordinal + 1) % sibling_count;

    size_t ordinal = 0;
    for (size_t i=0; i<count; ++i) {
        char p[384] = {};
        parent_path(entries[i].rel_path, p, sizeof(p));
        if (strcasecmp(p, parent)) continue;
        if (ordinal++ == target) return &entries[i];
    }
    return nullptr;
}

static bool choose_random_pan_path(char *out_path, size_t out_size)
{
    if (!out_path || out_size == 0) return false;
    out_path[0] = 0;
    PanEntry *entries = alloc_entries();
    if (!entries) return false;
    const size_t count = load_or_rebuild_pan_cache(entries, kMaxEntries);
    if (!count) { std::free(entries); return false; }
    const size_t idx = (size_t)(esp_random() % (uint32_t)count);
    std::snprintf(out_path, out_size, "%s", entries[idx].full_path);
    ESP_LOGI(TAG, "Random PAN %u/%u: %s", (unsigned)(idx+1), (unsigned)count, out_path);
    std::free(entries);
    return true;
}

} // namespace

extern "C" void tab5_panic_cleanup_staging(void)
{
    /*
     * Intent: PANIC launch files are transport artifacts, never user media.
     * Clean both current 6.11 root-level names and legacy 6.10 PLAY.PAN names
     * whenever the native launcher owns the SD card.  Only exact known paths
     * are removed; user-supplied PAN files are never touched.
     * Layer8 Aug/17/2026
     */
    const char *paths[] = {
        kPlayerHostPath, kPanStageHostPath, kBatchHostPath, kPendingRestartPath,
        kLegacyRootPlayPath, kLegacyPlayPath, kLegacyPlayerPath
    };
    unsigned removed = 0;
    for (const char *path : paths) {
        if (unlink(path) == 0) ++removed;
    }
    if (removed) ESP_LOGI(TAG, "PANIC staging cleanup: removed %u temporary file(s)", removed);
}

extern "C" int tab5_panic_prepare_random_runtime(char *out_path, size_t out_size)
{
    if (!out_path || out_size == 0) return 0;
    out_path[0] = 0;
    if (!choose_random_pan_path(out_path, out_size)) return 0;
    if (!tab5_panic_prepare_runtime(out_path)) {
        out_path[0] = 0;
        return 0;
    }
    ESP_LOGI(TAG, "PANIC live runtime staged (no ESP restart): %s", out_path);
    return 1;
}

extern "C" int tab5_panic_schedule_random_restart(void)
{
    char path[512];
    if (!choose_random_pan_path(path, sizeof(path))) return 0;
    FILE *fp = std::fopen(kPendingRestartPath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Cannot create PANIC restart handoff %s errno=%d", kPendingRestartPath, errno);
        return 0;
    }
    const size_t len = std::strlen(path);
    const bool write_ok = std::fwrite(path, 1, len, fp) == len && std::fputc('\n', fp) != EOF;
    const int close_rc = std::fclose(fp);
    const bool ok = write_ok && close_rc == 0;
    if (!ok) {
        unlink(kPendingRestartPath);
        ESP_LOGE(TAG, "PANIC restart handoff write failed");
        return 0;
    }
    ESP_LOGI(TAG, "PANIC restart handoff queued: %s", path);
    return 1;
}

extern "C" int tab5_panic_take_pending(char *out_path, size_t out_size)
{
    if (!out_path || out_size == 0) return 0;
    out_path[0] = 0;
    FILE *fp = std::fopen(kPendingRestartPath, "rb");
    if (!fp) return 0;
    char line[512] = {};
    const char *got = std::fgets(line, sizeof(line), fp);
    std::fclose(fp);
    unlink(kPendingRestartPath);
    if (!got) return 0;
    line[std::strcspn(line, "\r\n")] = 0;
    if (!line[0] || !ends_with_pan(line)) {
        ESP_LOGW(TAG, "Discarded stale PANIC restart handoff: %s", line[0] ? line : "<empty>");
        return 0;
    }
    FILE *pending_probe=std::fopen(line,"rb");
    if (!pending_probe) {
        ESP_LOGW(TAG, "Discarded unopenable PANIC restart handoff: %s errno=%d", line, errno);
        return 0;
    }
    std::fclose(pending_probe);
    std::snprintf(out_path, out_size, "%s", line);
    ESP_LOGI(TAG, "PANIC restart handoff accepted: %s", out_path);
    return 1;
}

extern "C" int tab5_panic_select_random(char *out_path, size_t out_size)
{
    if (!choose_random_pan_path(out_path, out_size)) return 0;
    draw_loading(out_path, "RANDOM ONE-SHOT");
    wait_release();
    return 1;
}

extern "C" int tab5_panic_select_file(char *out_path, size_t out_size)
{
    if (!out_path || out_size == 0) return 0;
    out_path[0]=0;

    size_t work_cap = panic_filer_work_cap();
    const size_t need_bytes =
        work_cap * (sizeof(PanEntry) + sizeof(FolderEntry));
    const size_t psram_free =
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    const size_t psram_largest =
        heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    printf("PANICNEXT_R5H: filer alloc cap=%u need=%u psram_free=%u largest=%u\n",
           (unsigned)work_cap, (unsigned)need_bytes,
           (unsigned)psram_free, (unsigned)psram_largest);

    PanEntry *entries=alloc_entries(work_cap);
    FolderEntry *folders=alloc_folders(work_cap);
    if (!entries || !folders) {
        printf("PANICNEXT_R5H: filer alloc FAIL cap=%u entries=%d folders=%d psram_free=%u largest=%u\n",
               (unsigned)work_cap, entries ? 1 : 0, folders ? 1 : 0,
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        std::free(entries);
        std::free(folders);
        return 0;
    }

    size_t count=load_or_rebuild_pan_cache(entries,work_cap);
    printf("PANICNEXT_R5H: filer cache loaded count=%u cap=%u\n",
           (unsigned)count, (unsigned)work_cap);
    char current_dir[384] = "/";
    size_t folder_count=build_child_folder_list(
        entries,count,current_dir,folders,work_cap);
    size_t folder_page=0;
    size_t file_page=0;

    ESP_LOGI(TAG, "%s", kPanicNextMarker);
    ESP_LOGI(TAG,
             "PANIC_UI_FINAL active pans=%u current=/ marker=PANICPLAYER_R1_PLAY_UI_IO_UIR3_20261007",
             (unsigned)count);

    wait_release();
    for (;;) {
        folder_count=build_child_folder_list(
            entries,count,current_dir,folders,work_cap);

        const size_t folder_pages=folder_count
                                ? ((folder_count+kPaneRows-1)/kPaneRows) : 1;
        if (folder_page>=folder_pages) folder_page=folder_pages-1;

        const size_t file_count=file_count_in_current_folder(
            entries,count,current_dir);
        const size_t file_pages=file_count
                              ? ((file_count+kPaneRows-1)/kPaneRows) : 1;
        if (file_page>=file_pages) file_page=file_pages-1;

        ESP_LOGI(TAG,
                 "PANIC_UI current=%s direct_pan=%u child_rows=%u fpage=%u ppage=%u",
                 current_dir, (unsigned)file_count, (unsigned)folder_count,
                 (unsigned)folder_page, (unsigned)file_page);

        draw_two_pane(entries,count,folders,folder_count,
                      current_dir,folder_page,file_page);
        wait_release();

        for (;;) {
            int x=0,y=0;
            if (!get_click(x,y)) { vTaskDelay(1); continue; }

            /* Right-upper REFRESH / VOL- / VOL+ */
            if (y>=kTopButtonY && y<kTopButtonY+kTopButtonH) {
                if (x>=kRefreshX && x<kRefreshX+kRefreshW) {
                    draw_loading(kCachePath,"REFRESHING CACHE");
                    /* REFRESH may discover more files than the compact cache-sized
                     * workspace.  Expand to the legacy maximum only on explicit
                     * user request; if the large allocation is unavailable during
                     * playback, preserve the current cache instead of dropping out. */
                    if (work_cap < kMaxEntries) {
                        PanEntry *expanded_entries=alloc_entries(kMaxEntries);
                        FolderEntry *expanded_folders=alloc_folders(kMaxEntries);
                        if (expanded_entries && expanded_folders) {
                            std::free(entries);
                            std::free(folders);
                            entries=expanded_entries;
                            folders=expanded_folders;
                            work_cap=kMaxEntries;
                            printf("PANICNEXT_R5H: REFRESH workspace expanded cap=%u\n",
                                   (unsigned)work_cap);
                        } else {
                            std::free(expanded_entries);
                            std::free(expanded_folders);
                            printf("PANICNEXT_R5H: REFRESH large workspace unavailable; existing cache retained\n");
                            break;
                        }
                    }
                    std::memset(entries,0,work_cap*sizeof(PanEntry));
                    std::memset(folders,0,work_cap*sizeof(FolderEntry));
                    count=rebuild_pan_cache(entries,work_cap);
                    copy_text_bounded(current_dir,sizeof(current_dir),"/");
                    folder_page=0;
                    file_page=0;
                    break;
                }
                if (x>=kTopVolMinusX && x<kTopVolMinusX+kTopVolMinusW) {
                    (void)tab5_audio_step_volume(-1);
                    break;
                }
                if (x>=kTopVolPlusX && x<kTopVolPlusX+kTopVolPlusW) {
                    (void)tab5_audio_step_volume(+1);
                    break;
                }
            }

            /* Pane rows */
            if (y>=kPaneTop && y<kPaneBottom) {
                const size_t row=(size_t)((y-kPaneTop)/kPaneRowH);

                if (x>=kFolderPaneX && x<kFolderPaneX+kFolderPaneW) {
                    const size_t idx=folder_page*kPaneRows+row;
                    if (idx<folder_count) {
                        const char *choice=folders[idx].rel_dir;
                        if (!std::strcmp(choice,"..")) {
                            parent_current_dir(current_dir,sizeof(current_dir));
                            ESP_LOGI(TAG,"PANIC_UI folder up -> %s",current_dir);
                        } else if (enter_child_dir(current_dir,sizeof(current_dir),choice)) {
                            ESP_LOGI(TAG,"PANIC_UI folder enter -> %s",current_dir);
                        }
                        folder_page=0;
                        file_page=0;
                        break;
                    }
                }

                if (x>=kFilePaneX && x<kFilePaneX+kFilePaneW) {
                    const size_t ord=file_page*kPaneRows+row;
                    const PanEntry *e=file_in_current_folder_at(
                        entries,count,current_dir,ord);
                    if (e) {
                        FILE *probe=std::fopen(e->full_path,"rb");
                        if (!probe) {
                            ESP_LOGW(TAG,
                                     "Cached PAN missing/unopenable: %s; use REFRESH",
                                     e->full_path);
                            break;
                        }
                        std::fclose(probe);
                        std::snprintf(out_path,out_size,"%s",e->full_path);
                        draw_loading(out_path,"STAGING PAN");
                        std::free(folders);
                        std::free(entries);
                        wait_release();
                        return 1;
                    }
                }
            }

            /* Pager buttons follow the exact pane widths. */
            if (y>=kPagerY && y<kPagerY+kPagerH) {
                const int folder_half=kFolderPaneW/2;
                const int file_half=kFilePaneW/2;

                if (x>=kFolderPaneX && x<kFolderPaneX+folder_half) {
                    if (folder_page>0) --folder_page;
                    break;
                }
                if (x>=kFolderPaneX+folder_half &&
                    x<kFolderPaneX+kFolderPaneW) {
                    if (folder_page+1<folder_pages) ++folder_page;
                    break;
                }
                if (x>=kFilePaneX && x<kFilePaneX+file_half) {
                    if (file_page>0) --file_page;
                    break;
                }
                if (x>=kFilePaneX+file_half &&
                    x<kFilePaneX+kFilePaneW) {
                    if (file_page+1<file_pages) ++file_page;
                    break;
                }
            }
        }
    }
}

extern "C" int tab5_panic_adjacent_file(const char *current_path, int delta,
                                         char *out_path, size_t out_size)
{
    if (!current_path || !current_path[0] || !out_path || !out_size ||
        (delta != -1 && delta != 1)) return 0;
    out_path[0] = 0;

    PanEntry *entries = alloc_entries();
    if (!entries) return 0;

    size_t count = load_or_rebuild_pan_cache(entries, kMaxEntries);
    const PanEntry *target = adjacent_in_same_folder(
        entries, count, current_path, delta);

    if (!target) {
        /* A user may have changed the SD card since the cache was written. */
        std::memset(entries, 0, kMaxEntries * sizeof(PanEntry));
        count = rebuild_pan_cache(entries, kMaxEntries);
        target = adjacent_in_same_folder(entries, count, current_path, delta);
    }

    if (!target) {
        ESP_LOGW(TAG, "PLAY %s: current PAN not found in same-folder cache: %s",
                 delta < 0 ? "PREV" : "NEXT", current_path);
        std::free(entries);
        return 0;
    }

    std::snprintf(out_path, out_size, "%s", target->full_path);
    ESP_LOGI(TAG, "PLAY %s same-folder: %s -> %s",
             delta < 0 ? "PREV" : "NEXT", current_path, out_path);
    std::free(entries);
    return 1;
}

extern "C" int tab5_panic_repeat_enabled(void)
{
    return __atomic_load_n(&s_playback_repeat_enabled, __ATOMIC_ACQUIRE) ? 1 : 0;
}

extern "C" int tab5_panic_prepare_runtime(const char *selected_pan_path)
{
    if (!selected_pan_path || !selected_pan_path[0]) return 0;
    /* Build 6.12c: old 6.10 transport names are never runtime inputs now.
     * Remove them again immediately before staging, not only at launcher boot,
     * so PLAY.PAN cannot survive a mixed-version/test session. */
    const char *legacy[] = { kLegacyRootPlayPath, kLegacyPlayPath, kLegacyPlayerPath };
    for (const char *path : legacy) {
        if (unlink(path) == 0) ESP_LOGI(TAG, "Removed legacy PANIC staging file: %s", path);
        else if (errno != ENOENT) ESP_LOGW(TAG, "Could not remove legacy PANIC staging file: %s errno=%d", path, errno);
    }
    if (!ends_with_pan(selected_pan_path)) {
        ESP_LOGE(TAG, "Selected path is not a PAN file: %s", selected_pan_path);
        return 0;
    }
    FILE *selected_probe=std::fopen(selected_pan_path,"rb");
    if (!selected_probe) {
        ESP_LOGE(TAG, "Selected PAN disappeared/unopenable: %s errno=%d",
                 selected_pan_path, errno);
        return 0;
    }
    std::fclose(selected_probe);
    if (!ensure_player_file()) return 0;
    /*
     * Intent: Stage the selected file under an 8.3-safe name so every
     * Human68k DOS operation (FILES/CHMOD/OPEN/SEEK), not just OPEN, sees
     * ordinary HostFS semantics. The original PAN file is never modified.
     * Layer8 Aug/17/2026
     */
    if (!stage_pan_file(selected_pan_path)) return 0;
    ESP_LOGI(TAG, "PANIC runtime ready: P68K.X + P68K.PAN from %s", selected_pan_path);
    return 1;
}

extern "C" int tab5_panic_prepare_batch(char drive_letter)
{
    if (drive_letter < 'A' || drive_letter > 'Z') return 0;
    FILE *fp = std::fopen(kBatchHostPath, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Cannot create %s errno=%d", kBatchHostPath, errno);
        return 0;
    }

    /*
     * Build 6.12c: restore the hardware-proven 6.11b bootstrap.  COMMAND.X
     * gets a tiny 8.3 batch first; if that does not launch PANIC, main.c
     * sends one direct P68K.X command 10 seconds later.  The user's 6.11b
     * hardware log showed that exact two-stage sequence starting PANIC.
     */
    /* R5G: always launch the exact staged PAN normally.  Repeat is a host-side
     * one-SPACE-per-second pulse because affected PAN data waits inside the
     * player instead of returning for wildcard enumeration. */
    char line[96];
    const int len = std::snprintf(line, sizeof(line),
                                  "%c:\\P68K.X %c:\\P68K.PAN\r\n",
                                  drive_letter, drive_letter);
    const size_t n = (len > 0) ? std::fwrite(line, 1, (size_t)len, fp) : 0u;
    const int close_rc = std::fclose(fp);
    if (len <= 0 || n != (size_t)len || close_rc != 0) {
        ESP_LOGE(TAG, "P68K.BAT write failed %u/%d", (unsigned)n, len);
        unlink(kBatchHostPath);
        return 0;
    }
    ESP_LOGI(TAG, "PANIC batch ready: %s -> %c:\\P68K.X %c:\\P68K.PAN",
             kBatchHostPath, drive_letter, drive_letter);
    return 1;
}

extern "C" void tab5_panic_reset_touch(void)
{
    s_touch_active = false;
    s_touch_started_in_corner = false;
    s_touch_started_volm = false;
    s_touch_started_volp = false;
    s_touch_started_prev = false;
    s_touch_started_next = false;
    s_touch_started_repeat = false;
    s_touch_started_turbo = false;
    s_touch_started_logo = false;
    s_touch_action_fired = false;
    s_touch_started_side_chrome = false;
    s_touch_start_us = 0;
    s_playback_controls_last_draw_us = 0;
}

extern "C" int tab5_panic_playback_controls_dirty(void)
{
    return s_playback_controls_last_draw_us == 0 ? 1 : 0;
}

extern "C" void tab5_panic_present_playback_controls(void)
{
    draw_playback_controls_if_due();
}

extern "C" void tab5_panic_run_rpg_interlude(void)
{
    auto &d = M5.Display;
    constexpr int tile = 32;
    constexpr int map_h = 18; /* 576 px; bottom 144 px is RPG status/dialogue. */
    constexpr uint16_t grass0 = 0x3DA5;
    constexpr uint16_t grass1 = 0x35A4;
    constexpr uint16_t path0  = 0xC5C6;
    constexpr uint16_t water0 = 0x251F;
    constexpr uint16_t tree0  = 0x1B43;
    constexpr uint16_t trunk0 = 0x8A22;
    constexpr uint16_t roof0  = 0xA982;
    constexpr uint16_t wall0  = 0xDE8B;

    int hero_x = 9, hero_y = 9;
    int hero_dx = 1, hero_dy = 0;
    uint32_t step = 0;
    bool exit_armed = false;
    bool exit_down = false;
    const int64_t start_us = esp_timer_get_time();

    auto blocked = [&](int tx, int ty) -> bool {
        if (tx < 1 || tx >= 39 || ty < 1 || ty >= map_h - 1) return true;
        if (tx >= 30 && ty >= 2 && ty <= 13) return true; /* water */
        if (tx >= 4 && tx <= 8 && ty >= 3 && ty <= 7) return true; /* house */
        return false;
    };

    auto draw_scene = [&]() {
        d.startWrite();
        d.setFont(nullptr);
        d.setTextDatum(textdatum_t::middle_center);

        for (int ty = 0; ty < map_h; ++ty) {
            for (int tx = 0; tx < 40; ++tx) {
                uint16_t c = ((tx ^ ty) & 1) ? grass0 : grass1;
                if (tx >= 30 && ty >= 2 && ty <= 13) c = water0;
                if ((ty == 9 || ty == 10) && tx > 8 && tx < 30) c = path0;
                d.fillRect(tx * tile, ty * tile, tile, tile, c);

                if ((tx == 1 || tx == 38 || ty == 1 || ty == 16) &&
                    ((tx + ty) % 2 == 0)) {
                    d.fillRect(tx*tile+8, ty*tile+7, 16, 20, tree0);
                    d.fillRect(tx*tile+14, ty*tile+22, 5, 9, trunk0);
                }

                if (tx >= 4 && tx <= 8 && ty >= 3 && ty <= 7) {
                    if (ty == 3) d.fillRect(tx*tile, ty*tile+12, tile, 20, roof0);
                    else d.fillRect(tx*tile, ty*tile, tile, tile, wall0);
                }

                if (c == water0 && ((tx + ty + (int)step) & 3) == 0)
                    d.drawFastHLine(tx*tile+5, ty*tile+16, 22, TFT_CYAN);
            }
        }

        /* Door and window details. */
        d.fillRect(6*tile+10, 6*tile+8, 12, 24, 0x61C3);
        d.fillRect(4*tile+8, 5*tile+8, 12, 10, 0x7E7F);
        d.fillRect(8*tile+12, 5*tile+8, 12, 10, 0x7E7F);

        /* A pointless slime. */
        const int sx = 23*tile + ((step & 1u) ? 3 : 0);
        const int sy = 6*tile + ((step & 2u) ? 2 : 0);
        d.fillRect(sx+7, sy+15, 20, 12, 0x07E0);
        d.fillRect(sx+11, sy+10, 12, 8, 0x07E0);
        d.fillRect(sx+12, sy+17, 3, 3, TFT_BLACK);
        d.fillRect(sx+21, sy+17, 3, 3, TFT_BLACK);

        /* 8-bit-ish hero, deliberately overbuilt for an easter egg. */
        const int px = hero_x * tile;
        const int py = hero_y * tile;
        const bool alt = (step & 1u) != 0;
        d.fillRect(px+10, py+5, 12, 7, 0xFC10);  /* hair */
        d.fillRect(px+11, py+12, 10, 8, 0xFDC9); /* face */
        d.fillRect(px+8, py+20, 16, 8, 0x001F);  /* tunic */
        d.fillRect(px+8, py+28, 6, 4, alt ? 0xFFFF : 0x7800);
        d.fillRect(px+18, py+28, 6, 4, alt ? 0x7800 : 0xFFFF);

        /* Bottom RPG UI. */
        d.fillRect(0, 576, 1280, 144, TFT_BLACK);
        d.drawRect(8, 584, 1264, 128, TFT_WHITE);
        d.drawRect(12, 588, 1256, 120, TFT_WHITE);
        d.setTextColor(TFT_WHITE, TFT_BLACK);
        d.setTextSize(3);
        d.drawString("PANIC QUEST", 200, 616);
        d.setTextSize(2);
        d.drawString("HERO  Lv.??   HP 99/99   GOLD  0", 760, 614);
        const uint32_t phase = (uint32_t)((esp_timer_get_time() - start_us) / 2500000LL) % 3u;
        const char *msg = phase == 0 ? "WHERE DID THE PLAYER GO ?" :
                          phase == 1 ? "THE HERO WALKS FOR NO REASON." :
                                       "THIS FEATURE HAS NO PRACTICAL VALUE.";
        d.drawString(msg, 520, 662);
        d.setTextSize(1);
        d.drawString(exit_armed ? "TOUCH ANYWHERE TO RETURN" : "...", 1070, 695);
        d.endWrite();
    };

    ESP_LOGI(TAG, "PANIC_QUEST_R1K ENTER: guest execution paused; pointless RPG active");

    for (;;) {
        M5.update();

        bool pressed = false;
        const size_t n = M5.Touch.getCount();
        for (size_t i = 0; i < n; ++i) {
            if (M5.Touch.getDetail(i).isPressed()) {
                pressed = true;
                break;
            }
        }

        /* Do not let the logo press that entered the easter egg immediately
         * dismiss it. Arm only after that finger has been released. */
        if (!exit_armed) {
            if (!pressed && esp_timer_get_time() - start_us > 350000LL)
                exit_armed = true;
        } else {
            if (pressed) exit_down = true;
            if (exit_down && !pressed) break;
        }

        if ((step % 2u) == 0u) {
            const uint32_t r = esp_random();
            switch (r & 3u) {
                case 0: hero_dx=1; hero_dy=0; break;
                case 1: hero_dx=-1; hero_dy=0; break;
                case 2: hero_dx=0; hero_dy=1; break;
                default: hero_dx=0; hero_dy=-1; break;
            }
            const int nx = hero_x + hero_dx;
            const int ny = hero_y + hero_dy;
            if (!blocked(nx, ny)) {
                hero_x = nx;
                hero_y = ny;
            }
        }

        draw_scene();
        ++step;
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    d.fillScreen(TFT_BLACK);
    ESP_LOGI(TAG, "PANIC_QUEST_R1K EXIT: returning to paused PANIC playback");
}

extern "C" int tab5_panic_process_playback_touch_sample(int x, int y, int pressed)
{
    /* PanicPlayer NEXT: touch is sampled by the Production CPU0 presenter.
     * This function owns only the UI state machine; it never calls M5.update()
     * and therefore never steals I2C/touch ownership from Production video. */
    if (pressed) {
        if (!s_touch_active) {
            s_touch_active=true;
            s_touch_started_side_chrome=(x<160 || x>=1120);
            s_touch_started_logo=point_in_play_rect(
                x,y,kPlaybackLeftX,kPlaybackLogoY,
                kPlaybackButtonW,kPlaybackLogoH);
            s_touch_started_in_corner=(x>=1120 && y<110);
            s_touch_started_prev=(x<160 && y>=545);
            s_touch_started_next=(x>=1120 && y>=545);

            s_touch_started_volp=point_in_play_rect(
                x,y,kPlaybackRightX,kPlaybackVolPlusY,
                kPlaybackButtonW,kPlaybackSmallH);
            s_touch_started_volm=point_in_play_rect(
                x,y,kPlaybackRightX,kPlaybackVolMinusY,
                kPlaybackButtonW,kPlaybackSmallH);
            s_touch_started_repeat=point_in_play_rect(
                x,y,kPlaybackRightX,kPlaybackRepeatY,
                kPlaybackButtonW,kPlaybackSmallH);
            s_touch_started_turbo=point_in_play_rect(
                x,y,kPlaybackLeftX,kPlaybackTurboY,
                kPlaybackButtonW,kPlaybackSmallH);
            s_touch_start_us=esp_timer_get_time();

            if (s_touch_started_logo) {
                s_touch_action_fired=true;
                ESP_LOGI(TAG,"PLAY PANIC PLAYER logo -> PANIC QUEST");
                return 5;
            }
            if (s_touch_started_in_corner) {
                s_touch_action_fired=true;
                ESP_LOGI(TAG,"PLAY BACK requested (Production CPU0 press-edge)");
                return 2;
            }
            if (s_touch_started_prev) {
                s_touch_action_fired=true;
                ESP_LOGI(TAG,"PLAY PREV requested (Production CPU0 press-edge)");
                return 3;
            }
            if (s_touch_started_next) {
                s_touch_action_fired=true;
                ESP_LOGI(TAG,"PLAY NEXT requested (Production CPU0 press-edge)");
                return 4;
            }
        }
        return 0;
    }

    if (!s_touch_active) return 0;

    const bool fired=s_touch_action_fired;
    const bool volm=s_touch_started_volm;
    const bool volp=s_touch_started_volp;
    const bool repeat=s_touch_started_repeat;
    const bool turbo=s_touch_started_turbo;
    const bool side_chrome=s_touch_started_side_chrome;
    tab5_panic_reset_touch();

    if (fired) return 0;

    if (volm) {
        const int v=tab5_audio_step_volume(-1);
        ESP_LOGI(TAG,"PLAY VOL- -> %d/255",v);
        return 0;
    }
    if (volp) {
        const int v=tab5_audio_step_volume(+1);
        ESP_LOGI(TAG,"PLAY VOL+ -> %d/255",v);
        return 0;
    }
    if (repeat) {
        const bool old_repeat =
            __atomic_load_n(&s_playback_repeat_enabled, __ATOMIC_ACQUIRE);
        const bool new_repeat = !old_repeat;
        __atomic_store_n(&s_playback_repeat_enabled, new_repeat, __ATOMIC_RELEASE);
        s_playback_controls_last_draw_us=0;
        ESP_LOGI(TAG,"PLAY REPEAT -> %s (host SPACE pulse every 1000 ms)",
                 new_repeat ? "ON" : "OFF");
        /* CPU1 observes this latch and sends one short SPACE pulse per second. */
        return 6;
    }
    if (turbo) {
        s_playback_controls_last_draw_us=0;
        return 7;
    }

    /* A center-screen tap remains PANIC.X SPACE. */
    return side_chrome ? 0 : 1;
}

extern "C" int tab5_panic_poll_playback_touch(void)
{
    /* Legacy ABI retained but deliberately inactive in NEXT. Production CPU0
     * owns touch sampling through tab5_panic_process_playback_touch_sample(). */
    return 0;
}
