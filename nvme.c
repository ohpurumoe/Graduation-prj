#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "pci.h"
#include "nvme.h"
#include "nvme_queue_entry.h"
#include "nvme_field.h"
#include "queue.h"
#include "buddy.h"


void
aqa_setting(uint bar)
{
  volatile uint *aqa = (volatile uint*)(bar+0x24);
  uint tmp = *aqa;
  tmp =0x0;
  tmp |=0x00400040;
  *aqa= tmp;
}

void
asq_setting(uint bar)
{
  volatile uint *asq_ba_low = (volatile uint*)(bar+0x28);
  volatile uint *asq_ba_upper = (volatile uint*)(bar+0x2c);
  admin_que.submission_queue = ((struct iosq_entry* )kalloc());
  memset(admin_que.submission_queue,0,4096);
  uint asq_space = V2P((uint)(admin_que.submission_queue));
  *asq_ba_low = asq_space;
  *asq_ba_upper = 0; 
}


void
acq_setting(uint bar)
{
  volatile uint *acq_ba_low = (volatile uint*)(bar+0x30);
  volatile uint *acq_ba_upper = (volatile uint*)(bar+0x34);
  admin_que.completion_queue = ((struct iocq_entry *)kalloc());
  memset(admin_que.completion_queue,0,4096);
  uint acq_space = V2P((uint)(admin_que.completion_queue));
  *acq_ba_low = acq_space;
  *acq_ba_upper = 0; 
}

void cc_setting(uint bar)
{
  volatile uint *cc = (volatile uint*)(bar+0x14);
  *cc |= 0x00460001;
}

int
cmp_commandid(struct iosq_entry command1, struct iocq_entry command2)
{
  if(command1.command_identifier[0] != command2.command_identifier[0]) {
    return -1;
  }
  if(command1.command_identifier[1] != command2.command_identifier[1]) {
    return -1;
  }
  return 1;
}

void
nvme_command_syn(struct iosq_entry command, struct nvme_queue *que)
{
  que->submission_queue[que->iosq_rear] = command;
  *(volatile uint*)que->submission_doorbell = ++que->iosq_rear;


  while((get_P(que->completion_queue[que->iocq_front]) != que->p));
  for(;;){  
    //cli();
    //cli();
    microdelay(0);
    //cprintf();
    //delay();  
    ushort SID = command.command_identifier[1]*0xff +command.command_identifier[0];
    struct iocq_entry tmp = que->completion_queue[que->iocq_front];  
    ushort CID = tmp.command_identifier[1]*0xff + tmp.command_identifier[0];
    if(SID==CID) break;
  }  
  *(volatile uint*)que->completion_doorbell = ++que->iocq_front;
  if(que->iosq_rear >= QSIZE) que->iosq_rear %= QSIZE;
  if(que->iocq_front >= QSIZE) {
    que->iocq_front %= QSIZE;
    if (que->p) que->p = 0;
    else que->p = 1;
  }
}

struct iosq_entry 
nvme_identify(uint command_id, uint controller_id, uint cns)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x6;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;
  ret.data_pointer[0]=V2P((uint)(kalloc()));
  ret.data_pointer[1]=0;
  ret.data_pointer[2]=0;
  ret.data_pointer[3]=0;
  admin_identify_set_controller_identifier(controller_id,&ret);
  admin_identify_set_CNS(cns,&ret);
  return ret;
}

struct iosq_entry
nvme_set_feature(uint command_id, uint sv)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x9;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;
  ret.data_pointer[0]=V2P((uint)kalloc());
  ret.data_pointer[1]=0;
  ret.data_pointer[2]=V2P((uint)kalloc());
  ret.data_pointer[3]=0;
  admin_set_feature_set_feature_identfier(0x7,&ret);
  admin_set_feature_set_sv(sv,&ret);
  ret.command_specific[1] = 0x00080008;
  return ret;
}

struct iosq_entry
nvme_create_iocq(uint command_id, uint qid)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x5;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;
  ret.data_pointer[0]=V2P((uint)kalloc());
  IO_que[qid].completion_queue = P2V(ret.data_pointer[0]);
  ret.data_pointer[1]=0;
  admin_create_io_completion_queue_set_queue_identfier(qid+1,&ret);
  admin_create_io_completion_queue_set_queue_size(64,&ret);
  admin_create_io_completion_queue_set_pc(1,&ret);
  admin_create_io_completion_queue_set_ien(0,&ret);
  admin_create_io_completion_queue_set_interrupt_vector(0,&ret);
  return ret;
}


struct iosq_entry
nvme_create_iosq(uint command_id, uint qid)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x1;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;
  ret.data_pointer[0]=V2P((uint)kalloc());
  IO_que[qid].submission_queue = P2V(ret.data_pointer[0]);
  ret.data_pointer[1]=0;
  admin_create_io_submission_queue_set_completion_queue_identifier(qid+1,&ret);
  admin_create_io_submission_queue_set_pc(1,&ret);
  admin_create_io_submission_queue_set_QPRIO(0,&ret);
  admin_create_io_submission_queue_set_queue_identfier(qid+1,&ret);
  admin_create_io_submission_queue_set_queue_size(64,&ret);
  return ret;
}


struct iosq_entry
nvme_write(uint command_id,uint *lba, uint num_lb, char* write_prps1,char* write_prps2)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x1;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;
  ret.metadata_pointer[0] = 0;
  ret.metadata_pointer[1] = 0;

  ret.data_pointer[0] = V2P((uint)write_prps1);
  ret.data_pointer[2] = V2P((uint)write_prps2);

  rw_command_set_lba(lba,&ret);
  rw_command_set_num_lb(num_lb,&ret);
  return ret;
}

