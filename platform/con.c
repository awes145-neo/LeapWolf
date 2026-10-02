// codepage 437 console

#include <stdarg.h>
#include <stdio.h>
#include "l2.h"

static const uint8_t font[256 * 16] = {
#embed "CP437.F16"
};

static struct {
    uint8_t *fb;
    uint16_t fg, bg;
    int col, row;
} con = {.fg = 0xFFF};

void l2_con_init(uint8_t *fb) {
    con.fb = fb;
    con.col = con.row = 0;
}

void l2_con_at(int col, int row) {
    con.col = col;
    con.row = row;
}

void l2_con_color(uint16_t fg, uint16_t bg) {
    con.fg = fg;
    con.bg = bg;
}

static void con_putc(unsigned char c) {
    if (c == '\n' || con.col >= 20) {
        con.col = 0;
        con.row = (con.row + 1) % 10;
        if (c == '\n')
            return;
    }
    for (int y = 0; y < 16; y++) {
        uint8_t bits = font[c * 16 + y];
        for (int x = 0; x < 8; x++)
            l2_pset(con.fb, con.col * 8 + x, con.row * 16 + y, bits & (0x80 >> x) ? con.fg : con.bg);
    }
    con.col++;
}

void l2_con_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    for (const char *s = buf; *s; s++)
        con_putc((unsigned char)*s);
}
