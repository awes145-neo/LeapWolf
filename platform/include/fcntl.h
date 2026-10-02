#ifndef _FCNTL_H
#define _FCNTL_H
#include "_l2c.h"
L2C_BEGIN
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x100
#define O_TRUNC 0x200
#define O_APPEND 0x400
#define O_BINARY 0
#define O_TEXT 0
int open(const char *path, int flags, ...);
L2C_END
#endif
