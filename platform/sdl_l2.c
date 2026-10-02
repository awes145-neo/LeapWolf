// SDL1.2 for the leapster2 (include/SDL.h)

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "l2.h"

int main(int argc, char **argv);

void l2_main(void) {
    l2_io_init("B:\\Wolf3D");
    static char *argv[] = {"wolf3d", NULL};
    exit(main(1, argv));
}

// basic shit
int SDL_Init(Uint32 flags) { return 0; }
void SDL_Quit(void) {}
const char *SDL_GetError(void) { return "not supported"; }
Uint32 SDL_GetTicks(void) { return l2_ticks(); }

// BIOS tasks run when blocking, however the only one we know is frame sending, so we wait a little bit and poll if nothing's been done for a bit
static uint8_t *lcd;  // the 12-bit frame
static uint32_t last_yield;

void l2_prof(int stage, uint32_t us);
static void update_channels(void);
static void check_power(void);

static void yield(void) {
    check_power();
    update_channels();  // wait for a sound to end
    uint32_t t = l2_us();
    if (lcd)
        l2_present(lcd);
    last_yield = l2_ticks();
    l2_prof(L2P_YIELD, l2_us() - t);
}

void SDL_Delay(Uint32 ms) {
    Uint32 end = l2_ticks() + ms;
    do
        yield();
    while ((int32_t)(l2_ticks() - end) < 0);
}

// video

#define OUT_W 160
#define OUT_H 160
#define OUT_Y ((L2_SCREEN_H - OUT_H) / 2)

static SDL_Surface *screen;
static uint16_t pal12[256];
// cropping so the game looks a little better, menus at least, we ain't gonna get 100% perfect but we can get close
#ifndef L2_MENU_CROP
#define L2_MENU_CROP 32
#endif
static uint16_t row_src[OUT_H];  // source line for each output line
static uint16_t col_src[OUT_W];  // and source column for each output column
static const uint8_t *pending_view;  // (l2_show_view)
static const uint8_t *last_view;     // the last frame's, if it was the view
static uint8_t rows_shown[1024];

const uint8_t *l2_rows_shown(void) { return rows_shown; }
const uint16_t *l2_row_src(void) { return row_src; }
const uint16_t *l2_col_src(void) { return col_src; }

void l2_set_crop(int menus) {
    int crop = menus ? L2_MENU_CROP : 0, w = screen ? screen->w : 320, span = w - 2 * crop;
    for (int x = 0; x < OUT_W; x++)
        col_src[x] = (uint16_t)(crop + (x * span + span / 2) / OUT_W);
}
void l2_show_view(const uint8_t *view) { pending_view = view; }

static SDL_Surface *shown;  // what SDL_Flip shows
static void update_channels(void);
static int audio_open;
static void materialize(void);

static SDL_PixelFormat format8 = {NULL, 8, 1};
static SDL_VideoInfo video_info = {&format8};

const SDL_VideoInfo *SDL_GetVideoInfo(void) { return &video_info; }

static SDL_Surface *new_surface(Uint32 flags, int w, int h) {
    SDL_Surface *s = calloc(1, sizeof *s);
    if (!s)
        return NULL;
    s->pixels = calloc(1, w * h);
    if (!s->pixels) {
        free(s);
        return NULL;
    }
    s->flags = flags;
    s->w = w;
    s->h = h;
    s->pitch = w;
    s->pal.ncolors = 256;
    s->pal.colors = s->colors;
    s->fmt.palette = &s->pal;
    s->fmt.BitsPerPixel = 8;
    s->fmt.BytesPerPixel = 1;
    s->format = &s->fmt;
    return s;
}

SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags) {
    if (bpp != 8 || screen)
        return screen;
    screen = new_surface(flags & ~SDL_DOUBLEBUF, w, h);
    lcd = calloc(1, L2_SCREEN_H * L2_PITCH);
    for (int y = 0; y < OUT_H; y++) {
        row_src[y] = (uint16_t)((y * h + h / 2) / OUT_H);
        if (row_src[y] < sizeof rows_shown)
            rows_shown[row_src[y]] = 1;
    }
    l2_set_crop(0);
    return screen;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth, Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am) {
    return depth == 8 ? new_surface(flags, w, h) : NULL;
}

