#ifndef _SYS_STAT_H
#define _SYS_STAT_H
#include "../_l2c.h"
#include <sys/types.h>
L2C_BEGIN
struct stat {
    off_t st_size;
    mode_t st_mode;
};
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_ISDIR(m) (((m) & 0170000) == S_IFDIR)
#define S_ISREG(m) (((m) & 0170000) == S_IFREG)
#define S_IREAD 0400
#define S_IWRITE 0200
int stat(const char *path, struct stat *st);
int mkdir(const char *path, mode_t mode);
L2C_END
#endif
