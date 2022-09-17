#ifndef NVME_H
#define NVME_H

#define ADDRESS_PORT 0xcf8
#define DATA_PORT 0xcfc
#include "pci.h"
#include "nvme_queue_entry.h"

void nvme_attach(struct pci_func *pcif);
void
nvme_command_syn(struct iosq_entry command, struct nvme_queue *que);
struct iosq_entry
nvme_read(uint command_id,uint *lba, uint num_lb, char *read_prps1, char *read_prps2);
struct iosq_entry
nvme_write(uint command_id,uint *lba, uint num_lb, char* write_prps1,char* write_prps2);

struct nvme_queue admin_que;
struct nvme_queue IO_que[8];
#endif	