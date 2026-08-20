#ifndef ATHRILL_EXDEV_TEST_ABI_H
#define ATHRILL_EXDEV_TEST_ABI_H

#include "athrill_exdev_header.h"

typedef void (*AthrillExDevTestCallbackType)(void *);

typedef struct {
    AthrillExDeviceHeaderType header;
    char *datap;
    void *ops;
    AthrillExDevTestCallbackType devinit;
    AthrillExDevTestCallbackType supply_clock;
    void (*cleanup)(void);
} AthrillExDevTestDeviceType;

#endif
