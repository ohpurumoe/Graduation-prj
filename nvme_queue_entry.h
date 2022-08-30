#ifndef entry
#define entry
#include "types.h"

struct iosq_entry{
    uchar opcode;
    uchar PSDT_FUSE;
    uchar command_identifier[2]; 
    uint namespace_identifier;
    uint reserved[2];
    uint metadata_pointer[2];
    uint data_pointer[4];
    uint command_specific[6];
};

struct iocq_entry{
    uint command_specific;
    uint reserved;
    uchar sq_head_pointer[2];
    uchar sq_identifier[2];
    uchar command_identifier[2];
    uchar status_field_p[2];  
};

struct nvme_queue{
    int iosq_front , iosq_rear , iocq_front , iocq_rear;
    int p;
    volatile int submission_doorbell;
    volatile int completion_doorbell;
    struct iosq_entry *submission_queue;
    struct iocq_entry *completion_queue;
};
#endif