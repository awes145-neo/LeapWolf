// io libc
// files on the fat driver, paths are relative to folders and a fatal error shows the log tail

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "l2.h"

struct L2File {
    void *h;       // the BIOS's file
    long pos, len;  // (the BIOS has no ftell)
    int used;
};

#define MAX_FILES 16
static struct L2File files[MAX_FILES];
static struct L2File std_files[3];
FILE *stdin = &std_files[0], *stdout = &std_files[1], *stderr = &std_files[2];

static void fix_path(char *out, const char *path) {
    size_t i = 0;
    for (; path[i] && i < 255; i++)
        out[i] = path[i] == '/' ? '\\' : path[i];
    out[i] = 0;
}

FILE *fopen(const char *path, const char *mode) {
    char p[256];
    fix_path(p, path);
    struct L2File *f = NULL;
    for (int i = 0; i < MAX_FILES; i++)
        if (!files[i].used) {
            f = &files[i];
            break;
        }
    if (!f)
        return NULL;
    int reading = mode[0] == 'r' && !strchr(mode, '+');
    uint32_t len = l2_filelength(p);
    if (reading && len == 0xFFFFFFFF)
        return NULL;
    char m[4] = {mode[0], 'b', 0, 0};
    if (strchr(mode, '+'))
        m[2] = '+';
    f->h = l2_fopen(p, m);
    if (!f->h)
        return NULL;
    f->used = 1;
    f->len = mode[0] == 'w' || len == 0xFFFFFFFF ? 0 : (long)len;
    f->pos = mode[0] == 'a' ? f->len : 0;
    return f;
}

int fclose(FILE *f) {
    if (!f || !f->used)
        return EOF;
    l2_fclose(f->h);
    f->used = 0;
    return 0;
}

size_t fread(void *buf, size_t size, size_t n, FILE *f) {
    if (!f || !f->used || !size)
        return 0;
    // (in 16 KB pieces: the BIOS's limit per call is unknown)
    size_t want = size * n, got = 0;
    while (got < want) {
        size_t piece = want - got > 16384 ? 16384 : want - got;
        long r = (long)l2_fread((uint8_t *)buf + got, 1, piece, f->h);
        if (r <= 0)
            break;
        got += (size_t)r;
        if ((size_t)r < piece)
            break;
    }
    f->pos += got;
    return got / size;
}

size_t fwrite(const void *buf, size_t size, size_t n, FILE *f) {
    if (f == stdout || f == stderr) {
        // (printf's own path)
        for (size_t i = 0; i < size * n; i++)
            putchar(((const char *)buf)[i]);
        return n;
    }
    if (!f || !f->used || !size)
        return 0;
    size_t put = l2_fwrite(buf, 1, size * n, f->h);
    if ((long)put > 0) {
        f->pos += put;
        if (f->pos > f->len)
            f->len = f->pos;
    } else {
        put = 0;
    }
    return put / size;
}

int fseek(FILE *f, long off, int whence) {
    if (!f || !f->used)
        return -1;
    long to = whence == SEEK_SET ? off : whence == SEEK_CUR ? f->pos + off : f->len + off;
    if (to < 0 || l2_fseek(f->h, to, SEEK_SET) < 0)
        return -1;
    f->pos = to;
    return 0;
}

long ftell(FILE *f) { return f && f->used ? f->pos : -1; }
int fflush(FILE *f) { return 0; }

int fgetc(FILE *f) {
    unsigned char c;
    return fread(&c, 1, 1, f) == 1 ? c : EOF;
}

int fputc(int c, FILE *f) {
    unsigned char ch = (unsigned char)c;
    return fwrite(&ch, 1, 1, f) == 1 ? ch : EOF;
}

int fputs(const char *s, FILE *f) { return fwrite(s, 1, strlen(s), f) ? 0 : EOF; }

int remove(const char *path) {
    char p[256];
    fix_path(p, path);
    return l2_fdelete(p) == 0 ? 0 : -1;
}

int unlink(const char *path) { return remove(path); }

