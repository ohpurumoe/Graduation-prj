#ifndef NVME_H
#define NVME_H

#define ADDRESS_PORT 0xcf8
#define DATA_PORT 0xcfc
#include "pci.h"

void nvme_attach(struct pci_func *pcif);
#endif	