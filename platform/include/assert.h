#include "_l2c.h"
#undef assert
#ifdef NDEBUG
#define assert(x) ((void)0)
#else
L2C_BEGIN
__attribute__((noreturn)) void l2_assert_fail(const char *expr, const char *file, int line);
L2C_END
#define assert(x) ((x) ? (void)0 : l2_assert_fail(#x, __FILE__, __LINE__))
#endif