int stat(const char *path, struct stat *st) {
    char p[256];
    fix_path(p, path);
    uint32_t len = l2_filelength(p);
    if (len == 0xFFFFFFFF)
        return -1;
    st->st_size = (off_t)len;
    st->st_mode = S_IFREG;
    return 0;
}

int mkdir(const char *path, mode_t mode) {
    char p[256];
    fix_path(p, path);
    return l2_mkdir(p) == 0 ? 0 : -1;
}

// posix descriptors

int open(const char *path, int flags, ...) {
    const char *mode = (flags & 3) == O_RDONLY ? "rb" : (flags & O_TRUNC) || (flags & O_CREAT) ? "wb" : "r+b";
    FILE *f = fopen(path, mode);
    return f ? (int)(f - files) + 3 : -1;
}

static FILE *fd_file(int fd) {
    return fd >= 3 && fd < 3 + MAX_FILES && files[fd - 3].used ? &files[fd - 3] : NULL;
}

int read(int fd, void *buf, size_t n) {
    FILE *f = fd_file(fd);
    return f ? (int)fread(buf, 1, n, f) : -1;
}

int write(int fd, const void *buf, size_t n) {
    if (fd == 1 || fd == 2)
        return (int)fwrite(buf, 1, n, stdout);
    FILE *f = fd_file(fd);
    return f ? (int)fwrite(buf, 1, n, f) : -1;
}

off_t lseek(int fd, off_t off, int whence) {
    FILE *f = fd_file(fd);
    if (!f || fseek(f, off, whence))
        return -1;
    return f->pos;
}

int close(int fd) {
    FILE *f = fd_file(fd);
    return f ? fclose(f) : -1;
}

int chdir(const char *path) {
    char p[256];
    fix_path(p, path);
    return l2_chdir(p) ? 0 : -1;
}

// log

#define LOG_LINES 10
static char log_tail[LOG_LINES][21];  // the last lines, for the crash screen
static int log_row, log_col;
static FILE *log_file;

void l2_io_init(const char *dir) {
    chdir(dir);
    log_file = fopen("wolf3d.log", "w");
}

// (the log is written in 4 KB blocks: one BIOS call a character is slow)
static char log_buf[4096];
static size_t log_len;

static void log_flush(void) {
    if (log_file && log_len)
        fwrite(log_buf, 1, log_len, log_file);
    log_len = 0;
}

int putchar(int c) {
    if (log_file) {
        log_buf[log_len++] = (char)c;
        if (log_len == sizeof log_buf)
            log_flush();
    }
    if (c == '\n' || log_col >= 20) {
        log_row = (log_row + 1) % LOG_LINES;
        log_col = 0;
        memset(log_tail[log_row], 0, 21);
        if (c == '\n')
            return c;
    }
    log_tail[log_row][log_col++] = (char)c;
    return c;
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
    char buf[512];
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    if (f == stdout || f == stderr) {
        for (char *s = buf; *s; s++)
            putchar(*s);
        return n;
    }
    return (int)fwrite(buf, 1, strlen(buf), f);
}

int vprintf(const char *fmt, va_list ap) { return vfprintf(stdout, fmt, ap); }

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return n;
}

