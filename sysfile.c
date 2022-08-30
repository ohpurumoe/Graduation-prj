//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "pagecache.h"
#include "memlayout.h"

struct spinlock cachelock;
struct spinlock readlock[NOFILE];
struct spinlock writelock[NOFILE];

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.


int
sys_get_ticks(void)
{
  acquire(&tickslock);
  int ret= ticks;
  release(&tickslock);
  return ret;
}
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}




int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  //fd add
  for (int i = 0; i < NOFILE; i++){
    if(f->ip == namei(myproc()->NAMEFD[i].name)){
      for (int j = 0; j < NOFILE; j++){
        if(myproc()->NAMEFD[i].fd[j] == 0) {
          myproc()->NAMEFD[i].fd[j] = fd;
          myproc()->NAMEFD[i].fdcnt++; 
          myproc()->NAMEFD[i].f = f;
          i = NOFILE;
          break;
        }
      }
    }
  }

  return fd;
}
///////////////////////////////////////////////////////////////////check lock;;;
int
sys_fileoffset(void)
{
  struct file *f;
  int n;
  if(argfd(0, 0, &f) < 0 || argint(1, &n) < 0)
    return -1;
  if(n > (f->ip->size)) return -1;
  acquire(&cachelock);
  f->off = n;
  release(&cachelock);
  return 0;
}



int 
check_cache(struct file *f, int pgidx)
{
  ///////////////////////need optimization
  for (int i = 0; i < NOFILE; i++){
    if(myproc()->NAMEFD[i].f == f) {
      //cprintf("MYPROC()->NAMEFD\n");
      for (int j = 0; j < NFILE; j++) {
        
        /*cprintf("cache meta idx : %d\n", j );
        cprintf("namefd: %s, meta : %s\n",myproc()->NAMEFD[i].name,CACHE_META[j].name);
        cprintf("namehash compare : %d\n", (myproc()->NAMEFD[i].namehash == CACHE_META[j].namehash));
        cprintf("namehash compare : %d , %d\n", myproc()->NAMEFD[i].namehash , CACHE_META[j].namehash);
        cprintf("name len compare : %d\n",(strlen(myproc()->NAMEFD[i].name) == strlen(CACHE_META[j].name)));
        cprintf("cmp compare:  %d\n\n",(strncmp(myproc()->NAMEFD[i].name,CACHE_META[j].name,strlen(CACHE_META[j].name)) == 0));
        */    
        if(
          (myproc()->NAMEFD[i].namehash == CACHE_META[j].namehash)&&
          (strlen(myproc()->NAMEFD[i].name) == strlen(CACHE_META[j].name))&&
          (strncmp(myproc()->NAMEFD[i].name,CACHE_META[j].name,strlen(CACHE_META[j].name)) == 0)
        )
        {       
          //cprintf("check cache pid : %d, name %s, cachemetaidx %d\n ", myproc()->pid,CACHE_META[j].name,j);     
          cache_meta_idx[myproc()->meta_idx_idx] = j;
          break;
        }
      }
      break;
    }
  }

  //cprintf("cache_meta_idx : %d\n",cache_meta_idx);
  //cprintf("pgidx %x %d\n",CACHE_META[cache_meta_idx].pageidx[pgidx] ,pgidx );
  if((cache_meta_idx[myproc()->meta_idx_idx]!= 0xff )&&CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[pgidx]!=0xff) {

    //cprintf("check cache valid %d \n",CACHE[CACHE_META[cache_meta_idx].pageidx[1]].valid);

    return cachevalid;
  }
  else {
    return cacheinvalid;
  }
}
////////////////////////////
void
write_direct(struct file *f,int fd)
{
  int namehash = -1;
  char *name = 0;
  for (int i = 0; i < NOFILE; i++){
    for (int j = 0; j < NOFILE; j++){
     // cprintf("NAMEFD idx %d FD idx %d fd val of NAMEFD %d, input fd %d\n", i,j,myproc()->NAMEFD[i].fd[j],fd);

      if(myproc()->NAMEFD[i].fd[j] == fd) {
        name = myproc()->NAMEFD[i].name;
        namehash = myproc()->NAMEFD[i].namehash;//////////////////////////
        i = NOFILE;
        break;
      }
    }
  }
  //need optimization
  for (int i = 0; i < NFILE; i++) {
    if((namehash == CACHE_META[i].namehash) && (strlen(name) == strlen(CACHE_META[i].name)) && (strncmp(name,CACHE_META[i].name,strlen(name))==0)){
      for(int j = 0; j < (MAXFILE * BSIZE)/PGSIZE + 1; j++){
        if(j < 3){
         // cprintf("NAME %s, i-idx %d ,pgidx %d, dirty %d, valid %d\n",name,i,CACHE_META[i].pageidx[j], CACHE[CACHE_META[i].pageidx[j]].dirty,CACHE[CACHE_META[i].pageidx[j]].valid);
        }
        if(CACHE_META[i].pageidx[j] != 0xff && CACHE[CACHE_META[i].pageidx[j]].dirty == cachevalid && CACHE[CACHE_META[i].pageidx[j]].valid ==1) {
 
          PageCacheFileWrite(f,CACHE[CACHE_META[i].pageidx[j]].page,PGSIZE,j*PGSIZE);
          acquire(&cachelock);
          CACHE[CACHE_META[i].pageidx[j]].dirty = cacheinvalid;
          release(&cachelock);
        }
      }
      //acquire(&cachelock);
      //CACHE_META[i].close = cachevalid;
      //release(&cachelock);
      break;
    }
  }
}

