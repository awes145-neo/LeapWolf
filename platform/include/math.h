#ifndef _MATH_H
#define _MATH_H
#include "_l2c.h"
L2C_BEGIN
#define M_PI 3.14159265358979323846
double sin(double x);
double cos(double x);
double tan(double x);
double atan(double x);
double atan2(double y, double x);
double sqrt(double x);
double floor(double x);
double ceil(double x);
double fabs(double x);
double pow(double x, double y);
double exp(double x);
double log(double x);
double log10(double x);
double fmod(double x, double y);
float sinf(float x);
float cosf(float x);
float sqrtf(float x);
float floorf(float x);
float fabsf(float x);
L2C_END
#endif