int fprintf(FILE *f, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

int puts(const char *s) {
    printf("%s\n", s);
    return 0;
}

int vsprintf(char *buf, const char *fmt, va_list ap) { return vsnprintf(buf, (size_t)-1 >> 1, fmt, ap); }

// ending

static void (*exit_fns[8])(void);
static int n_exit_fns;

int atexit(void (*fn)(void)) {
    if (n_exit_fns == 8)
        return -1;
    exit_fns[n_exit_fns++] = fn;
    return 0;
}

// Shows the log's last lines; there's no way back to the system menu.
static __attribute__((noreturn)) void stop(const char *why) {
    l2_forget_relaunch();  // (so a restart doesn't come back here)
    if (log_file) {
        log_flush();
        fclose(log_file);
        log_file = NULL;
    }
    static uint8_t fb[L2_SCREEN_H * L2_PITCH];
    memset(fb, 0, sizeof fb);
    l2_con_init(fb);
    l2_con_color(0xFF0, 0x000);
    l2_con_printf("%s\n", why);
    l2_con_color(0xFFF, 0x000);
    for (int i = 1; i <= LOG_LINES - 1; i++)
        l2_con_printf("%.20s\n", log_tail[(log_row + 1 + i) % LOG_LINES]);
    for (;;)
        l2_present(fb);
}

void exit(int code) {
    while (n_exit_fns)
        exit_fns[--n_exit_fns]();
    if (code == 0) {
        if (log_file) {
            log_flush();
            fclose(log_file);
            log_file = NULL;
        }
        l2_quit();  // (the game's own Quit: back to the home menu next time)
    }
    stop("Stopped (error)");
}

void abort(void) { stop("Aborted"); }

void l2_assert_fail(const char *expr, const char *file, int line) {
    printf("assert %s\n%s:%d\n", expr, file, line);
    stop("Assertion failed");
}

// memory

static uint32_t heap_now, heap_peak;  // (bytes in use, for the log)

void *malloc(size_t n) {
    uint32_t *p = l2_malloc(n + 8);
    if (!p) {
        printf("malloc(%lu) failed: %luK in use\n", (unsigned long)n, (unsigned long)(heap_now >> 10));
        return NULL;
    }
    p[0] = n;
    heap_now += n + 8;
    if (heap_now > heap_peak)
        heap_peak = heap_now;
    return p + 2;
}

void free(void *p) {
    if (p) {
        heap_now -= ((uint32_t *)p)[-2] + 8;
        l2_free((uint32_t *)p - 2);
    }
}

void l2_heap_report(const char *when) {
    printf("heap %s: %luK in use, peak %luK\n", when, (unsigned long)(heap_now >> 10), (unsigned long)(heap_peak >> 10));
}

void *calloc(size_t n, size_t size) {
    void *p = malloc(n * size);
    if (p)
        memset(p, 0, n * size);
    return p;
}

void *realloc(void *p, size_t n) {
    if (!p)
        return malloc(n);
    size_t old = ((uint32_t *)p)[-2];
    if (n <= old)
        return p;
    void *q = malloc(n);
    if (q) {
        memcpy(q, p, old);
        free(p);
    }
    return q;
}

// other bits

static uint32_t rand_state = 1;
int rand(void) {
    rand_state = rand_state * 1103515245 + 12345;
    return (rand_state >> 16) & RAND_MAX;
}
void srand(unsigned seed) { rand_state = seed; }

time_t time(time_t *t) {
    time_t now = (time_t)(l2_ticks() / 1000);
    if (t)
        *t = now;
    return now;
}

char *getenv(const char *name) { return NULL; }

char *strdup(const char *s) {
    char *d = malloc(strlen(s) + 1);
    return d ? strcpy(d, s) : NULL;
}

char *strncat(char *d, const char *s, size_t n) {
    char *e = d + strlen(d);
    for (; n && *s; n--)
        *e++ = *s++;
    *e = 0;
    return d;
}

char *strerror(int e) { return "error"; }

void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *)) {
    // (insertion sort: only ever used on short lists)
    char *b = base, tmp[64];
    for (size_t i = 1; i < n; i++)
        for (size_t j = i; j > 0 && cmp(b + (j - 1) * size, b + j * size) > 0; j--) {
            memcpy(tmp, b + j * size, size);
            memcpy(b + j * size, b + (j - 1) * size, size);
            memcpy(b + (j - 1) * size, tmp, size);
        }
}

size_t SDL_strlcpy(char *dst, const char *src, size_t size) {
    size_t len = strlen(src);
    if (size) {
        size_t n = len < size - 1 ? len : size - 1;
        memcpy(dst, src, n);
        dst[n] = 0;
    }
    return len;
}

size_t SDL_strlcat(char *dst, const char *src, size_t size) {
    size_t d = strlen(dst);
    return d + SDL_strlcpy(dst + d, src, size > d ? size - d : 0);
}

// C++ runtime bits for -fno-exceptions code
void *__dso_handle;
int __cxa_atexit(void (*fn)(void *), void *arg, void *dso) { return 0; }
void __cxa_pure_virtual(void) { abort(); }