struct iosq_entry
nvme_read(uint command_id,uint *lba, uint num_lb, char *read_prps1, char *read_prps2)
{
  struct iosq_entry ret;
  memset(&ret,0,sizeof(ret));
  ret.opcode = 0x2;
  set_FUSE(0,&ret);
  set_PSDT(0,&ret);
  ret.command_identifier[1] = command_id / 0xff;
  ret.command_identifier[0] = command_id % 0xff;
  ret.namespace_identifier = 1;

  ret.metadata_pointer[0] = 0;
  ret.metadata_pointer[1] = 0;

  ret.data_pointer[0]=V2P((uint)read_prps1);
  ret.data_pointer[2]=V2P((uint)read_prps1);

  rw_command_set_lba(lba,&ret);
  rw_command_set_num_lb(num_lb,&ret);
  return ret;
}

void
nvme_attach(struct pci_func *pcif)
{
    pci_func_nvme_enable(pcif);
    uint baseio = pcif->BAR;

    *(volatile uint*)(baseio+0x14) = 0;//cc register

    while(1){
      volatile uint *rdy = (volatile uint*)(baseio+0x1c);
      if(*rdy%2 == 0){
          cprintf("nvme setting start.. 1 admin queue, 8 IO queue\n");
          break;
      }
    } 

    init_que(&admin_que,0,0);
    admin_que.submission_doorbell = (volatile uint)(baseio + 0x1000);
    admin_que.completion_doorbell = (volatile uint)(baseio + 0x1004);

    for (int i = 0; i < 8; i++){
      init_que(&IO_que[i],0,0);
      IO_que[i].submission_queue = (struct iosq_entry *)kalloc();
      IO_que[i].completion_queue = (struct iocq_entry *)kalloc();
      IO_que[i].submission_doorbell = (volatile uint)(baseio + 0x1000 + 8*(i+1));
      IO_que[i].completion_doorbell = (volatile uint)(baseio + 0x1004 + 8*(i+1));
    }

    aqa_setting(baseio);
    asq_setting(baseio);
    acq_setting(baseio);
    cc_setting(baseio);

    while(1){
    volatile uint *rdy = (volatile uint*)(baseio+0x1c);
      if(*rdy%2 ==1){
          cprintf("nvme ready complelete!\n");
          break;
      }
    }    
    nvme_command_syn(nvme_identify(1,0,1),&admin_que);
    nvme_command_syn(nvme_set_feature(2,0),&admin_que);
    nvme_command_syn(nvme_create_iocq(3,0),&admin_que);
    nvme_command_syn(nvme_create_iosq(4,0),&admin_que);


    return;
};


void
print_cq_entry(struct iocq_entry check)
{
  cprintf("command id %d %d\n",check.command_identifier[1],check.command_identifier[0]);
  cprintf("command specific %d\n", check.command_specific);
  cprintf("sq_head_pointer %d %d\n",check.sq_head_pointer[1], check.sq_head_pointer[0]);
  cprintf("sq_id %d %d\n", check.sq_identifier[1], check.sq_identifier[0]);
  cprintf("status + p %d %d\n",check.status_field_p[1], check.status_field_p[0]);
  cprintf("\n\n");
}

void
sys_nvme_setting(void)
{
  //cprintf("total : %d, hit : %d\n",totalorder, cachehit);
  cprintf("hit rate : %d\n", cachehit * 100 / totalorder);
  totalorder = 0;
  cachehit = 0;
}

void
sys_buddy_testing(void)
{

  printBuddySystem();
  printHash();

  char *A = dmalloc(4096);
  cprintf("A - %x\n", A);
  printBuddySystem();
  printHash();

  char *B = dmalloc(4096 * 2);
  cprintf("B - %x\n", B);
  printBuddySystem();
  printHash();

  char *C = dmalloc(4096 * 3);
  cprintf("C - %x\n", C);
  printBuddySystem();
  printHash();

  char *D = dmalloc(4096 * 8);
  cprintf("D - %x\n", D);
  printBuddySystem();
  printHash();

  char *E = dmalloc(4096);
  cprintf("E - %x\n", E);
  printBuddySystem();
  printHash();

  char *F = dmalloc(4096);
  cprintf("F - %x\n", F);
  printBuddySystem();
  printHash();

  char *G = dmalloc(4097);
  cprintf("G - %x\n", G);
  printBuddySystem();
  printHash();

  cprintf("\nD free\n");
  dfree(D);
  printBuddySystem();
  printHash();

  char *H = dmalloc(4096 * 4);
  cprintf("H - %x\n", H);
  printBuddySystem();
  printHash();

  cprintf("\nE free\n");
  dfree(E);
  printBuddySystem();
  printHash();

  cprintf("\nB free\n");
  dfree(B);
  printBuddySystem();
  printHash();

  cprintf("\nF free\n");
  dfree(F);
  printBuddySystem();
  printHash();

  cprintf("\nH free\n");
  dfree(H);
  printBuddySystem();
  printHash();

  cprintf("\nC free\n");
  dfree(C);
  printBuddySystem();
  printHash();

  cprintf("\nA free\n");
  dfree(A);
  printBuddySystem();
  printHash();

  cprintf("\nG free\n");
  dfree(G);
  printBuddySystem();
  printHash();
}