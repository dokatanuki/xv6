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
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  // bufferの実体
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // LRUであり，使われたバッファは先頭につなぎ直す
  // cacheへのアクセスはbufではなくheadからのリストを介して行われる
  struct buf head;
} bcache;

// 双方向リストであるバッファリストを初期化する
void
binit(void)
{
  struct buf *b;

  // bcacheのlockを初期化
  initlock(&bcache.lock, "bcache");

//PAGEBREAK!
  // Create linked list of buffers
  // 自分自身を指すようにリストを初期化
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // buffer cache をロックする
  acquire(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock);
	  // buffer単位でsleeplock
	  // これからこのバッファを使うからロックを獲得
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // Even if refcnt==0, B_DIRTY indicates a buffer is in use
  // because log.c has modified it but not yet committed it.
  // bcache.head.prevはリストの末尾
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
	// 現在どのスレッドも使用していなくて，書き戻しも必要ない
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->dev = dev;
      b->blockno = blockno;
	  // flagsはB_VALIDでもB_DIRTYでもない
	  // breadによってdiskから正しく読みだしてくれる
	  // B_VALIDだったら以前のdataを使用してしまう
      b->flags = 0;
      b->refcnt = 1;
      release(&bcache.lock);
	  // refcntがすでに1であるため，reusedされる心配はないため，
	  // bcacheをreleaseしたあとでもおっけー
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  // devのblocknoからblockを読み出す
  // もしキャッシュになかったら空いてるキャッシュに情報だけを詰める
  b = bget(dev, blockno);
  // キャッシュに入ったばかりの場合はまだデータが入っていないため，ディスクから読み出す
  if((b->flags & B_VALID) == 0) {
    iderw(b);
  }
  // bを使うときはロックを獲得する
  return b;
}

// Write b's contents to disk.  Must be locked.
// b.lockが獲得された状態でないといけない
// bread(dev, blockno)でb->lockが獲得された状態でbがかえってくるため，
// lockは獲得されているはずである．
// 変更が起きた場合にはbwriteを呼んでやってb->flagsにB_DIRTYをセットし，
// iderwをよびだす
// 使い終わったらb->lockをreleaseする必要がある
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  b->flags |= B_DIRTY;
  iderw(b);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}
//PAGEBREAK!
// Blank page.

