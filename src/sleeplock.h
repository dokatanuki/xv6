// Long-term locks for processes
// xv6ではoverheadが少ないため一般的にはspinlockが使われる
// File System でのみsleeplockが使用される
// spinlockでは割り込みを無効化し、ロックが獲得できるまでビジーループ
// sleeplockではロックの獲得自体をスリープで待てる
struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};