void SDL_FreeSurface(SDL_Surface *s) {
    if (s && s == shown)
        materialize();
    if (s && s != screen) {
        free(s->pixels);
        free(s);
    }
}

static void materialize(void) {
    pending_view = NULL;  // something other than the view frame is drawn
    if (shown && shown != screen) {
        uint32_t t = l2_us();
        memcpy(screen->pixels, shown->pixels, screen->w * screen->h);
        l2_prof(L2P_BLIT, l2_us() - t);
    }
    shown = screen;
}

int SDL_LockSurface(SDL_Surface *s) {
    if (s == screen)
        materialize();
    return 0;
}
void SDL_UnlockSurface(SDL_Surface *s) {}

// for each ab pair, the lcd wants 3 bytes, being a GB, a R | b R, b GB.
// lut_a and lut_b hold each part of which as a 24 bit word
static uint32_t lut_a[256], lut_b[256];

static void update_pal12(void) {
    for (int i = 0; i < 256; i++) {
        SDL_Color c = screen->colors[i];
        uint16_t p = (uint16_t)(((c.r >> 4) << 8) | (c.g & 0xF0) | (c.b >> 4));
        pal12[i] = p;
        lut_a[i] = (p & 0xFFu) | (((p >> 4) & 0xF0u) << 8);
        lut_b[i] = ((uint32_t)(p >> 8) << 8) | ((p & 0xFFu) << 16);
    }
}

int SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int first, int n) {
    if (!s)
        return 0;
    if (first < 0 || first + n > 256)
        return 0;
    memcpy(s->colors + first, colors, n * sizeof *colors);
    // a physical palette change shows at once, as on VGA
    if (s == screen) {
        update_pal12();
        if (flags & SDL_PHYSPAL) {
            pending_view = last_view;  // the same frame, in the new colors
            SDL_Flip(screen);
        }
    }
    return 1;
}

int SDL_SetColors(SDL_Surface *s, SDL_Color *colors, int first, int n) {
    return SDL_SetPalette(s, SDL_LOGPAL | SDL_PHYSPAL, colors, first, n);
}

int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
    if (dst == screen && src != screen && !srcrect && (!dstrect || (dstrect->x == 0 && dstrect->y == 0)) &&
        src->w == screen->w && src->h == screen->h) {
        // the whole frame, every frame: only the lines the LCD shows
        uint32_t t = l2_us();
        for (int y = 0; y < src->h; y++)
            if (rows_shown[y])
                memcpy((uint8_t *)screen->pixels + y * screen->pitch, (const uint8_t *)src->pixels + y * src->pitch,
                       src->w);
        l2_prof(L2P_BLIT, l2_us() - t);
        shown = screen;
        return 0;
    }
    if (src == screen || dst == screen)
        materialize();
    uint32_t t = l2_us();
    int sx = 0, sy = 0, w = src->w, h = src->h, dx = 0, dy = 0;
    if (srcrect) {
        sx = srcrect->x;
        sy = srcrect->y;
        w = srcrect->w;
        h = srcrect->h;
    }
    if (dstrect) {
        dx = dstrect->x;
        dy = dstrect->y;
    }
    // clip to both surfaces
    if (sx < 0) { w += sx; dx -= sx; sx = 0; }
    if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (dx < 0) { w += dx; sx -= dx; dx = 0; }
    if (dy < 0) { h += dy; sy -= dy; dy = 0; }
    if (sx + w > src->w) w = src->w - sx;
    if (sy + h > src->h) h = src->h - sy;
    if (dx + w > dst->w) w = dst->w - dx;
    if (dy + h > dst->h) h = dst->h - dy;
    if (w <= 0 || h <= 0)
        return 0;
    const uint8_t *s = (const uint8_t *)src->pixels + sy * src->pitch + sx;
    uint8_t *d = (uint8_t *)dst->pixels + dy * dst->pitch + dx;
    if (w == src->pitch && w == dst->pitch) {
        memcpy(d, s, w * h);
    } else {
        for (int y = 0; y < h; y++, s += src->pitch, d += dst->pitch)
            memcpy(d, s, w);
    }
    l2_prof(L2P_BLIT, l2_us() - t);
    return 0;
}

