#ifndef _UNISTD_H
#define _UNISTD_H
#include "_l2c.h"
#include <sys/types.h>
L2C_BEGIN
int read(int fd, void *buf, size_t n);
int write(int fd, const void *buf, size_t n);
off_t lseek(int fd, off_t off, int whence);
int close(int fd);
int unlink(const char *path);
int chdir(const char *path);
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
L2C_END
#endif
