#include "x86.h"
#include "pci.h"
#include "nvme.h"
#include "defs.h"
#include "memlayout.h"

uint
read_pci_config(uint bus, uint device, uint func, uint offset)
{
   uint v;
   outl(ADDRESS_PORT, KERNBASE| (bus<<16) | (device << 11) | (func<<8) | offset);
   v= inl(DATA_PORT);
   return v;
}

uint
write_pci_config(uint bus, uint device, uint func, uint offset, uint val)
{
	outl(ADDRESS_PORT ,KERNBASE| (bus << 16) | (device << 11) | (func<<8) | offset);
	outl(DATA_PORT,val);
	return 0;
}

void
pci_scan(void)
{
	for (int bus = 0; bus < 256; bus++){
		for (int dev = 0; dev < 32; dev++){
			for (int func = 0; func < 8; func++){
				struct pci_func f;
				memset(&f, 0, sizeof(f));
				f.productVendor = read_pci_config(bus,dev,func, PCI_ProductVendor_REG);
				f.bus = bus;
				f.dev = dev;
				f.func = func;
				if((f.productVendor == Nvme_ProductVendor)){
					nvme_attach(&f);
				}
			}
		}
	}
}

void
pci_func_nvme_enable(struct pci_func *f)
{
	write_pci_config(f->bus,f->dev,f->func, PCI_STATUS_REG, PCI_STATUS_ENABLES);
	f->BAR = PCI_REAL_BAR(read_pci_config(f->bus,f->dev,f->func, PCI_BAR0));
}