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
#include "buddy.h"
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
check_cache_hash_check(int idx)
{
  struct check_cache_HASH *tmp;
  int namehash = myproc()->NAMEFD[idx].namehash;
  char *name = myproc()->NAMEFD[idx].name;
  for (tmp = &check_cache_hash[namehash % check_cache_hash_num] ; ; tmp = tmp->next){
    if(strlen(name) == strlen(tmp->name) && (strncmp(name,tmp->name,strlen(name)) == 0)){
      cache_meta_idx[myproc()->meta_idx_idx] = tmp->meta_idx;
      return 1;
    }  
    if(tmp->next == 0) return -1;
  }
  return -1;
}

void
add_check_cache_hash(int idx, int meta_idx)
{
  struct check_cache_HASH *tmp;
  int namehash = myproc()->NAMEFD[idx].namehash;
  for (tmp = &check_cache_hash[namehash % check_cache_hash_num] ; ; tmp = tmp->next){
    if(tmp->next == 0){
      tmp->next = (struct check_cache_HASH *)dmalloc(sizeof(struct check_cache_HASH));
      tmp->next->name = myproc()->NAMEFD[idx].name;
      tmp->next->meta_idx = meta_idx;
      tmp->next->next = 0;
      return;
    }
  }
}

int 
check_cache(struct file *f, int pgidx)
{
  ///////////////////////need optimization
  for (int i = 0; i < NOFILE; i++){
    if(myproc()->NAMEFD[i].f == f) {

      //hash finds
      //hash miss
      //if(check_cache_hash_check(i) == -1){
        for (int j = 0; j < NFILE; j++) {           
          if(
            (myproc()->NAMEFD[i].namehash == CACHE_META[j].namehash)&&
            (strlen(myproc()->NAMEFD[i].name) == strlen(CACHE_META[j].name))&&
            (strncmp(myproc()->NAMEFD[i].name,CACHE_META[j].name,strlen(CACHE_META[j].name)) == 0)
          )
          {
            //add member in hash table
            add_check_cache_hash(i,j);
            cache_meta_idx[myproc()->meta_idx_idx] = j;
            break;
          }
        }
      //}
      break;
    }
  }

  if((cache_meta_idx[myproc()->meta_idx_idx]!= 0xff )&&CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[pgidx]!=0xff) {
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
      if(myproc()->NAMEFD[i].fd[j] == fd) {
        name = myproc()->NAMEFD[i].name;
        namehash = myproc()->NAMEFD[i].namehash;
        i = NOFILE;
        break;
      }
    }
  }
  cprintf("name is .. ");
  for (int i = 0; i < 8;i++){
    cprintf("%d ",name[i]);
  }
  cprintf("\n");
  cprintf("hash %d\n",namehash);
    //need optimization
  for (int i = 0; i < NFILE; i++) {
    if((namehash == CACHE_META[i].namehash) && (strlen(name) == strlen(CACHE_META[i].name)) && (strncmp(name,CACHE_META[i].name,strlen(name))==0)){
      for(int j = 0; j < (MAXFILE * BSIZE)/PGSIZE + 1; j++){
        if(CACHE_META[i].pageidx[j] != 0xff && CACHE[CACHE_META[i].pageidx[j]].dirty == cachevalid && CACHE[CACHE_META[i].pageidx[j]].valid ==1) {
          cprintf("%x %d \n", CACHE[CACHE_META[i].pageidx[j]].page, CACHE_META[i].pageidx[j]);
          PageCacheFileWrite(f,CACHE[CACHE_META[i].pageidx[j]].page,PGSIZE,j*PGSIZE);
          acquire(&cachelock);
          CACHE[CACHE_META[i].pageidx[j]].dirty = cacheinvalid;
          release(&cachelock);
        }
      }
      break;
    }
  }
}

