// Sleeping locks

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

// sleeplockを獲得する、すでに獲得されていたら待ち状態に入り、スリープする(relesesleepによって起こされる)
// acquireとの違いは、ロックの獲得自体をスリープで待てる点である
void
acquiresleep(struct sleeplock *lk)
{
  // &lk->lk: sleeplock内のspinlockへのポインタ
  // spinlockを獲得
  // sleeplockの競合が起こらないように
  acquire(&lk->lk);
  while (lk->locked) {
	// spinlockを解放し、再開するときに再獲得する
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->pid = myproc()->pid;
  // spinlockを解放
  release(&lk->lk);
}

// sleeplockを解放し、待っているスレッドがいたら起こす
void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->pid = 0;
  // 解放しようとしているsleeplockを待っているスレッドがあったらそいつに通知する
  wakeup(lk);
  release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked;
  release(&lk->lk);
  return r;
}



