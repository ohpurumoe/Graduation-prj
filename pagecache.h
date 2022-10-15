#ifndef PAGECACHE 
#define PAGECACHE
#include "mmu.h"
#include "param.h"
#include "fs.h"
#include "types.h"


#define cacheinvalid            -1
#define cachevalid              1
#define check_cache_hash_num    113


struct cache{
  int reference_time;
  uchar metaidx;
  uchar metapgidx;
  char page[PGSIZE];
  int dirty;
  int valid;
  struct file *f;
  int fd;
  int idx;
  int ref;
  int dev;
};

struct cache CACHE[CACHESIZE];

struct cache_metadata{
  char *name;
  int namehash;
  int valid;
  int close;
  int cache_page_num;
  int dev;
  uchar pageidx[(MAXFILE * BSIZE)/PGSIZE + 1];
};

struct cache_metadata CACHE_META[NFILE];

struct check_cache_HASH{
  char *name;
  int dev;
  uchar meta_idx;
  struct check_cache_HASH *next;
};

struct check_cache_HASH check_cache_hash[check_cache_hash_num];

//polynomial rolling hash function

#endif