// libm
// contains math for the OPL emulator and accurate precision for the game to work right

#include <stdint.h>
#include <math.h>

static const double LN2 = 0.69314718055994530942;
static const double PI_2 = 1.57079632679489661923;

typedef union {
    double d;
    uint64_t u;
} Bits;

double fabs(double x) { return x < 0 ? -x : x; }
float fabsf(float x) { return x < 0 ? -x : x; }

double floor(double x) {
    if (x >= 4503599627370496.0 || x <= -4503599627370496.0)
        return x;  // (already whole)
    double t = (double)(int64_t)x;
    return t > x ? t - 1 : t;
}

double ceil(double x) { return -floor(-x); }
float floorf(float x) { return (float)floor(x); }

double fmod(double x, double y) {
    if (y == 0)
        return 0;
    double q = x / y;
    q = q < 0 ? ceil(q) : floor(q);
    return x - q * y;
}

// x = m * 2^e with m in [1, 2)
static double split(double x, int *e) {
    Bits b = {x};
    *e = (int)((b.u >> 52) & 0x7FF) - 1023;
    b.u = (b.u & 0x800FFFFFFFFFFFFFull) | (1023ull << 52);
    return b.d;
}

static double scale2(double x, int e) {
    Bits b = {x};
    int exp = (int)((b.u >> 52) & 0x7FF) + e;
    if (exp <= 0)
        return 0;
    if (exp >= 0x7FF)
        exp = 0x7FE;
    b.u = (b.u & 0x800FFFFFFFFFFFFFull) | ((uint64_t)exp << 52);
    return b.d;
}

double sqrt(double x) {
    if (x <= 0)
        return 0;
    int e;
    double m = split(x, &e);
    if (e & 1) {
        m *= 2;
        e--;
    }
    double r = 1.0 + (m - 1.0) * 0.4;  // in [1, 2) for m in [1, 4)
    for (int i = 0; i < 6; i++)
        r = 0.5 * (r + m / r);
    return scale2(r, e / 2);
}

float sqrtf(float x) { return (float)sqrt(x); }

double log(double x) {
    if (x <= 0)
        return -1e300;
    int e;
    double m = split(x, &e);
    if (m > 1.41421356237309504880) {
        m *= 0.5;
        e++;
    }
    // log(m) = 2 atanh((m - 1) / (m + 1))
    double s = (m - 1) / (m + 1), s2 = s * s, term = s, sum = 0;
    for (int k = 1; k < 40; k += 2) {
        sum += term / k;
        term *= s2;
    }
    return 2 * sum + e * LN2;
}

double log10(double x) { return log(x) / 2.30258509299404568402; }

double exp(double x) {
    if (x > 709)
        return 1e308;
    if (x < -745)
        return 0;
    int k = (int)floor(x / LN2 + 0.5);
    double r = x - k * LN2, term = 1, sum = 1;
    for (int n = 1; n < 20; n++) {
        term *= r / n;
        sum += term;
    }
    return scale2(sum, k);
}

double pow(double x, double y) {
    if (y == 0)
        return 1;
    if (x == 0)
        return 0;
    if (x < 0) {
        double r = exp(y * log(-x));
        return ((int64_t)y & 1) ? -r : r;
    }
    return exp(y * log(x));
}

// sin and cos on [-pi/4, pi/4]
static double sin_core(double x) {
    double x2 = x * x, term = x, sum = x;
    for (int n = 2; n < 22; n += 2) {
        term *= -x2 / (n * (n + 1));
        sum += term;
    }
    return sum;
}

static double cos_core(double x) {
    double x2 = x * x, term = 1, sum = 1;
    for (int n = 1; n < 21; n += 2) {
        term *= -x2 / (n * (n + 1));
        sum += term;
    }
    return sum;
}

// quadrant q (0-3) and remainder in [-pi/4, pi/4]
static double reduce(double x, int *q) {
    double k = floor(x / PI_2 + 0.5);
    *q = (int)((int64_t)k & 3);
    // (pi/2 in two parts, for less cancellation)
    return (x - k * 1.57079632673412561417) - k * 6.07710050650619224932e-11;
}

double sin(double x) {
    int q;
    double r = reduce(x, &q);
    switch (q) {
    case 0: return sin_core(r);
    case 1: return cos_core(r);
    case 2: return -sin_core(r);
    default: return -cos_core(r);
    }
}

double cos(double x) {
    int q;
    double r = reduce(x, &q);
    switch (q) {
    case 0: return cos_core(r);
    case 1: return -sin_core(r);
    case 2: return -cos_core(r);
    default: return sin_core(r);
    }
}

double tan(double x) { return sin(x) / cos(x); }
float sinf(float x) { return (float)sin(x); }
float cosf(float x) { return (float)cos(x); }

double atan(double x) {
    int neg = x < 0;
    if (neg)
        x = -x;
    int inv = x > 1;
    if (inv)
        x = 1 / x;
    // atan(x) = pi/6 + atan((x sqrt3 - 1) / (sqrt3 + x)) brings x under tan(pi/12)
    int shift = x > 0.26794919243112270647;
    if (shift)
        x = (x * 1.73205080756887729353 - 1) / (1.73205080756887729353 + x);
    double x2 = x * x, term = x, sum = 0;
    for (int k = 1; k < 30; k += 2) {
        sum += term / k;
        term *= -x2;
    }
    if (shift)
        sum += 0.52359877559829887308;
    if (inv)
        sum = PI_2 - sum;
    return neg ? -sum : sum;
}

double atan2(double y, double x) {
    if (x > 0)
        return atan(y / x);
    if (x < 0)
        return y >= 0 ? atan(y / x) + M_PI : atan(y / x) - M_PI;
    return y > 0 ? PI_2 : y < 0 ? -PI_2 : 0;
}