int
cache_fault_handler(struct file *f, int off, int n, int fd, int rwmode, int gotoidx)
{
  cprintf("HELLO??\n");
  if(cachedpg_num == CACHESIZE){
    cachedpg_num--;
    release(&cachelock);
    cprintf("victim??\n");;
    lru_policy();
    cprintf("YES\n");
    acquire(&cachelock);
  }
  
  cprintf("HELLO?????????????????\n");
  for (int i = 0; i < CACHESIZE; i++) {
    if(CACHE[i].valid == cacheinvalid) {
      CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE] = i;
      break;
    }
  }
  char *q = kalloc();

  release(&cachelock);
  pageCacheFileRead(f,q,PGSIZE,off- (off)%PGSIZE);
  acquire(&cachelock);

  if(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].valid == cachevalid) {
    CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]=0xff;
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
  
  memmove(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].page,q,PGSIZE);
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].valid = cachevalid;
        CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].cache_page_num++;
  cachedpg_num++;
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].metaidx = cache_meta_idx[myproc()->meta_idx_idx];
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].metapgidx = off/PGSIZE;
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].dirty = cacheinvalid;
  acquire(&tickslock);
  CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[off/PGSIZE]].reference_time = ticks;
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

  int startreadpgsize = (tmpfoff)/PGSIZE;
  int endreadpgsize = (tmpfoff+n-1)/PGSIZE;   

  cur = p;
  for (int i = startreadpgsize; i <= endreadpgsize; i++){
    cprintf(" i is ... %d %d\n", i,endreadpgsize );
    if(i==startreadpgsize) {
      
FIRSTREAD:
      acquire(&cachelock);

      

      if(check_cache(f,i) != 1){
        cprintf("check out!!!!!!!!!!!\n");
        if(cache_fault_handler(f,tmpfoff,n,fd, 0,1) == 1) {
          cprintf("infinite??\n");
          release(&cachelock);
          goto FIRSTREAD;
        }
      }

      if(tmpfoff/PGSIZE != (n+tmpfoff-1)/PGSIZE){
        cprintf("IFFFFF\n");
        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTREAD;
        }
        memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),(PGSIZE-(tmpfoff%PGSIZE)));
        cur+=(PGSIZE-(tmpfoff%PGSIZE));   
        totalread += (PGSIZE-(tmpfoff%PGSIZE));
        tmpfoff += (PGSIZE-(tmpfoff%PGSIZE)); 
        release(&cachelock);
      }
      else {
        cprintf("ELSEEE\n");
        if( (CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].valid) != cachevalid) {
          release(&cachelock);
          goto FIRSTREAD;
        }

        memmove(cur,&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),n);
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
  struct file *f;
  int n, fd;
  char *p;
  char *cur;
  int totalwrite = 0, tmpfoff;
  
  if(argfd(0, &fd, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) || argint(3,&tmpfoff) < 0){
    return -1;
  }

  int startwritepgsize = (tmpfoff)/PGSIZE;
  int endwritepgsize = (tmpfoff+n-1)/PGSIZE;
  cur = p;
  for (int i = startwritepgsize; i <= endwritepgsize; i++){
    if(i==startwritepgsize) {

FIRSTWRITE:
      acquire(&cachelock);
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

        memmove(&(CACHE[CACHE_META[cache_meta_idx[myproc()->meta_idx_idx]].pageidx[tmpfoff/PGSIZE]].page[tmpfoff%PGSIZE]),cur,n);

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

  if(argfd(0, &fd, &f) < 0){
    cprintf("HERERERERERER\n");
    return -1;
  }
  cprintf("f %x , fd %d\n", f, fd);
  write_direct(f,fd);
  cprintf("close fd is %d\n",fd);
  myproc()->ofile[fd] = 0;
  cprintf("1\n");
  fileclose(f);
  cprintf("2\n");
  close_namefd(fd);
  cprintf("3\n");
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

  begin_op(myproc()->cwd->dev);
  if((ip = namei(old)) == 0){
    end_op(myproc()->cwd->dev);
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op(myproc()->cwd->dev);
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

  end_op(myproc()->cwd->dev);

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op(myproc()->cwd->dev);
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

  begin_op(myproc()->cwd->dev);
  if((dp = nameiparent(path, name)) == 0){
    end_op(myproc()->cwd->dev);
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

  end_op(myproc()->cwd->dev);

  return 0;

bad:
  iunlockput(dp);
  end_op(myproc()->cwd->dev);
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

  begin_op(myproc()->cwd->dev);

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op(myproc()->cwd->dev);
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op(myproc()->cwd->dev);
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op(myproc()->cwd->dev);
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op(myproc()->cwd->dev);
    return -1;
  }
  iunlock(ip);
  end_op(myproc()->cwd->dev);

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

  begin_op(myproc()->cwd->dev);

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op(myproc()->cwd->dev);
      return -1;
    }
  } 
  else {
    cprintf("open nvme readme??\n");
    if((ip = namei(path)) == 0){
      cprintf("right before end op in open syscall\n");
      end_op(myproc()->cwd->dev);
      return -1;
    }
    cprintf("open nvme readme ip complete??\n");
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op(myproc()->cwd->dev);
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op(myproc()->cwd->dev);
    return -1;
  }
  iunlock(ip);
  end_op(myproc()->cwd->dev);

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

  begin_op(myproc()->cwd->dev);
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op(myproc()->cwd->dev);
    return -1;
  }
  iunlockput(ip);
  end_op(myproc()->cwd->dev);
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op(myproc()->cwd->dev);
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op(myproc()->cwd->dev);
    return -1;
  }
  iunlockput(ip);
  end_op(myproc()->cwd->dev);
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op(myproc()->cwd->dev);
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op(myproc()->cwd->dev);
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op(myproc()->cwd->dev);
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op(myproc()->cwd->dev);
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

int
sys_cd(void)
{
  char *path;
  if (argstr(0,&path) < 0 ){
    return -1;
  }
  cprintf("before cd cwd %x\n", myproc()->cwd);
  myproc()->cwd = namei(path);
  cprintf("after cd cwd %x\n", myproc()->cwd);
  cprintf("END CD\n");
  return 0;
}