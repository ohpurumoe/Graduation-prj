//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "mmu.h"
#include "pagecache.h"
#include "proc.h"
#include "buddy.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

int cachedpg_num;
int cache_meta_idx[70];

struct spinlock filewritelock;


int 
hash(char *str)
{
  int len = strlen(str);
  int ret = 0;
  int maxhash = 1410918871;
  int prime = 73;

  for (int i = 0; i < len; i++){
    if (i == 0 ) ret = (ret + (str[i] - 'a' + 1) * 1 ) % maxhash;
    else {
      ret = (ret + (str[i] - 'a' + 1) * (prime % maxhash) ) % maxhash;
      prime = prime * 73;
    }
  }
  return ret;
}



void
setfds(struct file *f, int i, int fd)
{
  for (int j = 0; j < NOFILE; j++){
    if(myproc()->NAMEFD[i].fd[j] == 0){
      myproc()->NAMEFD[i].fd[j] = fd;
      myproc()->NAMEFD[i].fdcnt++; 
      myproc()->NAMEFD[i].f = f;
      break;
    }
  }
}

int
already_exist_namefd(struct file *f, char *name, int fd)
{ 
  int namehash = hash(name);
  ///////////////////////need optimization
  for (int i = 0; i < NOFILE; i++){
    if((namehash == myproc()->NAMEFD[i].namehash) && 
    (strlen(name) == strlen(myproc()->NAMEFD[i].name)) && 
    (strncmp(name,myproc()->NAMEFD[i].name,strlen(name)) == 0)){
      myproc()->NAMEFD[i].valid = 1;
      setfds(f,i,fd);
      return cachevalid;
    }
  }
  return cacheinvalid;
}

//fd + name
void
set_namefd(struct file *f, char *name, int fd)
{
  acquire(&cachelock);
  for (int i = 0; i < NOFILE; i++) {
    if(myproc()->NAMEFD[i].valid == cacheinvalid) {///////////////////////////////should fix it
      myproc()->NAMEFD[i].name = name;
      myproc()->NAMEFD[i].namehash = hash(name);
      setfds(f,i,fd);
      myproc()->NAMEFD[i].valid = cachevalid;

      for (int j = 0; j < NFILE; j++){
        if(CACHE_META[j].valid == cachevalid){
          if((myproc()->NAMEFD[i].namehash == CACHE_META[j].namehash) &&
          strlen(myproc()->NAMEFD[i].name) == strlen(CACHE_META[j].name) &&
          (strncmp(myproc()->NAMEFD[i].name,CACHE_META[j].name,strlen(name))== 0)){
            release(&cachelock);
            return;
          }
        }
      }

      int insertidx = -1;
      for (int j = 0; j < NFILE; j++){
        if((insertidx == -1) &&(CACHE_META[j].valid==cacheinvalid)){
          insertidx = j;
          break;
        }
      }


      if(insertidx == -1){
        cprintf("oops!\n");
      }
      else {
        CACHE_META[insertidx].name = name;
        CACHE_META[insertidx].namehash = myproc()->NAMEFD[i].namehash;
        CACHE_META[insertidx].valid=cachevalid;
      }
      break;
    }  
  }

  release(&cachelock);
}
void
close_namefd(int fd)
{
  ///////////////////////need optimization
  for (int i = 0; i < NOFILE; i++) {
    for (int j = 0; j < NOFILE; j++){
      if(myproc()->NAMEFD[i].fd[j]==fd) {
      //valid check
        myproc()->NAMEFD[i].fd[j] = cacheinvalid;
        myproc()->NAMEFD[i].fdcnt--;
        if(myproc()->NAMEFD[i].fdcnt == 0)
          myproc()->NAMEFD[i].valid = cacheinvalid;
        return;
      }
    }
  }
}
//name hash



//collision


//initialize cache meta and hash
void
init_cachemeta(void)
{
  ///////////////////////need optimization
  for (int i = 0; i < NFILE; i++){
    for (int j = 0; j < (MAXFILE * BSIZE)/PGSIZE + 1; j++){
      CACHE_META[i].pageidx[j]=0xff;
    }
    CACHE_META[i].valid = cacheinvalid;
    CACHE_META[i].close = cacheinvalid;
    CACHE_META[i].cache_page_num = 0;
  }

  for (int i = 0; i < check_cache_hash_num; i++){
    check_cache_hash[i].name = 0;
    check_cache_hash[i].meta_idx = 0xff;
    check_cache_hash[i].next = 0;
  }

}
//initialize cache
void
init_cache(void)
{
  for (int i = 0; i < CACHESIZE; i++){
    CACHE[i].reference_time = 0X7FFFFFFF;
    memset(CACHE[i].page,0,PGSIZE);
    CACHE[i].dirty = cacheinvalid;
    CACHE[i].valid = cacheinvalid;
    CACHE[i].metaidx = 0xff;
    CACHE[i].metapgidx = 0xff;
    CACHE[i].idx = 0x7fffffff;
  }
  
}

struct file*
getf(int metaidx)
{
  for (int i = 0; i < NOFILE; i++){
    if((myproc()->NAMEFD[i].namehash == CACHE_META[metaidx].namehash) &&
    (strlen(myproc()->NAMEFD[i].name) == strlen(CACHE_META[metaidx].name))&&
    (strncmp(myproc()->NAMEFD[i].name,CACHE_META[metaidx].name,strlen(myproc()->NAMEFD[i].name))==0)
    ){
      return myproc()->NAMEFD[i].f;
    }
  }
  return (struct file*)cacheinvalid;
}




