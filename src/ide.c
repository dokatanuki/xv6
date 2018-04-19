// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;
static void idestart(struct buf*);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
  int r;

  // I/Oportの0x1f7にはdiskの状態が格納されている
  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

// IDE: Integrated Device Electronics
// I/Oインタフェース
// I/O APICのIDE割り込みを有効化
void
ideinit(void)
{
  int i;

  initlock(&idelock, "ide");
  // ioapicからIRQ_IDEを最後のCPUのLAPICに対して通知できるように初期化
  ioapicenable(IRQ_IDE, ncpu - 1);
  // Motherboardのポートを見て，ディスクが有効になるまで待機する
  idewait(0);

  // Check if disk 1 is present
  // 0x1f6が指すportの4bit目が1ならディスク01を使用することを意味する
  outb(0x1f6, 0xe0 | (1<<4));
  // busyループもどき
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
	  // ディスク01が存在するかどうかのフラグ
      havedisk1 = 1;
      break;
    }
  }

  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE)
    panic("incorrect blockno");
  int sector_per_block =  BSIZE/SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;
  int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
  int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

  if (sector_per_block > 7) panic("idestart");

  idewait(0);
  outb(0x3f6, 0);  // generate interrupt
  outb(0x1f2, sector_per_block);  // number of sectors
  outb(0x1f3, sector & 0xff);
  outb(0x1f4, (sector >> 8) & 0xff);
  outb(0x1f5, (sector >> 16) & 0xff);
  outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
	// 書き込み
    outb(0x1f7, write_cmd);
	// TBC: 何をしている?
	// bufの中のデータの1/4をどこかに渡している
	// write命令を行うことをportにセットし，書き込みデータを指定の場所に送っている？
    outsl(0x1f0, b->data, BSIZE/4);
  } else {
    outb(0x1f7, read_cmd);
  }
}

// Interrupt handler.
// ideからの割り込みがあった際に呼び出される
// idequeueの先頭が処理されたことを意味し、先頭からbufを取り除き、
// そのbufをchanとして待つプロセスをSLEEPINGからRUNNABLEに変える
void
ideintr(void)
{
  struct buf *b;

  // First queued buffer is the active request.
  acquire(&idelock);

  if((b = idequeue) == 0){
    release(&idelock);
    return;
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
    insl(0x1f0, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  wakeup(b);

  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

  release(&idelock);
}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
// 対象のバッファを処理する
// リストの操作はクリティカルセクションであるため、ロックで保護する
void
iderw(struct buf *b)
{
  struct buf **pp;

  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev != 0 && !havedisk1)
    panic("iderw: ide disk 1 not present");

  // sleepを実行するまえにロックを獲得する
  // 条件を調べて，スリープに入る前にwakeupが実行されないように
  // ideintrでもidelockを獲得しているはず
  acquire(&idelock);  //DOC:acquire-lock

  // Append b to idequeue.
  // idequeueの末尾にバッファを追加
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;

  // 対象のバッファがリストの先頭であった場合，idestartを送る必要がある
  // Start disk if necessary.
  if(idequeue == b)
	// diskの処理開始, 終わったら割り込みを入れてくる？
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
	// TBC: 誰がwakeする？diskからbufの処理完了のwakeが来るまでスリープする
    sleep(b, &idelock);
  }


  release(&idelock);
}
