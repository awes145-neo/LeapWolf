// bios wrappers

#include "chorus.h"
#include "l2.h"

uint32_t l2_ticks(void) {
    return gp->mpi->kernel->getTimestamp(NULL);
}

uint32_t l2_us(void) {
    uint32_t us = 0, ms = gp->mpi->kernel->getTimestamp(&us);
    return ms * 1000 + us;
}

// sleep
static uint8_t sleep_event[64] __attribute__((aligned(8)));

void l2_sleep(uint32_t ms) {
    void **kernel = (void **)gp->mpi->kernel;
    static int made;
    if (!made) {
        ((bool (*)(void *, uint32_t))kernel[0])(sleep_event, 0);
        made = 1;
    }
    ((bool (*)(void *, uint32_t, uint32_t, uint32_t))kernel[6])(sleep_event, 1, 0, ms);
}

uint32_t l2_buttons(void) {
    struct inputState s;
    gp->mpi->button->getInputState(&s);
    return s.pressedButtons;
}

void l2_present(const uint8_t *fb) {
    while (!gp->mpi->lcd->lockScreen(true)) {}
    gp->mpi->lcd->copyToScreen((uint8_t *)fb, 0, L2_SCREEN_H, true, NULL, true);
    gp->mpi->lcd->unlockScreen();
}

// powermpi
static void **power_mpi(void) {
    return *(void ***)((uint8_t *)gp->mpi + 172);
}

// 0x0180B004 power button, 5 seconds until full powerdown
int l2_power_button(void) {
    return !(*(volatile __attribute__((uncached)) uint32_t *)0x0180B004 & 0x20000000);
}

void l2_shut_down(void) {
    ((void (*)(void))power_mpi()[8])();
    for (;;) {}
}

void l2_forget_relaunch(void) {
    gp->mpi->fat32->fdelete(L2_RELAUNCH_LINK);
}

void l2_reset_idle(void) {
    ((void (*)(void))power_mpi()[5])();
}

// even more audio stuff
typedef struct { uint16_t data, pad; } __attribute__((uncached)) ApuWord;
typedef struct { uint16_t high, pad0, low, pad1; } __attribute__((uncached)) ApuLong;

#define apu_command ((volatile __attribute__((uncached)) uint8_t *)0x01802070)
#define apu_start ((volatile ApuLong *)0x018040C4)
#define apu_end ((volatile ApuLong *)0x01804104)
#define apu_volume0 ((volatile ApuWord *)0x0180413C)
#define apu_decay ((volatile ApuWord *)0x01804184)
#define apu_volume1 ((volatile ApuWord *)0x018041A4)
#define APU_TRIGGER 0x80

void l2_voice_play(int voice, const uint8_t *alaw, uint32_t len, uint16_t volume) {
    uint32_t start = (uint32_t)alaw, end = start + len;
    apu_volume0[voice].data = 0;
    apu_volume1[voice].data = 0;
    apu_end[voice].high = end >> 16;
    apu_end[voice].low = end & 0xFFFF;
    apu_start[voice].high = start >> 16;
    apu_start[voice].low = start & 0xFFFF;
    apu_volume0[voice].data = volume;
    apu_volume1[voice].data = volume;
    apu_decay[voice].data = 0x7FFF;
    *apu_command = APU_TRIGGER | voice;
}

void l2_voice_stop(int voice) {
    apu_volume0[voice].data = 0;
    apu_volume1[voice].data = 0;
}

// audiompi
static void **audio_mpi(void) {
    return *(void ***)((uint8_t *)gp->mpi + 0x40);
}

void l2_feed_start(L2Feed feed, uint8_t volume) {
    ((void (*)(L2Feed, uint32_t))audio_mpi()[69])(feed, 1);
    ((void (*)(uint32_t))audio_mpi()[71])(volume);
}

void l2_feed_stop(void) {
    ((void (*)(void))audio_mpi()[70])();
}

void l2_feed_volume(uint8_t volume) {
    ((void (*)(uint32_t))audio_mpi()[71])(volume);
}

void l2_codec_enable(void) {
    ((void (*)(void))audio_mpi()[68])();
}

void l2_dcache_flush(void) {
    uint32_t ctrl;
    __asm__ volatile("sr 1, [0x4b]" ::: "memory");  // dc_flsh
    do
        __asm__ volatile("lr %0, [0x48]" : "=r"(ctrl) :: "memory");  // dc_ctrl
    while (ctrl & 0x100);
}

void l2_present_lines(const uint8_t *fb, int first, int count) {
    while (!gp->mpi->lcd->lockScreen(true)) {}
    gp->mpi->lcd->copyToScreen((uint8_t *)fb, first, count, true, NULL, true);
    gp->mpi->lcd->unlockScreen();
}

void *l2_malloc(size_t n) { return gp->mpi->kernel->malloc(n); }
void l2_free(void *p) { if (p) gp->mpi->kernel->free(p); }

void *l2_fopen(const char *path, const char *mode) { return gp->mpi->fat32->fopen(path, mode); }
int l2_fclose(void *f) { return gp->mpi->fat32->fclose(f); }
size_t l2_fread(void *buf, size_t size, size_t n, void *f) { return gp->mpi->fat32->fread(buf, size, n, f); }
size_t l2_fwrite(const void *buf, size_t size, size_t n, void *f) {
    return gp->mpi->fat32->fwrite((void *)buf, size, n, f);
}
int l2_fseek(void *f, int32_t off, int whence) { return gp->mpi->fat32->fseek(f, off, whence); }
uint32_t l2_filelength(const char *path) { return gp->mpi->fat32->filelength(path); }
int l2_mkdir(const char *path) { return gp->mpi->fat32->mkdir(path); }
int l2_fdelete(const char *path) { return gp->mpi->fat32->fdelete(path); }
int l2_chdir(const char *path) { return gp->mpi->fat32->chdir(path); }

