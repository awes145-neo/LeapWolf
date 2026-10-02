#ifndef L2_SDL_H
#define L2_SDL_H

#ifdef L2_HOST
#include "sdl_rename.h"
#ifdef __cplusplus
extern "C" int wolf_main(int argc, char **argv);  // (the game's main, renamed)
#endif
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t Uint8;
typedef int8_t Sint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef int32_t Sint32;
typedef uint64_t Uint64;
typedef int64_t Sint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_INIT_TIMER 0x01
#define SDL_INIT_AUDIO 0x10
#define SDL_INIT_VIDEO 0x20
#define SDL_INIT_JOYSTICK 0x200
#define SDL_INIT_NOPARACHUTE 0x100000

int SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

#define SDL_arraysize(a) (sizeof(a) / sizeof((a)[0]))
#define SDL_stack_alloc(type, n) ((type *)malloc(sizeof(type) * (n)))
#define SDL_stack_free(p) free(p)
size_t SDL_strlcpy(char *dst, const char *src, size_t size);
size_t SDL_strlcat(char *dst, const char *src, size_t size);
#define SDL_strlen strlen
#define SDL_strrchr strrchr

// video

typedef struct {
    Uint8 r, g, b, unused;
} SDL_Color;

typedef struct {
    int ncolors;
    SDL_Color *colors;
} SDL_Palette;

typedef struct {
    SDL_Palette *palette;
    Uint8 BitsPerPixel;
    Uint8 BytesPerPixel;
} SDL_PixelFormat;

typedef struct {
    Sint16 x, y;
    Uint16 w, h;
} SDL_Rect;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat *format;
    int w, h;
    Uint16 pitch;
    void *pixels;
    SDL_Palette pal;
    SDL_PixelFormat fmt;
    SDL_Color colors[256];
} SDL_Surface;

typedef struct {
    SDL_PixelFormat *vfmt;
} SDL_VideoInfo;

#define SDL_SWSURFACE 0x00000000
#define SDL_HWSURFACE 0x00000001
#define SDL_HWPALETTE 0x20000000
#define SDL_DOUBLEBUF 0x40000000
#define SDL_FULLSCREEN 0x80000000
#define SDL_LOGPAL 0x01
#define SDL_PHYSPAL 0x02
#define SDL_MUSTLOCK(s) 0

const SDL_VideoInfo *SDL_GetVideoInfo(void);
SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags);
SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth, Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am);
void SDL_FreeSurface(SDL_Surface *s);
int SDL_LockSurface(SDL_Surface *s);
void SDL_UnlockSurface(SDL_Surface *s);
int SDL_SetColors(SDL_Surface *s, SDL_Color *colors, int first, int n);
int SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colors, int first, int n);
int SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int SDL_FillRect(SDL_Surface *s, SDL_Rect *r, Uint32 color);
Uint32 SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b);
int SDL_Flip(SDL_Surface *s);
int SDL_SaveBMP(SDL_Surface *s, const char *file);
void SDL_WM_SetCaption(const char *title, const char *icon);
int SDL_ShowCursor(int toggle);

#define SDL_QUERY -1
#define SDL_IGNORE 0
#define SDL_DISABLE 0
#define SDL_ENABLE 1

typedef enum { SDL_GRAB_QUERY = -1, SDL_GRAB_OFF = 0, SDL_GRAB_ON = 1 } SDL_GrabMode;
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode);

// keys

