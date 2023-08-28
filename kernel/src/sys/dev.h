#ifndef SYS_DEV_H
#define SYS_DEV_H

#include <stdint.h>
#include <lib/symbol.h>
#include <drivers/pci.h>

typedef enum {
    DEV_DRIVER_PCI
} dev_driver_type_t;

typedef struct {
    dev_driver_type_t type;
    union {
        pci_driver_t *pci;
    };
} dev_driver_t;

#define DEV_REGISTER_PCI(driver)                                                            \
    __attribute__((used, section(".drivers")))                                              \
    static dev_driver_t dev_driver_##driver = { .type = DEV_DRIVER_PCI, .pci = &driver }

#define DEV_FOREACH(DRIVER_PTR) for(DRIVER_PTR = (dev_driver_t *) ld_drivers_start; (uintptr_t) driver < (uintptr_t) ld_drivers_end; driver++)

extern symbol ld_drivers_start;
extern symbol ld_drivers_end;

void dev_initialize();

#endif