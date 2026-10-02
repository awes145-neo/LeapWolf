// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2025, 2026 toadster172 <toadster172@gmail.com>

#ifndef CHORUS_H
#define CHORUS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct moduleInterface
{
    bool (*init)(void);
    bool (*deInit)(void);
    bool (*isActive)(void);
    uint32_t (*getVersion0)(void);
    uint32_t (*getVersion1)(void);
    uint32_t (*getVersion2)(void);
    const char *(*getName)(void);
    const char *(*getDesc)(void);
};

struct mpiTriplet
{
    struct moduleInterface *genericItf;
    void *specificItf;
    uint32_t unk_8__b;
};

#define MPI_TRIPLET(X) struct moduleInterface *X##GenericItf; \
                       struct X##MPI *X; \
                       uint32_t unk__##X;

// Some opaque types that get passed to MQX via pointer
typedef uint8_t mqxEvent[0x20];

struct kernelMPI
{
    bool (*eventCreate)(mqxEvent *e);
    void *unk_4__b[2];
    uint32_t (*getEventBits)(mqxEvent *e);
    bool (*eventSet)(mqxEvent *e);
    bool (*eventClear)(mqxEvent *e);
    bool (*eventWait)(mqxEvent *e);
    void *unk_18__1b;
    void *todo_1c__c7[0x2A];
    void *(*malloc)(uint32_t size);
    void *(*mallocZero)(uint32_t size);
    void *todo_d0__ef[8];
    bool (*free)(void *ptr);
    void *todo_f4__197[0x29];
    void (*enableInterrupts)(void);
    void (*disableInterrupts)(void);
    void *unknown_1a0__1cf[0xC];
    void (*vprintf)(const char *format, va_list args); // Almost certainly not ABI compatible with LF compiler
    void *unknown_1d4__20f[0xF];
    // Returns a timestamp since boot in milliseconds. Might actually return a uint64_t
    //   but 2^32 ms is 50 days so it doesn't really matter anyway. us has a max of 999 and is
    //   effectively the remainder of the division converting hardware timer ticks to ms.
    //   Hardware timer ticks at 16 MHz.
    uint32_t (*getTimestamp)(uint32_t *us);
};

#define BUTTON_RIGHT           (0x0000'0080)
#define BUTTON_DOWN            (0x0000'0100)
#define BUTTON_LEFT            (0x0000'0200)
#define BUTTON_B               (0x0000'2000)
#define BUTTON_A               (0x0000'4000)
#define BUTTON_VOLUME_DOWN     (0x0000'8000)
#define BUTTON_VOLUME_UP       (0x0001'0000)
#define BUTTON_BRIGHTNESS_DOWN (0x0002'0000)
#define BUTTON_BRIGHTNESS_UP   (0x0100'0000)
#define BUTTON_CONTRAST        (0x0200'0000)
#define BUTTON_PAUSE           (0x0400'0000)
#define BUTTON_HINT            (0x1000'0000)
#define BUTTON_HOME            (0x2000'0000)
#define BUTTON_UP              (0x8000'0000)

struct inputState {
    uint32_t pressedButtons;
    uint32_t mostRecentlyPressed;
};

struct buttonMPI {
    bool (*setUnk)(uint32_t unk);
    void (*getInputState)(struct inputState *s);
};

struct lcdMPI
{
    void *unk_0__3;
    bool (*lockScreen)(bool unk);
    bool (*unlockScreen)(void);
    void *todo_C__43[0x0E];
    bool (*copyToScreen)(uint8_t *buf, int32_t start, uint32_t scanlines, bool block, void (*completedCallback)(bool success), bool flushCache);
};

typedef void chorusFile;

#define CHORUS_SEEK_SET  0
#define CHORUS_SEEK_CURR 1
#define CHORUS_SEEK_END  2

// Device 0 is the "RAM disk" (whatever that's actually backed by)
// Device 1 is the internal storage / SD card.
// BIOS programming implies 8 devices max, but it doesn't seem anything above 1 is used
// Paths use windows-style drive conventions, with A: and B: being 0 and 1 respectively

// LF seems to have licensed their driver from smxFS. https://www.smxrtos.com/doc/smxfs.pdf
struct fat32MPI {
    void *unk0__17[6];
    uint32_t (*freeKB)(uint32_t device);
    uint32_t (*totalKB)(uint32_t device);
    uint32_t (*ioctl)(uint32_t device, uint32_t cmd, uint32_t unk); // ?????? Name taken from debug text
    bool (*writeProtect)(uint32_t device);
    // I'm not sure whether 'b' in the mode actually does anything. The function doesn't seem
    //   to check it, but some of the calls in the BIOS do specify a mode with it
    chorusFile *(*fopen)(const char *path, const char *mode);
    int (*fclose)(chorusFile *f);
    size_t (*fread)(void *buf, size_t size, size_t n, chorusFile *f);
    size_t (*fwrite)(void *buf, size_t size, size_t n, chorusFile *f);
    int (*fseek)(chorusFile *f, int32_t off, int whence);
    int (*fflush)(chorusFile *f);
    int (*fdelete)(const char *path);
    uint32_t (*filelength)(const char *path); // Return 0xFFFF'FFFF on error
    bool (*findfile)(const char *path); // Checks if a file exists?
    int (*mkdir)(const char *path);
    int (*rmdir)(const char *path);
    bool (*chdir)(const char *path);
    bool (*setcwd)(const char *path); // Literally the same thing as chdir
    int (*getcwd)(const char *buf, int maxLen);
    void *unk60__6F[4];
    bool (*rename)(const char *oldName, const char *newName);
};

struct mpiTable
{
    uint8_t unk_0__b[0x0C];
    MPI_TRIPLET(kernel);
    uint8_t todo0[0x08 * 12];
    MPI_TRIPLET(button);
    uint8_t todo1[0x04 * 12];
    MPI_TRIPLET(lcd);
    uint8_t todo2[0x25 * 12];
    MPI_TRIPLET(fat32);
};

struct gpStruct
{
    void *unk_0;
    struct mpiTable *mpi;
    void *unk_8__3f[14];
};

struct ribHeader {
    char copyright[24];
    uint16_t version;
    uint16_t ribCount;
    void *deviceStart;
    void *deviceEnd;
    uint32_t *fullChecksum;
    uint32_t *sparseChecksum;
    void *bootSafeFcnTable;
    uint32_t reserved[4];
    void *ribStart;
};

// Included so that clangd doesn't yell at me, since LLVM doesn't support ARC
#ifndef __arc__
    extern struct gpStruct *gp;
#else
    register struct gpStruct *gp asm("gp");
#endif

#endif