typedef enum {
    SDLK_UNKNOWN = 0,
    SDLK_BACKSPACE = 8, SDLK_TAB = 9, SDLK_RETURN = 13, SDLK_PAUSE = 19, SDLK_ESCAPE = 27,
    SDLK_SPACE = 32,
    SDLK_0 = 48, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9,
    SDLK_a = 97, SDLK_b, SDLK_c, SDLK_d, SDLK_e, SDLK_f, SDLK_g, SDLK_h, SDLK_i, SDLK_j,
    SDLK_k, SDLK_l, SDLK_m, SDLK_n, SDLK_o, SDLK_p, SDLK_q, SDLK_r, SDLK_s, SDLK_t,
    SDLK_u, SDLK_v, SDLK_w, SDLK_x, SDLK_y, SDLK_z,
    SDLK_DELETE = 127,
    SDLK_KP0 = 256, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
    SDLK_KP8, SDLK_KP9,
    SDLK_KP_ENTER = 271,
    SDLK_UP = 273, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT, SDLK_INSERT, SDLK_HOME, SDLK_END,
    SDLK_PAGEUP, SDLK_PAGEDOWN,
    SDLK_F1 = 282, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9,
    SDLK_F10, SDLK_F11, SDLK_F12,
    SDLK_NUMLOCK = 300, SDLK_CAPSLOCK, SDLK_SCROLLOCK, SDLK_RSHIFT, SDLK_LSHIFT, SDLK_RCTRL,
    SDLK_LCTRL, SDLK_RALT, SDLK_LALT,
    SDLK_PRINT = 316,
    SDLK_LAST = 323
} SDLKey;

typedef enum {
    KMOD_NONE = 0, KMOD_LSHIFT = 0x1, KMOD_RSHIFT = 0x2, KMOD_LCTRL = 0x40, KMOD_RCTRL = 0x80,
    KMOD_LALT = 0x100, KMOD_RALT = 0x200, KMOD_NUM = 0x1000, KMOD_CAPS = 0x2000
} SDLMod;
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_CTRL (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_ALT (KMOD_LALT | KMOD_RALT)

SDLMod SDL_GetModState(void);

// events

enum {
    SDL_NOEVENT = 0, SDL_ACTIVEEVENT, SDL_KEYDOWN, SDL_KEYUP, SDL_MOUSEMOTION,
    SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_JOYAXISMOTION, SDL_JOYBALLMOTION,
    SDL_JOYHATMOTION, SDL_JOYBUTTONDOWN, SDL_JOYBUTTONUP, SDL_QUIT
};

#define SDL_APPMOUSEFOCUS 0x01
#define SDL_APPINPUTFOCUS 0x02
#define SDL_APPACTIVE 0x04

typedef struct {
    Uint8 scancode;
    SDLKey sym;
    SDLMod mod;
    Uint16 unicode;
} SDL_keysym;

typedef union {
    Uint8 type;
    struct { Uint8 type, gain, state; } active;
    struct { Uint8 type, which, state; SDL_keysym keysym; } key;
    struct { Uint8 type, which, button, state; } jbutton;
} SDL_Event;

int SDL_PollEvent(SDL_Event *e);
int SDL_WaitEvent(SDL_Event *e);
Uint8 SDL_EventState(Uint8 type, int state);

// mouse and joystick

#define SDL_BUTTON(x) (1 << ((x) - 1))
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
Uint8 SDL_GetMouseState(int *x, int *y);
void SDL_WarpMouse(Uint16 x, Uint16 y);

typedef struct SDL_Joystick SDL_Joystick;
#define SDL_HAT_CENTERED 0
#define SDL_HAT_UP 1
#define SDL_HAT_RIGHT 2
#define SDL_HAT_DOWN 4
#define SDL_HAT_LEFT 8
int SDL_NumJoysticks(void);
SDL_Joystick *SDL_JoystickOpen(int index);
void SDL_JoystickClose(SDL_Joystick *j);
int SDL_JoystickNumButtons(SDL_Joystick *j);
int SDL_JoystickNumHats(SDL_Joystick *j);
void SDL_JoystickUpdate(void);
Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int axis);
Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int hat);
Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int button);

// memory i/o

typedef struct SDL_RWops {
    const Uint8 *base;
    size_t size;
} SDL_RWops;
SDL_RWops *SDL_RWFromMem(void *mem, int size);

#ifdef __cplusplus
}
#endif

#endif