int
cache_fault_handler(struct file *f, int off, int n, int fd, int rwmode, int gotoidx)
{
  //cprintf("fault handler!! %d\n",off);
  if(cachedpg_num == CACHESIZE){
    cachedpg_num--;
    release(&cachelock);
    lru_policy();
    acquire(&cachelock);
  }
  

  for (int i = 0; i < CACHESIZE; i++) {
    if(CACHE[i].valid == cacheinvalid) {
      //cprintf("AAAAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCCCCC,   %d\n", myproc()->pid);
      CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE] = i;
      break;
    }
  }
  //cprintf("HANDLER PID1 %d cache_meta_idx %d cache idx %d\n", myproc()->pid,cache_meta_idx[myproc()->meta_idx_idx],CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]);
  //cprintf("off %d rwmode %d gotoidx %d\n",off,rwmode,gotoidx);
  
  //cprintf("MYPROC meta idx idx %d\n\n", myproc()->meta_idx_idx);
  char *q = kalloc();

  //f->off -= off%PGSIZE;
  release(&cachelock);
  //fileread(f, q, PGSIZE);
  pageCacheFileRead(f,q,PGSIZE,off- (off)%PGSIZE);
  acquire(&cachelock);

  if(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].valid == cachevalid) {
    CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]=0xff;
    //cprintf("AAAAAAAAAAAAAAAAAAAAAA\n");
    //cprintf("pid %d, off %d, cache idx %d rwmode %d gotoidx %d\n", myproc()->pid,off,CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE],rwmode,gotoidx);
    kfree(q);
    if(rwmode == 0){
      if(gotoidx == 1) return 1;
      if(gotoidx == 2) return 2;
      if(gotoidx == 3) return 3;
    }
    else if(rwmode == 1){
      if(gotoidx == 1) return 4;
      if(gotoidx == 2) return 5;
      if(gotoidx == 3) return 6;    
    }
    
  }
  
  //cprintf("HANDLER, pid %d offset %d\n",myproc()->pid, off);

  //for (int i = 0; i < 20 ; i++) cprintf("%d ", q[i]);
  //cprintf("\n");
  //cprintf("HANDLER PID2 %d cache_meta_idx %d cache idx %d\n", myproc()->pid,cache_meta_idx[myproc()->meta_idx_idx],CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]);
  //cprintf("off %d rwmode %d gotoidx %d\n",off,rwmode,gotoidx);
  //cprintf("MYPROC meta idx idx %d\n\n", myproc()->meta_idx_idx);
  
  /*for(int i = 0; i < 10; i++) cprintf("%d ", q[i]);
  cprintf("\n\n");
  cprintf("cache idx %d\n",CACHE_META[cache_meta_idx].pageidx[off/PGSIZE]);
  */

  memmove(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].page,q,PGSIZE);
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].valid = cachevalid;
  cachedpg_num++;
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].metaidx = cache_meta_idx[myproc()->meta_idx_idx];
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].metapgidx = off/PGSIZE;
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].dirty = cacheinvalid;
  acquire(&tickslock);
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].reference_time = ticks;
  //cprintf("cache idx %d , TIME %d\n", CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE],CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].reference_time );
  release(&tickslock);
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].f = f;
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].fd = fd;
  kfree(q);
  return 0;
}
//wrong return value;

