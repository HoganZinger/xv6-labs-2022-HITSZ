// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13

//hashmap、lock and hash function
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];
} bcache;

int hash(int n){
  return n % NBUCKETS;
}

//auxiliary function
//write cache
void
write_cache(struct buf *get_buf, uint dev, uint blockno){
  get_buf->dev = dev;
  get_buf->blockno = blockno;
  get_buf->valid = 0;
  get_buf->refcnt = 1;
  get_buf->timecnt = ticks;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&(bcache.lock[i]), "bcache_hash");
  }

  // Create linked list of buffers

  bcache.head[0].next = &bcache.buf[0];
  for(b = bcache.buf; b < bcache.buf+NBUF - 1; b++){
    b->next = b + 1;

    initsleeplock(&b->lock, "buffer");
    
  }
  initsleeplock(&b->lock, "buffer");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

//get cache
static struct buf*
bget(uint dev, uint blockno){
  struct buf *b, *last;
  struct buf *get_buf = 0;
  int id = hash(blockno);
  acquire(&(bcache.lock[id]));
  //determine whether the block is already cached
  for(b = bcache.head[id].next, last = &(bcache.head[id]); b; b = b->next, last = last -> next)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->timecnt = ticks;
      b->refcnt++;
      release(&(bcache.lock[id]));
      acquiresleep(&b->lock);
      return b ;

    }
    if(b->refcnt == 0){
      get_buf = b;
    }
  }
  //not cached and has free block
  if(get_buf){
    write_cache(get_buf, dev, blockno);
    release(&(bcache.lock[id]));
    acquiresleep(&(get_buf->lock));
    return get_buf;
  }
  //find the block that is the least used from another pool
  int lock_cnt = -1;
  uint time = __UINT32_MAX__;
  struct buf *tmp;
  struct buf *last_use = 0;
  for(int i = 0; i < NBUCKETS; i++){
    if(i == id) continue;
    acquire(&(bcache.lock[i]));
    for(b = bcache.head[i].next, tmp = &(bcache.head[i]); b; b = b->next, tmp = tmp-> next){
      if(b->refcnt == 0){
        if(b->timecnt < time){
          time = b->timecnt;
          last_use = tmp;
          get_buf = b;
          //last free block not in the pool, release its lock
          if(lock_cnt != -1 && lock_cnt != i &&holding(&(bcache.lock[lock_cnt]))){
            release(&(bcache.lock[lock_cnt]));
          }
          lock_cnt = i;
        
        }
      }
    }
    //not used block ,release lock
    if(lock_cnt != i){
      release(&(bcache.lock[i]));
    }

  }

  if(!get_buf){
    panic("bget: no buffers");
  }

  //take the chosen block out of the other pool
  last_use->next = get_buf->next;
  get_buf->next = 0;
  release(&(bcache.lock[lock_cnt]));
  //put it in the pool needing it
  b = last;
  b->next = get_buf;
  write_cache(get_buf, dev, blockno);
  
  release(&(bcache.lock[id]));
  acquiresleep(&(get_buf->lock));
  
  return get_buf;



  
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
  
}

void
bpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}

