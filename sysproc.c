#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "buddy.h"
#include "slab.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

extern struct slab_cache slab[SLAB_NUM];

char test_mem[PGSIZE];
char tmp_buf[PGSIZE];

struct{
  char* ptr;
  int n;
} ptr_list[1000];

static int cnt = 0;
static int req_p = 0;
static int req_s = 0;

int
sys_kernel_dmalloc(void)
{
  int n;

  if(argint(0, &n) < 0)
    return -1;

  ptr_list[cnt].ptr = dmalloc(n);
  
  if(n > 2048) req_p += PGSIZE;
  else req_s += slab[calc_slab_idx(n)].obj_sz;

  if(cnt == 0){
    for(int i=0; i<PGSIZE; i++){
      test_mem[i] = i % 0x100;
    }
  }

  memmove(ptr_list[cnt].ptr, test_mem, n);

  if(cnt == 999){
    for(int i=0; i<1000; i++){
      int len = ptr_list[i].n;

      memmove(tmp_buf, ptr_list[i].ptr, len);
      if(!memcmp(tmp_buf, test_mem, len)) dfree((void*)(ptr_list[i].ptr));
      else {
        cprintf("assert : memcmp return non zero value\n");
        return -1;
      }
    }

    cprintf("dfree suceess!\n");
  }

  cnt++;

  return calc_slab_mem() + (req_s + req_p);
}