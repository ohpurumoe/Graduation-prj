#ifndef PAGECACHE 
#define PAGECACHE
#include "mmu.h"
#include "param.h"
#include "fs.h"
#include "types.h"


#define cacheinvalid -1
#define cachevalid 1



struct cache{
  int reference_time;
  uchar metaidx;
  uchar metapgidx;
  char page[PGSIZE];
  int dirty;
  int valid;
  struct file *f;
  int fd;
};

struct cache CACHE[CACHESIZE];

struct cache_metadata{
  char *name;
  int namehash;
  int valid;
  int close;
  uchar pageidx[(MAXFILE * BSIZE)/PGSIZE + 1];
};

struct cache_metadata CACHE_META[NFILE];





//polynomial rolling hash function

#endif