int SDL_FillRect(SDL_Surface *s, SDL_Rect *r, Uint32 color) {
    if (s == screen)
        materialize();
    int x = 0, y = 0, w = s->w, h = s->h;
    if (r) {
        x = r->x;
        y = r->y;
        w = r->w;
        h = r->h;
    }
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > s->w) w = s->w - x;
    if (y + h > s->h) h = s->h - y;
    for (int i = 0; i < h; i++)
        memset((uint8_t *)s->pixels + (y + i) * s->pitch + x, (int)color, w);
    return 0;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b) { return 0; }

static uint32_t prof_us[L2P_COUNT];

void l2_prof(int stage, uint32_t us) { prof_us[stage] += us; }

int SDL_Flip(SDL_Surface *s) {
    if (s != screen || !lcd)
        return 0;
    check_power();
    update_channels();
    uint32_t t0 = l2_us();
    const SDL_Surface *from = shown ? shown : screen;
    if (pending_view) {
        // the full-screen view: 160x160, every pixel, 8 at a time
        for (int y = 0; y < L2_SCREEN_H; y++) {
            const uint32_t *src = (const uint32_t *)(pending_view + y * L2_SCREEN_W);
            uint32_t *d = (uint32_t *)(lcd + y * L2_PITCH);
            for (int x = 0; x < L2_SCREEN_W; x += 8, src += 2, d += 3) {
                uint32_t s0 = src[0], s1 = src[1];
                uint32_t p0 = lut_a[s0 & 0xFF] | lut_b[(s0 >> 8) & 0xFF];
                uint32_t p1 = lut_a[(s0 >> 16) & 0xFF] | lut_b[s0 >> 24];
                uint32_t p2 = lut_a[s1 & 0xFF] | lut_b[(s1 >> 8) & 0xFF];
                uint32_t p3 = lut_a[(s1 >> 16) & 0xFF] | lut_b[s1 >> 24];
                d[0] = p0 | (p1 << 24);
                d[1] = (p1 >> 8) | (p2 << 16);
                d[2] = (p2 >> 16) | (p3 << 8);
            }
        }
        last_view = pending_view;
    } else if (!((uintptr_t)lcd & 3)) {
        last_view = NULL;
        // through the column table, 8 pixels at a time: 3 words written
        for (int y = 0; y < OUT_H; y++) {
            const uint8_t *src = (const uint8_t *)from->pixels + row_src[y] * from->pitch;
            const uint16_t *c = col_src;
            uint32_t *d = (uint32_t *)(lcd + (OUT_Y + y) * L2_PITCH);
            for (int x = 0; x < OUT_W; x += 8, c += 8, d += 3) {
                uint32_t p0 = lut_a[src[c[0]]] | lut_b[src[c[1]]];
                uint32_t p1 = lut_a[src[c[2]]] | lut_b[src[c[3]]];
                uint32_t p2 = lut_a[src[c[4]]] | lut_b[src[c[5]]];
                uint32_t p3 = lut_a[src[c[6]]] | lut_b[src[c[7]]];
                d[0] = p0 | (p1 << 24);
                d[1] = (p1 >> 8) | (p2 << 16);
                d[2] = (p2 >> 16) | (p3 << 8);
            }
        }
    } else {
        last_view = NULL;
        for (int y = 0; y < OUT_H; y++) {
            const uint8_t *src = (const uint8_t *)from->pixels + row_src[y] * from->pitch;
            uint8_t *d = lcd + (OUT_Y + y) * L2_PITCH;
            for (int x = 0; x < OUT_W; x += 2, d += 3) {
                uint16_t a = pal12[src[col_src[x]]], b = pal12[src[col_src[x + 1]]];
                d[0] = (uint8_t)a;
                d[1] = (uint8_t)(((a >> 4) & 0xF0) | (b >> 8));
                d[2] = (uint8_t)b;
            }
        }
    }
    pending_view = NULL;
    uint32_t t1 = l2_us();
    l2_prof(L2P_CONVERT, t1 - t0);

#ifdef L2_PROFILE
    static uint32_t prof_start, prof_frames;
    prof_frames++;
    if (t1 - prof_start >= 1000000) {
        if (prof_start)
            show_profile(prof_frames, t1 - prof_start);
        prof_start = t1;
        prof_frames = 0;
    }
#endif
    l2_present(lcd);
    l2_prof(L2P_LCD, l2_us() - t1);
    last_yield = l2_ticks();
    return 0;
}

