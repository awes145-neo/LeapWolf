#ifndef _STDLIB_H
#define _STDLIB_H
#include "_l2c.h"
L2C_BEGIN
#define RAND_MAX 0x7FFF
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
__attribute__((noreturn)) void exit(int code);
__attribute__((noreturn)) void abort(void);
int atexit(void (*fn)(void));
int atoi(const char *s);
long atol(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
int rand(void);
void srand(unsigned seed);
int abs(int x);
long labs(long x);
char *getenv(const char *name);
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));
L2C_END
#endif
