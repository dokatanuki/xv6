#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  // 読み込まれたbyte数
  uint nread;     // number of bytes read
  // 書き込まれたbyte数
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *p;

  p = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((p = (struct pipe*)kalloc()) == 0)
    goto bad;
  p->readopen = 1;
  p->writeopen = 1;
  p->nwrite = 0;
  p->nread = 0;
  initlock(&p->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = p;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = p;
  return 0;

//PAGEBREAK: 20
 bad:
  if(p)
    kfree((char*)p);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *p, int writable)
{
  acquire(&p->lock);
  if(writable){
    p->writeopen = 0;
    wakeup(&p->nread);
  } else {
    p->readopen = 0;
    wakeup(&p->nwrite);
  }
  if(p->readopen == 0 && p->writeopen == 0){
    release(&p->lock);
    kfree((char*)p);
  } else
    release(&p->lock);
}

//PAGEBREAK: 40
//addr[0: n-1]をpに書き込む
//pipewriteではp->nwriteの値しか変化しない
int
pipewrite(struct pipe *p, char *addr, int n)
{
  int i;

  // pipeのロックを獲得する(pのデータを保護するため)
  acquire(&p->lock);
  for(i = 0; i < n; i++){
	// pipeのbufがマックスだったら読み出されるまでsleepする
    while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
	  // このパイプの読み込み口が閉じている
	  // もしくはこのプロセスがキルされた
      if(p->readopen == 0 || myproc()->killed){
        release(&p->lock);
        return -1;
      }
	  // nreadを読み出そうとして待っているプロセスを起こす
	  // 起きてもこいつがp->lockを持っているからすぐにwakeupすることはできない
	  // つまり，sleepする前に書き出されることはない
      wakeup(&p->nread);
	  // p->nwriteをchanにしてsleep
	  // p->nwriteは動的に変化する値であるため，この待ちを行なっているパイプがたくさんあるという状態にはならない
	  // 仮に定数であったとしたら，他のパイプを待っているプロセスも起こしてしまうことになる
      sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
    }
	// PIPESIZEを越えて書き込まないように%PIPESIZEでClampしてやる
    p->data[p->nwrite++ % PIPESIZE] = addr[i]j;
  }
  wakeup(&p->nread);  //DOC: pipewrite-wakeup1
  release(&p->lock);
  return n;
}

// pからaddr[0: n-1]に読み出す
// pipereadではp->nreadの値しか変化しない
int
piperead(struct pipe *p, char *addr, int n)
{
  int i;

  // readしている間はwriteできないように，ロックを獲得する
  acquire(&p->lock);
  while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
	// プロセスがキルされた
    if(myproc()->killed){
      release(&p->lock);
      return -1;
    }
	// 読みだせるものがない場合，p->nreadをchanにしてsleepする
    sleep(&p->nread, &p->lock); //DOC: piperead-sleep
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(p->nread == p->nwrite)
      break;
    addr[i] = p->data[p->nread++ % PIPESIZE];
  }
  wakeup(&p->nwrite);  //DOC: piperead-wakeup
  release(&p->lock);
  return i;
}
