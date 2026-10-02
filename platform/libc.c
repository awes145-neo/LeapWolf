// libc

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// memory management and strings

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    if ((((uintptr_t)d | (uintptr_t)s) & 3) == 0) {
        for (; n >= 16; n -= 16, d += 16, s += 16) {
            ((uint32_t *)d)[0] = ((const uint32_t *)s)[0];
            ((uint32_t *)d)[1] = ((const uint32_t *)s)[1];
            ((uint32_t *)d)[2] = ((const uint32_t *)s)[2];
            ((uint32_t *)d)[3] = ((const uint32_t *)s)[3];
        }
        for (; n >= 4; n -= 4, d += 4, s += 4)
            *(uint32_t *)d = *(const uint32_t *)s;
    }
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d <= s || d >= s + n)
        return memcpy(dst, src, n);
    while (n--)
        d[n] = s[n];
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    uint8_t *d = dst;
    uint32_t w = (uint8_t)c * 0x01010101u;
    while (n && ((uintptr_t)d & 3)) {
        *d++ = (uint8_t)c;
        n--;
    }
    for (; n >= 4; n -= 4, d += 4)
        *(uint32_t *)d = w;
    while (n--)
        *d++ = (uint8_t)c;
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *x = a, *y = b;
    for (; n; n--, x++, y++)
        if (*x != *y)
            return *x - *y;
    return 0;
}

void *memchr(const void *p, int c, size_t n) {
    const uint8_t *s = p;
    for (; n; n--, s++)
        if (*s == (uint8_t)c)
            return (void *)s;
    return NULL;
}

size_t strlen(const char *s) {
    const char *e = s;
    while (*e)
        e++;
    return e - s;
}

char *strcpy(char *d, const char *s) {
    char *r = d;
    while ((*d++ = *s++)) {}
    return r;
}

char *strncpy(char *d, const char *s, size_t n) {
    char *r = d;
    for (; n && *s; n--)
        *d++ = *s++;
    for (; n; n--)
        *d++ = 0;
    return r;
}

char *strcat(char *d, const char *s) {
    strcpy(d + strlen(d), s);
    return d;
}

int strcmp(const char *a, const char *b) {
    for (; *a && *a == *b; a++, b++) {}
    return (uint8_t)*a - (uint8_t)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (; n && *a && *a == *b; n--, a++, b++) {}
    return n ? (uint8_t)*a - (uint8_t)*b : 0;
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

int strcasecmp(const char *a, const char *b) {
    for (; *a && lower(*a) == lower(*b); a++, b++) {}
    return lower((uint8_t)*a) - lower((uint8_t)*b);
}

int strncasecmp(const char *a, const char *b, size_t n) {
    for (; n && *a && lower(*a) == lower(*b); n--, a++, b++) {}
    return n ? lower((uint8_t)*a) - lower((uint8_t)*b) : 0;
}

char *strchr(const char *s, int c) {
    for (;; s++) {
        if (*s == (char)c)
            return (char *)s;
        if (!*s)
            return NULL;
    }
}

char *strrchr(const char *s, int c) {
    const char *r = NULL;
    for (;; s++) {
        if (*s == (char)c)
            r = s;
        if (!*s)
            return (char *)r;
    }
}

char *strstr(const char *h, const char *n) {
    size_t len = strlen(n);
    for (; *h; h++)
        if (!strncmp(h, n, len))
            return (char *)h;
    return len ? NULL : (char *)h;
}

// numbers

long strtol(const char *s, char **end, int base) {
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    int neg = 0;
    if (*s == '-' || *s == '+')
        neg = *s++ == '-';
    if ((base == 0 || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        base = 16;
    } else if (base == 0) {
        base = *s == '0' ? 8 : 10;
    }
    unsigned long v = 0;
    for (;; s++) {
        int d = *s >= '0' && *s <= '9' ? *s - '0'
              : lower(*s) >= 'a' && lower(*s) <= 'z' ? lower(*s) - 'a' + 10 : 99;
        if (d >= base)
            break;
        v = v * base + d;
    }
    if (end)
        *end = (char *)s;
    return neg ? -(long)v : (long)v;
}

unsigned long strtoul(const char *s, char **end, int base) { return (unsigned long)strtol(s, end, base); }
int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }
int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }

