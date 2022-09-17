#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;
  int block[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding; // how many FS sys calls are executing.
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log log;
struct log nvmelog;

static void recover_from_log(uint dev);
static void commit(uint dev);

void
initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  if (dev == ROOTDEV){
    initlock(&log.lock, "log");
    readsb(dev, &sb);
    log.start = sb.logstart;
    log.size = sb.nlog;
    cprintf("normal dev log.size %d\n",log.size);
    log.dev = dev;
    recover_from_log(dev);
  }
  if (dev == NVMEDEV) {
    initlock(&nvmelog.lock, "nvmelog");
    readsb(dev, &sb);
    nvmelog.start = sb.logstart;
    nvmelog.size = sb.nlog;
    cprintf("nvme dev log.size %d\n",log.size);
    nvmelog.dev = dev;
    recover_from_log(dev);    
  }
}

// Copy committed blocks from log to their home location
static void
install_trans(uint dev)
{

  int tail;

  if(dev == ROOTDEV){
    for (tail = 0; tail < log.lh.n; tail++) {
      struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
      struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
      memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
      bwrite(dbuf);  // write dst to disk
      brelse(lbuf);
      brelse(dbuf);
    }
  }
  if (dev == NVMEDEV){
    for (tail = 0; tail < nvmelog.lh.n; tail++) {
      struct buf *lbuf = bread(nvmelog.dev, nvmelog.start+tail+1); // read log block
      struct buf *dbuf = bread(nvmelog.dev, nvmelog.lh.block[tail]); // read dst
      memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
      bwrite(dbuf);  // write dst to disk
      brelse(lbuf);
      brelse(dbuf);
    }    
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(uint dev)
{
  if (dev == ROOTDEV){
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *lh = (struct logheader *) (buf->data);
    int i;
    log.lh.n = lh->n;
    for (i = 0; i < log.lh.n; i++) {
      log.lh.block[i] = lh->block[i];
    }
    brelse(buf);
  }
  if (dev == NVMEDEV){
    struct buf *buf = bread(nvmelog.dev, nvmelog.start);
    struct logheader *lh = (struct logheader *) (buf->data);
    int i;
    nvmelog.lh.n = lh->n;
    for (i = 0; i < nvmelog.lh.n; i++) {
      nvmelog.lh.block[i] = lh->block[i];
    }
    brelse(buf);    
  }
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(uint dev)
{
  if (dev == ROOTDEV){
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader *) (buf->data);
    int i;
    hb->n = log.lh.n;
    for (i = 0; i < log.lh.n; i++) {
      hb->block[i] = log.lh.block[i];
    }
    bwrite(buf);
    brelse(buf);
  }
  if(dev == NVMEDEV){
    struct buf *buf = bread(nvmelog.dev, nvmelog.start);
    struct logheader *hb = (struct logheader *) (buf->data);
    int i;
    hb->n = nvmelog.lh.n;
    for (i = 0; i < nvmelog.lh.n; i++) {
      hb->block[i] = nvmelog.lh.block[i];
    }
    bwrite(buf);
    brelse(buf);    
  }
}

static void
recover_from_log(uint dev)
{
  read_head(dev);
  install_trans(dev); // if committed, copy from log to disk
  if (dev == ROOTDEV) log.lh.n = 0;
  if (dev == NVMEDEV) nvmelog.lh.n = 0;
  write_head(dev); // clear the log
}

// called at the start of each FS system call.
void
begin_op(uint dev)
{
  if (dev == ROOTDEV){
    acquire(&log.lock);
    while(1){
      if(log.committing){
        sleep(&log, &log.lock);
      } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
        // this op might exhaust log space; wait for commit.
        sleep(&log, &log.lock);
      } else {
        log.outstanding += 1;
        release(&log.lock);
        break;
      }
    }
  }
  if (dev == NVMEDEV){
    acquire(&nvmelog.lock);
    while(1){
      if(nvmelog.committing){
        sleep(&nvmelog, &nvmelog.lock);
      } else if(nvmelog.lh.n + (nvmelog.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
        // this op might exhaust log space; wait for commit.
        sleep(&nvmelog, &nvmelog.lock);
      } else {
        nvmelog.outstanding += 1;
        release(&nvmelog.lock);
        break;
      }
    }    
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(uint dev)
{
  if(dev == ROOTDEV){
    int do_commit = 0;

    acquire(&log.lock);
    log.outstanding -= 1;
    if(log.committing)
      panic("log.committing");
    if(log.outstanding == 0){
      do_commit = 1;
      log.committing = 1;
    } else {
      // begin_op() may be waiting for log space,
      // and decrementing log.outstanding has decreased
      // the amount of reserved space.
      wakeup(&log);
    }
    release(&log.lock);

    if(do_commit){
      // call commit w/o holding locks, since not allowed
      // to sleep with locks.
      commit(dev);
      acquire(&log.lock);
      log.committing = 0;
      wakeup(&log);
      release(&log.lock);
    }
  }
  if(dev == NVMEDEV){
    int do_commit = 0;

    acquire(&nvmelog.lock);
    nvmelog.outstanding -= 1;
    if(nvmelog.committing)
      panic("nvmelog.committing");
    if(nvmelog.outstanding == 0){
      do_commit = 1;
      nvmelog.committing = 1;
    } else {
      // begin_op() may be waiting for log space,
      // and decrementing log.outstanding has decreased
      // the amount of reserved space.
      wakeup(&nvmelog);
    }
    release(&nvmelog.lock);

    if(do_commit){
      // call commit w/o holding locks, since not allowed
      // to sleep with locks.
      commit(dev);
      acquire(&nvmelog.lock);
      nvmelog.committing = 0;
      wakeup(&nvmelog);
      release(&nvmelog.lock);
    }    
  }
}

// Copy modified blocks from cache to log.
static void
write_log(uint dev)
{
  int tail;
  if (dev == ROOTDEV){
    for (tail = 0; tail < log.lh.n; tail++) {
      struct buf *to = bread(log.dev, log.start+tail+1); // log block
      struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
      memmove(to->data, from->data, BSIZE);
      bwrite(to);  // write the log
      brelse(from);
      brelse(to);
    }
  }
  if (dev == NVMEDEV){
    for (tail = 0; tail < nvmelog.lh.n; tail++) {
      struct buf *to = bread(nvmelog.dev, nvmelog.start+tail+1); // log block
      struct buf *from = bread(nvmelog.dev, nvmelog.lh.block[tail]); // cache block
      memmove(to->data, from->data, BSIZE);
      bwrite(to);  // write the log
      brelse(from);
      brelse(to);
    }    
  }
}

static void
commit(uint dev)
{
  if(dev == ROOTDEV){
    if (log.lh.n > 0) {
      write_log(dev);     // Write modified blocks from cache to log
      write_head(dev);    // Write header to disk -- the real commit
      install_trans(dev); // Now install writes to home locations
      log.lh.n = 0;
      write_head(dev);    // Erase the transaction from the log
    }
  }
  if(dev == NVMEDEV){
    if (nvmelog.lh.n > 0) {
      write_log(dev);     // Write modified blocks from cache to log
      write_head(dev);    // Write header to disk -- the real commit
      install_trans(dev); // Now install writes to home locations
      nvmelog.lh.n = 0;
      write_head(dev);    // Erase the transaction from the log
    }    
  }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache with B_DIRTY.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  if(b->dev == ROOTDEV){
    int i;

    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
      panic("too big a transaction");
    if (log.outstanding < 1)
      panic("log_write outside of trans");

    acquire(&log.lock);
    for (i = 0; i < log.lh.n; i++) {
      if (log.lh.block[i] == b->blockno)   // log absorbtion
        break;
    }
    log.lh.block[i] = b->blockno;
    if (i == log.lh.n)
      log.lh.n++;
    b->flags |= B_DIRTY; // prevent eviction
    release(&log.lock);
  }
  if(b->dev == NVMEDEV){
    int i;

    if (nvmelog.lh.n >= LOGSIZE || nvmelog.lh.n >= nvmelog.size - 1) {
      cprintf("nvmelog.lh.n is %d and LOGSIZE %d nvmelog.size %d\n", nvmelog.lh.n, LOGSIZE,nvmelog.size );
      panic("too big a transaction");

    }
    if (nvmelog.outstanding < 1)
      panic("log_write outside of trans");

    acquire(&nvmelog.lock);
    for (i = 0; i < nvmelog.lh.n; i++) {
      if (nvmelog.lh.block[i] == b->blockno)   // log absorbtion
        break;
    }
    nvmelog.lh.block[i] = b->blockno;
    if (i == nvmelog.lh.n)
      nvmelog.lh.n++;
    b->flags |= B_DIRTY; // prevent eviction
    release(&nvmelog.lock);    
  }
}