int
sys_caching_read(void)
{ 
  
  struct file *f;
  int n,fd;
  char *p;
  char *cur;
  int totalread = 0;
  int tmpfoff;
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) || argint(3,&tmpfoff) < 0)
    return -1;

  //cprintf("CACHING READ pid %d , tmpfoff %d\n",myproc()->pid, tmpfoff);
  int startreadpgsize = (tmpfoff)/PGSIZE;
  int endreadpgsize = (tmpfoff+n-1)/PGSIZE;   

  cur = p;
  for (int i = startreadpgsize; i <= endreadpgsize; i++){
    if(i==startreadpgsize) {
      
FIRSTREAD:
      acquire(&cachelock);

      

      if(check_cache(f,i) != 1){
        if(cache_fault_handler(f,tmpfoff,n,fd, 0,1) == 1) {
          release(&cachelock);
          goto FIRSTREAD;
        }
      }

      if(tmpfoff/PGSIZE != (n+tmpfoff-1)/PGSIZE){

        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTREAD;
        }

        //cprintf("ADDR cur %x p %x\n",cur,p);


        memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),(PGSIZE-(tmpfoff%PGSIZE)));
        cur+=(PGSIZE-(tmpfoff%PGSIZE));   
        totalread += (PGSIZE-(tmpfoff%PGSIZE));
        tmpfoff += (PGSIZE-(tmpfoff%PGSIZE)); 
        release(&cachelock);
      }
      else {

        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTREAD;
        }
        //cprintf("cache meta idx %d , pgidx %d, idx %d n %d\n", cache_meta_idx, tmpfoff/PGSIZE, tmpfoff%PGSIZE,n);
        //cprintf("READ start idx PID %d, readsize %d off %d\n", myproc()->pid,n,tmpfoff%PGSIZE);
        //for (int i = 0; i < 10;  i++) cprintf("%d ",CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[i] );


        memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),n);
        
        //cprintf("READ end idx PID %d\n", myproc()->pid);
        //for (int i = 0; i < 10;  i++) cprintf("%d ",cur[i] );


        cur+=n; 
        tmpfoff += n;
        totalread += n;
        release(&cachelock);
      }  
    }
    else if(i == (endreadpgsize)) {

SECONDREAD:
      acquire(&cachelock);
      if(check_cache(f,i) != 1){
        if(cache_fault_handler(f,tmpfoff,n,fd,0,2)==2) {
          release(&cachelock);
          goto SECONDREAD;
        }
      }
      if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
        release(&cachelock);
        goto SECONDREAD;
      }
      memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[0]),n-((uint)cur-(uint)p));
      
 //cprintf("\nADDR cur %x p %x\n",cur,p);
        //cprintf("end idx PID %d\n", myproc()->pid);
        //for (int i = 0; i < 10;  i++) cprintf("%d ",CACHE[CACHE_META[cache_meta_idx].pageidx[tmpfoff/PGSIZE]].page[i] );
        //cprintf("\n");

        //cprintf("\nP\n");
        //cprintf("end idx PID %d\n", myproc()->pid);
        //for (int i = 0; i < 10;  i++) cprintf("%d ",p[i] );
        //cprintf("\n");      

      char *curtmp = cur;
      cur+=n-((uint)cur-(uint)p);
      tmpfoff += n-((uint)curtmp-(uint)p);
      totalread +=n-((uint)curtmp-(uint)p);
      f->off = tmpfoff;
      release(&cachelock);
    }
    else {

THIRDREAD:
      acquire(&cachelock);
      if(check_cache(f,i)!=1){
        if(cache_fault_handler(f,tmpfoff,n,fd,0,3)==3) {
          release(&cachelock);
          goto THIRDREAD;
        }
      }
      if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
        release(&cachelock);
        goto THIRDREAD;
      }
      memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[0]),(PGSIZE));
      
      cur+=PGSIZE;
      tmpfoff += PGSIZE;
      totalread += PGSIZE;
      release(&cachelock);
    } 
  }
  acquire(&cachelock);
  
  
  /*cprintf("CACHE VALUE pid %d\n",myproc()->pid);
  for (int i = 0; i < 2; i++){
    cprintf("pid %d, idx %d, metaidx %d, metapgidx %d dirty %d, valid %d fd %d\n",
    myproc()->pid,
    i,CACHE[i].metaidx,
    CACHE[i].metapgidx, 
    CACHE[i].dirty, 
    CACHE[i].valid,
    CACHE[i].fd);
    for (int j = 4024; j < 4024 + 20; j++){
      cprintf("%d ", CACHE[i].page[j]);
    }
    cprintf("\n");
  }  
  
  cprintf("CACHE META VALUE\n");
  for (int i = 0; i < 3; i++){
    cprintf("%s : pgidx %d, %d ,%d\n",CACHE_META[i].name, CACHE_META[i].pageidx[0], CACHE_META[i].pageidx[1],CACHE_META[i].pageidx[2]);
  }
  cprintf("------------------------------------------------------------------------------\n");*/
  release(&cachelock);
  return totalread;
}
int
sys_read(void)
{
  struct file *f;
  int n,fd;
  char *p;
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f,p,n); 
}