int SDL_SaveBMP(SDL_Surface *s, const char *file) { return -1; }
void SDL_WM_SetCaption(const char *title, const char *icon) {}
int SDL_ShowCursor(int toggle) { return 0; }
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode) { return SDL_GRAB_OFF; }

// power button

static uint32_t power_at;      // when it was pressed (ms), 0 if not
static int power_claimed;

int l2_power_pressed(void) { return power_at != 0; }
int l2_power_claimed(void) { return power_claimed; }
void l2_power_claim(void) { power_claimed = 1; }

void l2_power_halt(void) {
    Mix_HaltChannel(-1);
    l2_forget_relaunch();
    if (lcd)
        memset(lcd, 0, L2_SCREEN_H * L2_PITCH);
    for (;;) {
        if (lcd)
            l2_present(lcd);
        l2_sleep(50);
    }
}

// set the lcd to black every few steps
static void fade_lcd(void) {
    if (!lcd)
        return;
    static uint8_t orig[L2_SCREEN_H * L2_PITCH];
    memcpy(orig, lcd, sizeof orig);
    for (int k = 7; k >= 0; k--) {
        for (unsigned i = 0; i < sizeof orig; i++) {
            uint8_t b = orig[i];
            lcd[i] = (uint8_t)((((b >> 4) * k / 8) << 4) | ((b & 15) * k / 8));
        }
        l2_present(lcd);
        l2_sleep(60);
    }
}

void l2_quit(void) {
    static int busy;
    if (!busy) {
        busy = 1;
        Mix_HaltChannel(-1);
        fade_lcd();
        l2_forget_relaunch();
        l2_shut_down();
    }
    l2_power_halt();
}

// check the power button
static void check_power(void) {
    static int busy;
    if (busy)
        return;
    uint32_t now = l2_ticks();
    if (!power_at) {
        if (!l2_power_button())
            return;
        power_at = now ? now : 1;
    }
    uint32_t since = now - power_at;
    if ((!power_claimed && since >= 300) || (power_claimed && since >= 4500)) {
        busy = 1;
        Mix_HaltChannel(-1);
        fade_lcd();
        l2_power_halt();
    }
}

// input

#define QUEUE 32
static SDL_Event queue[QUEUE];
static int q_head, q_tail;
static uint32_t held;      // buttons last seen
static SDLKey b_key;       // what B is pressing (Space, or Insert with Hint)
static SDLKey home_key;    // what Home is pressing (Left Shift, or keypad 5 with Hint: the cheat menu)

static void push_key(int type, SDLKey key) {
#ifdef L2_HOST
    if (getenv("L2_KEYLOG"))
        fprintf(stderr, "%u key %s %d\n", l2_ticks(), type == SDL_KEYDOWN ? "down" : "up", key);
#endif
    int next = (q_tail + 1) % QUEUE;
    if (next == q_head)
        return;
    SDL_Event *e = &queue[q_tail];
    memset(e, 0, sizeof *e);
    e->type = (Uint8)type;
    e->key.keysym.sym = key;
    q_tail = next;
}

static const struct {
    uint32_t button;
    SDLKey key;
} keymap[] = {
    {L2_UP, SDLK_UP}, {L2_DOWN, SDLK_DOWN}, {L2_LEFT, SDLK_LEFT}, {L2_RIGHT, SDLK_RIGHT},
    {L2_A, SDLK_LCTRL}, {L2_HINT, SDLK_LALT}, {L2_PAUSE, SDLK_ESCAPE},
};

static void poll_buttons(void) {
    if (l2_ticks() - last_yield > 200)  // each frame sends counts
        yield();
    uint32_t now = l2_buttons(), changed = now ^ held;
    for (unsigned i = 0; i < sizeof keymap / sizeof keymap[0]; i++)
        if (changed & keymap[i].button)
            push_key(now & keymap[i].button ? SDL_KEYDOWN : SDL_KEYUP, keymap[i].key);
    if (changed & L2_B) {
        if (now & L2_B) {
            b_key = now & L2_HINT ? SDLK_INSERT : SDLK_SPACE;
            push_key(SDL_KEYDOWN, b_key);
        } else {
            push_key(SDL_KEYUP, b_key);
        }
    }
    if (changed & L2_HOME) {
        if (now & L2_HOME) {
            home_key = now & L2_HINT ? SDLK_KP5 : SDLK_UNKNOWN;
            if (home_key == SDLK_KP5) {
                push_key(SDL_KEYDOWN, home_key);
            }
        } else {
            push_key(SDL_KEYUP, home_key);
        }
    }
    static int running;
    int run = (now & L2_B) && b_key == SDLK_SPACE;
    if (run != running) {
        push_key(run ? SDL_KEYDOWN : SDL_KEYUP, SDLK_LSHIFT);
        running = run;
    }
    held = now;
    // halt the powerdown timer as someone is playing
    static uint32_t last_reset;
    if (now && l2_ticks() - last_reset >= 1000) {
        l2_reset_idle();
        last_reset = l2_ticks();
    }
}

