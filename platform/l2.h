// wrappers on chorus.h

#ifndef L2_H
#define L2_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define L2_SCREEN_W 160
#define L2_SCREEN_H 160
#define L2_PITCH 240

// buttons (from chorus.h)
#define L2_RIGHT 0x00000080u
#define L2_DOWN 0x00000100u
#define L2_LEFT 0x00000200u
#define L2_B 0x00002000u
#define L2_A 0x00004000u
#define L2_PAUSE 0x04000000u
#define L2_HINT 0x10000000u
#define L2_HOME 0x20000000u
#define L2_UP 0x80000000u

void l2_main(void); // the program's entry, called on our own stack

uint32_t l2_ticks(void);
uint32_t l2_us(void);
void l2_sleep(uint32_t ms);

enum { L2P_CLEAR, L2P_WALLS, L2P_SPRITES, L2P_DOUBLE, L2P_GAME, L2P_AUDIO, L2P_CONVERT, L2P_LCD, L2P_YIELD, L2P_BLIT, L2P_COUNT }; // debug stuff that i'll keep in here justin case
const uint8_t *l2_rows_shown(void);
// For each of the LCD's 160 lines and columns, the screen's line and column it
// shows (2D screens: cropped at the sides, see L2_CROP in sdl_l2.c)
const uint16_t *l2_row_src(void);
const uint16_t *l2_col_src(void);
void l2_set_crop(int menus);  // nonzero: the menus' crop (L2_MENU_CROP), else none
void l2_show_view(const uint8_t *view);
void l2_prof(int stage, uint32_t us);
uint32_t l2_buttons(void);
void l2_present(const uint8_t *fb);
void l2_present_lines(const uint8_t *fb, int first, int count);
void l2_reset_idle(void);
int l2_power_button(void);
__attribute__((noreturn)) void l2_shut_down(void);
// fix wolf3d or spear of destiny photobombing your next boot, we define this here and have it deleted
#define L2_RELAUNCH_LINK "B:\\Leapster\\PlayGame.lnk"
void l2_forget_relaunch(void);

// power stuff
int l2_power_pressed(void);
int l2_power_claimed(void);
void l2_power_claim(void);
__attribute__((noreturn)) void l2_power_halt(void);
__attribute__((noreturn)) void l2_quit(void); 
void l2_heap_report(const char *when);

void *l2_malloc(size_t n);
void l2_free(void *p);

// sound
#define L2_VOICE_RATE 8000
void l2_voice_play(int voice, const uint8_t *alaw, uint32_t len, uint16_t volume);
void l2_voice_stop(int voice);
void l2_dcache_flush(void);  // (before the chip reads what the CPU wrote)

// audiompi stuff
typedef int (*L2Feed)(int16_t *buf);
#define L2_FEED_SAMPLES 128
#define L2_FEED_RATE 11025  // (measured with hwtest/sndtest.c: ~86 calls a second)
void l2_feed_start(L2Feed feed, uint8_t volume);
void l2_feed_stop(void);
void l2_feed_volume(uint8_t volume);
void l2_codec_enable(void);
uint32_t l2_alaw_length(uint32_t src_len, uint32_t src_rate);
void l2_pcm8_to_alaw(const uint8_t *src, uint32_t src_len, uint32_t src_rate, uint8_t *dst, uint32_t dst_len);
void l2_pcm16_to_alaw(const int16_t *src, uint32_t n, uint8_t *dst);

// file management
void *l2_fopen(const char *path, const char *mode);
int l2_fclose(void *f);
size_t l2_fread(void *buf, size_t size, size_t n, void *f);
size_t l2_fwrite(const void *buf, size_t size, size_t n, void *f);
int l2_fseek(void *f, int32_t off, int whence);
uint32_t l2_filelength(const char *path);
int l2_mkdir(const char *path);
int l2_fdelete(const char *path);
int l2_chdir(const char *path);
void l2_io_init(const char *dir);

static inline void l2_pset(uint8_t *fb, int x, int y, uint16_t c) {
    uint8_t *p = fb + y * L2_PITCH + (x >> 1) * 3;
    if (x & 1) {
        p[1] = (p[1] & 0xF0) | (c >> 8);
        p[2] = c & 0xFF;
    } else {
        p[0] = c & 0xFF;
        p[1] = (p[1] & 0x0F) | ((c >> 4) & 0xF0);
    }
}

// text console in the code page 437 font
void l2_con_init(uint8_t *fb);
void l2_con_at(int col, int row);
void l2_con_color(uint16_t fg, uint16_t bg);
void l2_con_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif

// tbf you'll get a lot more learned from this looking at toadster172's "chorus.h"