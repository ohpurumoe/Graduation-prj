#include "queue.h"
#include "param.h"
#include "defs.h"
#include "nvme_queue_entry.h"

void 
init_que(struct nvme_queue *que, uint sqsz, uint cqsz)
{
    que->iosq_front = 0;
    que->iosq_rear = 0;
    que->iocq_front = 0;
    que->iocq_rear = 0;
    que->p = 1;

}
