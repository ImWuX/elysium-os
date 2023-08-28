#include "dev.h"
#include <drivers/pci.h>

void dev_initialize() {
    pci_enumerate(acpi_find_table((uint8_t *) "MCFG"));
}