int SDL_PollEvent(SDL_Event *e) {
    check_power();
    update_channels();
    if (q_head == q_tail)
        poll_buttons();
    if (q_head == q_tail)
        return 0;
    if (e)
        *e = queue[q_head];
    q_head = (q_head + 1) % QUEUE;
    return 1;
}

int SDL_WaitEvent(SDL_Event *e) {
    while (!SDL_PollEvent(e))
        SDL_Delay(5);
    return 1;
}

Uint8 SDL_EventState(Uint8 type, int state) { return 0; }
SDLMod SDL_GetModState(void) { return KMOD_NUM; }
// (no mouse: always at the centre, where Wolf4SDL keeps warping it)
Uint8 SDL_GetMouseState(int *x, int *y) {
    if (x) *x = screen ? screen->w / 2 : 0;
    if (y) *y = screen ? screen->h / 2 : 0;
    return 0;
}
void SDL_WarpMouse(Uint16 x, Uint16 y) {}

int SDL_NumJoysticks(void) { return 0; }
SDL_Joystick *SDL_JoystickOpen(int index) { return NULL; }
void SDL_JoystickClose(SDL_Joystick *j) {}
int SDL_JoystickNumButtons(SDL_Joystick *j) { return 0; }
int SDL_JoystickNumHats(SDL_Joystick *j) { return 0; }
void SDL_JoystickUpdate(void) {}
Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int axis) { return 0; }
Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int hat) { return 0; }
Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int button) { return 0; }

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
    static SDL_RWops rw;
    rw.base = mem;
    rw.size = (size_t)size;
    return &rw;
}

// sound

// we occupy both voice channels 5 and 6 for audio... 1-4 are likely used for the l2's MIDI system, and idfk what 7 wants from me
// anyhow, we play each sound in 8KHz A-Law data, "upsampling" from the 7042Hz of wolf3d
// you'll be able to see more in id_sd.cpp
// no music here though, because there's too much of a delay for everything to sound okay
// so we basically have a child of the PC-Jaguar-GBA port of wolf3d

#define FIRST_VOICE 5

static struct {
    const Mix_Chunk *chunk;
    uint32_t start, end;  // (ms)
    uint8_t left, right;
    int8_t group;
    uint8_t playing;
    uint8_t *buf;         // (A-Law, for sampled sounds)
    uint32_t buf_size;
} chans[MIX_CHANNELS];
static void (*channel_done)(int channel);

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize) {
    for (int i = 0; i < MIX_CHANNELS; i++) {
        chans[i].left = chans[i].right = 255;
        chans[i].group = -1;
    }
    audio_open = 1;
    return 0;
}

void Mix_CloseAudio(void) {
    Mix_HaltChannel(-1);
    audio_open = 0;
}

const char *Mix_GetError(void) { return "sound error"; }
int Mix_ReserveChannels(int num) { return num; }

int Mix_GroupChannels(int from, int to, int tag) {
    for (int i = from; i <= to && i < MIX_CHANNELS; i++)
        chans[i].group = (int8_t)tag;
    return to - from + 1;
}

static void finish(int channel) {
    chans[channel].playing = 0;
    if (channel_done)
        channel_done(channel);
}

// (ends the sounds whose time is up)
static void update_channels(void) {
    uint32_t now = l2_ticks();
    for (int i = 0; i < MIX_CHANNELS; i++)
        if (chans[i].playing && (int32_t)(now - chans[i].end) >= 0)
            finish(i);
}

int Mix_GroupAvailable(int tag) {
    update_channels();
    for (int i = 0; i < MIX_CHANNELS; i++)
        if (chans[i].group == tag && !chans[i].playing)
            return i;
    return -1;
}