void
clear_victim_cache(int victim_idx)
{
  CACHE[victim_idx].reference_time = 0X7FFFFFFF;
  CACHE[victim_idx].idx=0x7fffffff;
  memset(CACHE[victim_idx].page,0,PGSIZE);
  CACHE[victim_idx].dirty = cacheinvalid;
  CACHE[victim_idx].valid = cacheinvalid;
  CACHE[victim_idx].metaidx = 0xff;
  CACHE[victim_idx].metapgidx = 0xff;    
}

void
delete_hash(int idx)
{
  struct check_cache_HASH *tmp;
  int namehash = CACHE_META[CACHE[idx].metaidx].namehash;
  char *name = CACHE_META[CACHE[idx].metaidx].name;
  if (check_cache_hash[namehash % check_cache_hash_num].next == 0) return;

  for (tmp = &check_cache_hash[namehash % check_cache_hash_num]; ; tmp = tmp->next){
    if (tmp->next->next == 0){
      if(strlen(name) == strlen(tmp->next->name) && (strncmp(name,tmp->next->name,strlen(name)) == 0)){
        dfree(tmp->next);
        tmp->next = 0;
        return;
      } 
    }
    else {
      if(strlen(name) == strlen(tmp->name) && (strncmp(name,tmp->name,strlen(name)) == 0)){
        struct check_cache_HASH *tmp2 = tmp->next->next;
        dfree(tmp->next);
        tmp->next = tmp2;
        return;
      } 
    }
  }
}

//choose victim
void 
lru_policy(void)
{

  int victim_idx = -1;
  int victim_time = 0x7FFFFFFF;
  
  for(int i = 0; i < CACHESIZE; i++){
    if( CACHE[i].reference_time < victim_time ){
      victim_time = CACHE[i].reference_time;
      victim_idx = i;
    }
  }
  write_direct(CACHE[victim_idx].f,CACHE[victim_idx].fd);
  acquire(&cachelock);
  
  CACHE_META[CACHE[victim_idx].metaidx].pageidx[CACHE[victim_idx].metapgidx] = 0xff;
  CACHE_META[CACHE[victim_idx].metaidx].cache_page_num--;
  if(CACHE_META[CACHE[victim_idx].metaidx].cache_page_num == 0) {
    delete_hash(victim_idx);
  }

  clear_victim_cache(victim_idx);
  release(&cachelock);  
}


void
random_policy(void)
{
  int victim_idx = ticks % CACHESIZE;
  write_direct(CACHE[victim_idx].f,CACHE[victim_idx].fd);
  acquire(&cachelock);
  
  CACHE_META[CACHE[victim_idx].metaidx].pageidx[CACHE[victim_idx].metapgidx] = 0xff;
  CACHE_META[CACHE[victim_idx].metaidx].cache_page_num--;
  if(CACHE_META[CACHE[victim_idx].metaidx].cache_page_num == 0) {
    delete_hash(victim_idx);
  }

  clear_victim_cache(victim_idx);
  release(&cachelock); 
}

void
FIFO_policy(void)
{
  int victim_idx = -1;
  int victim_time = 0x7FFFFFFF;
  
  for(int i = 0; i < CACHESIZE; i++){
    if( CACHE[i].idx < victim_time ){
      victim_time = CACHE[i].reference_time;
      victim_idx = i;
    }
  }
  write_direct(CACHE[victim_idx].f,CACHE[victim_idx].fd);
  acquire(&cachelock);
  
  CACHE_META[CACHE[victim_idx].metaidx].pageidx[CACHE[victim_idx].metapgidx] = 0xff;
  CACHE_META[CACHE[victim_idx].metaidx].cache_page_num--;
  if(CACHE_META[CACHE[victim_idx].metaidx].cache_page_num == 0) {
    delete_hash(victim_idx);
  }

  clear_victim_cache(victim_idx);
  release(&cachelock); 

}
//struct file *f,int fd


void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_op(ff.ip->dev);
    iput(ff.ip);
    end_op(ff.ip->dev);
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}
int
pageCacheFileRead(struct file *f, char *addr, int n, int off)
{
  int r;
  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);

  if(f->type == FD_INODE){
    ilock(f->ip);
    //f->off = off;
    if((r = readi(f->ip, addr, off, n)) > 0)
      //f->off += r;

    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}
// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;
  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);

  if(f->type == FD_INODE){
    ilock(f->ip);
    //cprintf("here???\n");
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    //cprintf("here too\n");
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}
int
PageCacheFileWrite(struct file *f, char *addr, int n, int off)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE){

    //acquire(&filewritelock);
    int ret = pipewrite(f->pipe, addr, n);
    //release(&filewritelock);
    return ret;

  }
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    //acquire(&filewritelock);
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op(f->ip->dev);
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, off, n1)) > 0)
        off += r;
      iunlock(f->ip);
      end_op(f->ip->dev);

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    //release(&filewritelock);
    return i == n ? n : -1;
  }
  panic("filewrite");
}
//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE){

    //acquire(&filewritelock);
    int ret = pipewrite(f->pipe, addr, n);
    //release(&filewritelock);
    return ret;

  }
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    //acquire(&filewritelock);
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op(f->ip->dev);
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op(f->ip->dev);

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    //release(&filewritelock);
    return i == n ? n : -1;
  }
  panic("filewrite");
}

