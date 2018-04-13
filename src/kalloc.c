// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld
				   // 物理メモリのロードされたカーネルの終端

// freelistで空きpageを管理するための構造体
struct run {
  struct run *next;
};

// カーネルが割り当てる物理メモリの候補となるアドレスのポインタ
// kallocにおいてfreelistの先頭のrunを割り当てられる物理ページとして返す
// freelistはspinlockで守られている
struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
// 物理アドレスのendから4MBまでの領域をカーネルのフリーリストとして初期化する
// entrypgdirは[KERNBASE, KERNBASE+4MB]を[0, 4MB]と対応づけているため、mainの冒頭では4MBまでしか使えない
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

// [vstart, vend]までの領域を解放し、カーネルのフリーリストにつなげる
void
freerange(void *vstart, void *vend)
{
  char *p;
  // PGROUNDUP: オフセットの切り上げ
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
	// 1ページ分の領域を解放し、カーネルのフリーリストにつなげる
	// 初期化におけるkfreeは、まだ獲得していない物理アドレス空間をフリーリストにつなぐこととなるため、本来の意図とは反している
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// vが指しているページの物理アドレスを解放し、カーネルのフリーリストにつなぐ
void
kfree(char *v)
{
  struct run *r;

  // vがページの先頭か
  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  // 1詰めして、メモリを破壊する(解放前の情報を残すと,dangling pointerによって誤作動する可能性がある)
  memset(v, 1, PGSIZE);

  if(kmem.use_lock)
    acquire(&kmem.lock);
  // 空いた領域をrun構造体でキャスト
  r = (struct run*)v;
  // rをkmem.freelistの先頭につなげる
  r->next = kmem.freelist;
  kmem.freelist = r;
  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// 1ページ分(4096byte)の物理メモリを確保し、そのアドレスを返す
// kmemのフリーリストの先頭を返す
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

