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

#define NBUC 13

struct {
  struct spinlock lock;
  struct buf head;
} bcaches[NBUC];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

extern uint ticks;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.lock, "bcache");

  for (int i = 0; i < NBUC; i++){
		initlock(&bcaches[i].lock, "bcache.bucket");
		bcaches[i].head.prev = &bcaches[i].head;
		bcaches[i].head.next = &bcaches[i].head;
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
		b->tick = -1;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
	struct buf *lru; // Potential LRU.
  int no = blockno%NBUC;
  acquire(&bcaches[no].lock);
  
  // Is the block already cached?
  for(b = bcaches[no].head.next; b != &bcaches[no].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
			b->tick = ticks;
      b->refcnt++;
      release(&bcaches[no].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Search in the bucket for LRU.
	lru = 0;
  for(b = bcaches[no].head.next; b != &bcaches[no].head; b = b->next){
    if(b->refcnt == 0){
			if (lru == 0){
				lru = b;
			} else{
				if (lru->tick >= b->tick) lru = b;
			}
    }
  }
	if (lru){
		lru->dev = dev;
  	lru->blockno = blockno;
  	lru->valid = 0;
  	lru->refcnt = 1;
		lru->tick = ticks;
  	release(&bcaches[no].lock);
  	acquiresleep(&lru->lock);
  	return lru;
	}

  // Recycle the least recently used (LRU) unused buffer.
	int ou;
	int flag = 1; 
	while (flag){
		lru = 0;
		flag = 0;
  	for (b = bcache.buf; b < bcache.buf+NBUF; b++){
    	if (b->refcnt == 0){ // With bcaches[no].lock held and the previous for loop passed, it can be said that this buffer won't be in bucket bcaches[no].
				flag = 1;
				if (lru == 0){
					lru = b;
				} else {
					if (lru->tick >= b->tick) lru = b;
				}
			}
		}
    if (lru){
			if (lru->tick == -1){ 
        acquire(&bcache.lock);
				if (lru->refcnt == 0){ // Lucky! The buffer is still available.
        	lru->dev = dev;
					lru->blockno = blockno;
					lru->valid = 0;
					lru->refcnt = 1;
					lru->tick = ticks; 

					lru->next = bcaches[no].head.next;
					lru->prev = &bcaches[no].head;
					bcaches[no].head.next->prev = lru;
					bcaches[no].head.next = lru;
					
					release(&bcache.lock);
					release(&bcaches[no].lock);
					acquiresleep(&lru->lock);
					return lru;
				} else { // Unlucky! Go and search for another buffer.
					release(&bcache.lock);
				}
			} else {  // So this buffer is in some buffer else.
				ou = (lru->blockno)%NBUC;
				acquire(&bcaches[ou].lock);
				if (lru->refcnt == 0){ // Lucky! The buffer is still available.
        	lru->dev = dev;
					lru->blockno = blockno;
					lru->valid = 0;
					lru->refcnt = 1;
					lru->tick = ticks;

					lru->next->prev = lru->prev;
					lru->prev->next = lru->next;
					lru->next = bcaches[no].head.next;
					lru->prev = &bcaches[no].head;
					bcaches[no].head.next->prev = lru;
					bcaches[no].head.next = lru;

					release(&bcaches[ou].lock);
					release(&bcaches[no].lock);
					acquiresleep(&lru->lock);
					return lru;
				} else { // Unlucky! Go and search for another buffer.
					release(&bcaches[ou].lock);
				}
	  	}
		}
	}

  panic("bget: no buffers");
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

	int ou = (b->blockno)%NBUC;
  acquire(&bcaches[ou].lock);
  b->refcnt--;
	if (b->refcnt == 0) b->tick = ticks;
  release(&bcaches[ou].lock);
}

void
bpin(struct buf *b) { 
	int ou = (b->blockno)%NBUC;

  acquire(&bcaches[ou].lock);
  b->refcnt++;
  release(&bcaches[ou].lock);
}

void
bunpin(struct buf *b) { 
	int ou = (b->blockno)%NBUC;

  acquire(&bcaches[ou].lock);
  b->refcnt--;
  release(&bcaches[ou].lock);
}


