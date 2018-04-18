// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
// スレッドセーフな関数ではない
// lockを獲得してからreleaseで解放するまで割り込みされないようにpushcli()を呼び出す=>releaseの最後でpopcli()を呼び出す
void
acquire(struct spinlock *lk)
{
  pushcli(); // disable interrupts to avoid deadlock.
  // 自分自身がロックを獲得していたらおかしい
  if(holding(lk))
    panic("acquire");

  // The xchg is atomic.
  // lockがすでに獲得されている場合のxchgの返り値は1
  // つまり、lockが獲得できる状態になるまでビジーループ
  while(xchg(&lk->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  // コンパイラの最適化によるメモリアクセス命令の順序の変化を防ぐ
  // buildin function
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk->cpu = mycpu();
  getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
// acquireでpushcli()を実行=>releaseの最後にpopcli()
void
release(struct spinlock *lk)
{
  // 自身がロックを獲得していないのに呼び出していたらおかしい
  if(!holding(lk))
    panic("release");

  // デバッグ情報の初期化
  lk->pcs[0] = 0;
  lk->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (lk->locked) : );

  popcli();
}

// Record the current call stack in pcs[] by following the %ebp chain.
// デバッグ用関数
void
getcallerpcs(void *v, uint pcs[])
{
  uint *ebp;
  int i;

  ebp = (uint*)v - 2;
  for(i = 0; i < 10; i++){
    if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
      break;
    pcs[i] = ebp[1];     // saved %eip
    ebp = (uint*)ebp[0]; // saved %ebp
  }
  for(; i < 10; i++)
    pcs[i] = 0;
}

// Check whether this cpu is holding the lock.
int
holding(struct spinlock *lock)
{
  return lock->locked && lock->cpu == mycpu();
}


// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void
pushcli(void)
{
  int eflags;

  eflags = readeflags();
  // 割り込みを無効化する
  cli();
  // pushしているため、ここは必ず0にはならない
  // マルチスレッドではもしかしたら0になる？
  // pushするまえに割り込みが可能であったか？
  if(mycpu()->ncli == 0)
	// TBC: push前に割り込み可能でない状態はどんな状況？
    mycpu()->intena = eflags & FL_IF;
  mycpu()->ncli += 1;
}

void
popcli(void)
{
  if(readeflags()&FL_IF)
    panic("popcli - interruptible");
  if(--mycpu()->ncli < 0)
    panic("popcli");
  // push前に割り込みが可能でなかった場合にはそのままにしておく
  if(mycpu()->ncli == 0 && mycpu()->intena)
    sti();
}

