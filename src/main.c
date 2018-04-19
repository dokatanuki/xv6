#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
  // ここの呼び出しの段階で動いているスレッドがschedulerのスレッドとなる
  // TBC: espはどこを指している？
  // 4MB+KERNBASE-end分の領域を解放し、カーネルのフリーリストにつなげる
  // 仮想アドレス空間における範囲を指定している
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  // schedulerが利用するkpgdirを作成し、cr3にセットする
  kvmalloc();      // kernel page table
  // -----------↓ ここからcr3=kpgdir↓ -----------
  // mp: multi processor
  mpinit();        // detect other processors
  // LAPICの初期化，TBC: LAPICに内臓されているtimerの割り込みを処理できるようにセットアップして，TSSを有効化する？
  lapicinit();     // interrupt controller
  seginit();       // segment descriptors
  picinit();       // disable pic
  // IOAPICのIRQに割り込みをセットする(CPUの割り当ては行わない)
  ioapicinit();    // another interrupt controller
  // openされたコンソールのinodeに対応するdevice構造体に, read, writeの関数をセットする
  // ioapicenableでIRQ_KBDをcpu0に割り当てる
  consoleinit();   // console hardware
  uartinit();      // serial port
  pinit();         // process table
  // Interrupt Descriptor Table(IDT)にgatedescriptorをセットする
  tvinit();        // trap vectors
  binit();         // buffer cache
  fileinit();      // file table
  // IDE接続のデバイス(Diskなど)のセットアップ(I/Oデバイスの割り込みを有効化)
  ideinit();       // disk 
  startothers();   // start other processors
  // 4MBからMMIO領域の手前(物理アドレスの限界)までをフリーリストにつなげる
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  // 最初のプロセスのセットアップ(切り替えはmpmain)
  userinit();      // first user process
  mpmain();        // finish this processor's setup
}

// Other CPUs jump here from entryother.S.
static void
mpenter(void)
{
  switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

// Common CPU setup code.
static void
mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();       // load idt register
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  scheduler();     // start running processes
}

pde_t entrypgdir[];  // For entry.S

// Start the non-boot (AP) processors.
static void
startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void**)(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain()
    while(c->started == 0)
      ;
  }
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.
// attributeはgccにおけるオプション
// [Specifying Attributes of Variables](https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Variable-Attributes.html#Variable%20Attributes "Specifying Attributes of Variables")
// 仮想アドレス[0, 4MB), [KERNBASE, KERNBASE+4MB)を物理アドレス[0, 4MB)と対応づけるマッピング
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

