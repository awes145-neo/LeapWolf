// Shared by the C library headers below.
#ifndef L2C_H
#define L2C_H
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#ifdef __cplusplus
#define L2C_BEGIN extern "C" {
#define L2C_END }
#else
#define L2C_BEGIN
#define L2C_END
#endif
#endif
