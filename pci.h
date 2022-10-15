#ifndef PCI_H
#define PCI_H
#include "types.h"

#define	PCI_ProductVendor_REG			0x00
#define Nvme_ProductVendor              0x58458086

#define	PCI_STATUS_REG			        0x04   
#define PCI_STATUS_ENABLES              0x0000000F

#define	PCI_REAL_BAR(prebar)            (((prebar) >> 4) << 4)
#define	PCI_BAR0		                0x10

struct pci_func {
    uint bus;
    uint dev;
    uint func;
    uint productVendor;
    uint BAR;
};

uint
read_pci_config(uint bus, uint device, uint func, uint offset);
uint
write_pci_config(uint bus, uint device, uint func, uint offset, uint val);

void pci_scan(void);
void pci_func_nvme_enable(struct pci_func *f);

#endif
