#include "shared_library.h"
#include "exdev_test_abi.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    AthrillSharedLibraryHandle library;
    AthrillExDevTestDeviceType *device;
    char error_message[512];

    if (argc != 2) {
        fprintf(stderr, "usage: %s EXDEV_LIBRARY\n", argv[0]);
        return 1;
    }

    library = athrill_shared_library_open(
        argv[1], error_message, sizeof(error_message));
    if (library == NULL) {
        fprintf(stderr, "failed to open EXDEV library: %s\n", error_message);
        return 2;
    }

    device = (AthrillExDevTestDeviceType *)athrill_shared_library_symbol(
        library, "athrill_ex_device", error_message, sizeof(error_message));
    if (device == NULL) {
        fprintf(stderr, "failed to resolve athrill_ex_device: %s\n",
            error_message);
        athrill_shared_library_close(library);
        return 3;
    }
    if (device->header.magicno != ATHRILL_EXTERNAL_DEVICE_MAGICNO
        || device->header.version != ATHRILL_EXTERNAL_DEVICE_VERSION
        || device->header.memory_size != 64
        || device->datap == NULL
        || device->devinit == NULL
        || device->supply_clock == NULL
        || device->cleanup == NULL) {
        fprintf(stderr, "invalid EXDEV ABI descriptor\n");
        athrill_shared_library_close(library);
        return 4;
    }

    device->devinit(NULL);
    device->supply_clock(NULL);
    device->cleanup();
    if (device->datap[0] != 1 || device->datap[1] != 1
        || device->datap[2] != 1) {
        fprintf(stderr, "EXDEV callbacks were not invoked\n");
        athrill_shared_library_close(library);
        return 5;
    }

    error_message[0] = '\0';
    if (athrill_shared_library_symbol(
            library, "missing_exdev_symbol",
            error_message, sizeof(error_message)) != NULL
        || strlen(error_message) == 0U) {
        fprintf(stderr, "missing symbol did not report an error\n");
        athrill_shared_library_close(library);
        return 6;
    }

    athrill_shared_library_close(library);
    return 0;
}