// output

typedef struct {
    char *buf;
    size_t size, len;
} Out;

static void out(Out *o, char c) {
    if (o->len + 1 < o->size)
        o->buf[o->len] = c;
    o->len++;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    Out o = {buf, size, 0};
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            out(&o, *fmt);
            continue;
        }
        fmt++;
        int left = 0, zero = 0, plus = 0, space = 0, width = 0, prec = -1, lng = 0;
        for (;; fmt++) {
            if (*fmt == '-') left = 1;
            else if (*fmt == '0') zero = 1;
            else if (*fmt == '+') plus = 1;
            else if (*fmt == ' ') space = 1;
            else if (*fmt == '#') {}
            else break;
        }
        if (*fmt == '*') {
            width = va_arg(ap, int);
            fmt++;
        } else {
            while (*fmt >= '0' && *fmt <= '9')
                width = width * 10 + *fmt++ - '0';
        }
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                fmt++;
            } else {
                while (*fmt >= '0' && *fmt <= '9')
                    prec = prec * 10 + *fmt++ - '0';
            }
        }
        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 't')
            lng += *fmt++ == 'l';

        char tmp[24];
        const char *s = tmp;
        int n = 0;
        char sign = 0;
        switch (*fmt) {
        case 'd': case 'i': case 'u': case 'x': case 'X': case 'p': case 'o': {
            unsigned long long v;
            int base = *fmt == 'x' || *fmt == 'X' || *fmt == 'p' ? 16 : *fmt == 'o' ? 8 : 10;
            if (*fmt == 'p')
                v = (uintptr_t)va_arg(ap, void *);
            else if (*fmt == 'd' || *fmt == 'i') {
                long long sv = lng >= 2 ? va_arg(ap, long long) : lng ? va_arg(ap, long) : va_arg(ap, int);
                if (sv < 0) {
                    sign = '-';
                    v = -(unsigned long long)sv;
                } else {
                    v = sv;
                    sign = plus ? '+' : space ? ' ' : 0;
                }
            } else {
                v = lng >= 2 ? va_arg(ap, unsigned long long) : lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned);
            }
            const char *digits = *fmt == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
            char *e = tmp + sizeof tmp;
            do {
                *--e = digits[v % base];
                v /= base;
            } while (v);
            while (prec > tmp + sizeof tmp - e)
                *--e = '0';
            s = e;
            n = tmp + sizeof tmp - e;
            break;
        }
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            n = 1;
            break;
        case 's':
            s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            for (n = 0; s[n] && (prec < 0 || n < prec); n++) {}
            zero = 0;
            break;
        case '%':
            tmp[0] = '%';
            n = 1;
            break;
        default:  // (floats and anything else: printed as '?')
            if (*fmt == 'f' || *fmt == 'g' || *fmt == 'e')
                (void)va_arg(ap, double);
            tmp[0] = '?';
            n = 1;
            break;
        }
        int pad = width - n - (sign != 0);
        if (!left && !zero)
            for (; pad > 0; pad--) out(&o, ' ');
        if (sign)
            out(&o, sign);
        if (!left && zero)
            for (; pad > 0; pad--) out(&o, '0');
        for (int i = 0; i < n; i++)
            out(&o, s[i]);
        for (; pad > 0; pad--)
            out(&o, ' ');
        if (!*fmt)
            break;
    }
    if (size)
        buf[o.len < size ? o.len : size - 1] = 0;
    return (int)o.len;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, (size_t)-1 >> 1, fmt, ap);
    va_end(ap);
    return n;
}
