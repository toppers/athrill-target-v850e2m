#include "exdev_test_abi.h"

#include <stddef.h>

static char test_memory[64];

static void test_devinit(void *unused)
{
    (void)unused;
    test_memory[0] = 1;
}

static void test_supply_clock(void *unused)
{
    (void)unused;
    test_memory[1] = 1;
}

static void test_cleanup(void)
{
    test_memory[2] = 1;
}

ATHRILL_EXDEV_EXPORT AthrillExDevTestDeviceType athrill_ex_device = {
    {
        ATHRILL_EXTERNAL_DEVICE_MAGICNO,
        ATHRILL_EXTERNAL_DEVICE_VERSION,
        (int)sizeof(test_memory)
    },
    test_memory,
    NULL,
    test_devinit,
    test_supply_clock,
    test_cleanup
};