int
sys_caching_write(void)
{
  //acquire(&writelock);
  struct file *f;
  int n, fd;
  char *p;
  char *cur;
  int totalwrite = 0, tmpfoff;
  
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) || argint(3,&tmpfoff) < 0){
    //release(&writelock);
    return -1;
  }

  //cprintf("TMPOFF %d\n", tmpfoff);
  int startwritepgsize = (tmpfoff)/PGSIZE;
  int endwritepgsize = (tmpfoff+n-1)/PGSIZE;
  cur = p;
  for (int i = startwritepgsize; i <= endwritepgsize; i++){
    if(i==startwritepgsize) {

FIRSTWRITE:
      acquire(&cachelock);
     // cprintf("F         I          %x %x\n",f,i);
      if(check_cache(f,i)!=1){
        if(cache_fault_handler(f,tmpfoff,n,fd,1,1)==4) {
          release(&cachelock);
          goto FIRSTWRITE;
        }
      }
      if(tmpfoff/PGSIZE != (n+tmpfoff-1)/PGSIZE){
        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTWRITE;
        }        

        //cprintf("TMPFOFF                           %d\n",tmpfoff);


        memmove(&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),cur,(PGSIZE-(tmpfoff%PGSIZE)));
        CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].dirty = cachevalid;
        cur+=(PGSIZE-(tmpfoff%PGSIZE)); 
        totalwrite += (PGSIZE-(tmpfoff%PGSIZE));  
        tmpfoff += (PGSIZE-(tmpfoff%PGSIZE));
        release(&cachelock);

      }
      else {

        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTWRITE;
        }
    

        //cprintf("pid : %d\n",myproc()->pid);
        //cprintf("TMPOFF %d\n",tmpfoff);
        //for (int i = 0; i < 6; i++) cprintf("%d ",cur[i]);
        //cprintf("\n-------------------------\n");
        memmove(&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),cur,n);

        //cprintf("PID : %d\n",myproc()->pid);
        //for (int i = 0; i < 10; i++) cprintf("%d ",CACHE[CACHE_META[cache_meta_idx].pageidx[tmpfoff/PGSIZE]].page[i]);
        //cprintf("\n-------------------------\n");


        CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].dirty = cachevalid;
        cur+=n; 
        tmpfoff += n;
        totalwrite += n;
        release(&cachelock);
      }  
    }

    else if(i == (endwritepgsize)) {

SECONDWRITE:
      acquire(&cachelock);
      if(check_cache(f,i)!=1){
        if(cache_fault_handler(f,tmpfoff,n,fd,1,2)==5) {
          release(&cachelock);
          goto SECONDWRITE;
        }
      }
      if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
        release(&cachelock);
        goto SECONDWRITE;
      }

      memmove(&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[0]),cur,n-((uint)cur-(uint)p));
      CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].dirty = cachevalid;  
      char *curtmp = cur;
      cur+=n-((uint)cur-(uint)p);
      tmpfoff += n-((uint)curtmp-(uint)p);
      totalwrite +=n-((uint)curtmp-(uint)p);
      f->off = tmpfoff;
      release(&cachelock);
    }

    else {
THIRDWRITE:
      acquire(&cachelock);
      if(check_cache(f,i)!=1){
        if(cache_fault_handler(f,tmpfoff,n,fd,1,3)==6) {
          release(&cachelock);
          goto THIRDWRITE;
        }
      }
      if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
        release(&cachelock);
        goto THIRDWRITE;
      }
      memmove(&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[0]),cur,(PGSIZE));
      CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].dirty = cachevalid;
      cur+=PGSIZE;
      tmpfoff += PGSIZE;
      totalwrite += PGSIZE;
      release(&cachelock);
    } 
  }
  acquire(&cachelock);
  
  /*cprintf("CACHE VALUE pid %d\n",myproc()->pid);
  for (int i = 0; i < 2; i++){
    cprintf("pid %d, idx %d, metaidx %d, metapgidx %d dirty %d, valid %d fd %d\n",myproc()->pid,i,CACHE[i].metaidx,CACHE[i].metapgidx, CACHE[i].dirty, CACHE[i].valid,CACHE[i].fd);
    for (int j = 4024; j < 4024 + 20; j++){
      cprintf("%d ", CACHE[i].page[j]);
    }
    cprintf("\n");
  }  
  
  cprintf("CACHE META VALUE\n");
  for (int i = 0; i < 3; i++){
    cprintf("%s : pgidx %d, %d ,%d\n",CACHE_META[i].name, CACHE_META[i].pageidx[0], CACHE_META[i].pageidx[1],CACHE_META[i].pageidx[2]);
  }
  cprintf("------------------------------------------------------------------------------\n");*/
  release(&cachelock);
  return totalwrite;
}


int
sys_write(void)
{
  struct file *f;
  int n, fd;
  char *p;
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  
  return filewrite(f,p,n);
}

int
sys_caching_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  write_direct(f,fd);
  myproc()->ofile[fd] = 0;
  fileclose(f);
  close_namefd(fd);
  return 0;
}
int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}
int
sys_caching_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if(already_exist_namefd(f,path,fd)== -1) {
    set_namefd(f,path,fd);
  }


  return fd;
}
int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}
