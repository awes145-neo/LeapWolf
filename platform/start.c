// starts HomebrewMPI, running C++ functions and making sure that the leapster2 cannot return back from it

#include <stdbool.h>
#include <stdint.h>
#include "chorus.h"
#include "l2.h"

#define STACK_SIZE (256 * 1024)

uint32_t l2_stack[STACK_SIZE / 4] __attribute__((aligned(16)));

extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

void l2_run_on_stack(void (*fn)(void), void *top);

static void run(void) {
    for (void (**f)(void) = __init_array_start; f < __init_array_end; f++)
        (*f)();
    l2_main();
}

static void entry(void) {
#ifdef L2_BIOS_STACK
    run();
#else
    l2_run_on_stack(run, l2_stack + STACK_SIZE / 4);
#endif
    for (;;) {}
}

// The module interface binToRib points the image's MPI descriptor at
static bool hbActiveFlag;

static bool hbInit(void) {
    hbActiveFlag = true;
    entry();
    return true;
}

static bool hbDeInit(void) {
    hbActiveFlag = false;
    return true;
}

static bool hbIsActive(void) { return hbActiveFlag; }
static uint32_t hbGetVersion(void) { return 0x100; }

const char hbMPIName[] = "HomebrewMPI";
const char hbMPIDesc[] = "Hack to get native code running";

static const char *hbGetName(void) { return hbMPIName; }
static const char *hbGetDesc(void) { return hbMPIDesc; }

const struct moduleInterface hbMPI = {
    .init = hbInit,
    .deInit = hbDeInit,
    .isActive = hbIsActive,
    .getVersion0 = hbGetVersion,
    .getVersion1 = hbGetVersion,
    .getVersion2 = hbGetVersion,
    .getName = hbGetName,
    .getDesc = hbGetDesc,
};