int Mix_GroupOldest(int tag) {
    int oldest = -1;
    for (int i = 0; i < MIX_CHANNELS; i++)
        if (chans[i].group == tag && chans[i].playing &&
            (oldest < 0 || (int32_t)(chans[i].start - chans[oldest].start) < 0))
            oldest = i;
    return oldest;
}

int Mix_SetPanning(int channel, Uint8 left, Uint8 right) {
    if (channel >= 0 && channel < MIX_CHANNELS) {
        chans[channel].left = left;
        chans[channel].right = right;
    }
    return 1;
}

Mix_Chunk *l2_chunk_pcm8(const Uint8 *pcm, Uint32 len, Uint32 rate) {
    Mix_Chunk *c = calloc(1, sizeof *c);
    if (!c)
        return NULL;
    c->abuf = (Uint8 *)pcm;
    c->alen = len;
    c->volume = 128;
    c->rate = rate;
    return c;
}

Mix_Chunk *l2_chunk_alaw(const Uint8 *alaw, Uint32 len) {
    Mix_Chunk *c = l2_chunk_pcm8(alaw, len, 0);
    l2_dcache_flush();  // (the chip reads memory directly)
    return c;
}

Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc) { return NULL; }  // (l2_chunk_* instead)

void Mix_FreeChunk(Mix_Chunk *chunk) {
    for (int i = 0; i < MIX_CHANNELS; i++)
        if (chans[i].playing && chans[i].chunk == chunk)
            Mix_HaltChannel(i);
    free(chunk);
}

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops) {
    if (!audio_open || !chunk)
        return -1;
    update_channels();
    if (channel < 0)
        for (int i = 0; i < MIX_CHANNELS; i++)
            if (!chans[i].playing) {
                channel = i;
                break;
            }
    if (channel < 0 || channel >= MIX_CHANNELS)
        return -1;
    if (chans[channel].playing) {
        l2_voice_stop(FIRST_VOICE + channel);
        finish(channel);
    }
    const uint8_t *data = chunk->abuf;
    uint32_t len = chunk->alen;
    if (chunk->rate) {
        // sampled: into this voice's buffer as A-law
        len = l2_alaw_length(chunk->alen, chunk->rate);
        if (len > chans[channel].buf_size) {
            free(chans[channel].buf);
            chans[channel].buf = malloc(len);
            chans[channel].buf_size = chans[channel].buf ? len : 0;
            if (!chans[channel].buf)
                return -1;
        }
        l2_pcm8_to_alaw(chunk->abuf, chunk->alen, chunk->rate, chans[channel].buf, len);
        l2_dcache_flush();
        data = chans[channel].buf;
    }
    // (one speaker: the louder side's volume, up to 0x4000)
    unsigned vol = chans[channel].left > chans[channel].right ? chans[channel].left : chans[channel].right;
    uint32_t now = l2_ticks();
    chans[channel].chunk = chunk;
    chans[channel].start = now;
    chans[channel].end = now + len * 1000 / L2_VOICE_RATE + 1;
    chans[channel].playing = 1;
    l2_voice_play(FIRST_VOICE + channel, data, len, (uint16_t)(vol * 0x4000 / 255));
    return channel;
}

int Mix_HaltChannel(int channel) {
    for (int i = 0; i < MIX_CHANNELS; i++)
        if ((channel < 0 || channel == i) && chans[i].playing) {
            l2_voice_stop(FIRST_VOICE + i);
            finish(i);
        }
    return 0;
}

void Mix_HookMusic(void (*fn)(void *udata, Uint8 *stream, int len), void *arg) {}  // (no music)
void Mix_ChannelFinished(void (*fn)(int channel)) { channel_done = fn; }

// buzz: "according to my navicomputer, the -"
// woody: "shut up, just shut up you idiot"
// buzz: "sheriff, this is no time to panic"
// woody: "this is the perfect time to panic! i'm lost, andy is gone, they're gonna move from their house in 2 days and it's all your fault!"
// buzz: "my fault? if you hadn't pushed me out of the window in the first place-"
// woody: "oh yeah, well if you hadn't shown up in your stupid little cardboard spaceship and taken away everything that was important to me"
// buzz: "don't talk to me about importance! because of you the security of the entire universe is in jeopardy"
// woody: "WHAT? WHAT ARE YOU TALKING ABOUT?"
