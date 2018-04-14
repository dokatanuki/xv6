// Memory layout

#define EXTMEM  0x100000            // Start of extended memory
#define PHYSTOP 0xE000000           // Top physical memory
// 0xE000000=3758096384
#define DEVSPACE 0xFE000000         // Other devices are at high addresses
// 0xFE000000=68182605824

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE 0x80000000         // First kernel virtual address
// 0x80000000=2147483648

#define KERNLINK (KERNBASE+EXTMEM)  // Address where kernel is linked

#define V2P(a) (((uint) (a)) - KERNBASE)
// Physical to Virtual
// 物理アドレスを、カーネルの仮想アドレスに変換すると捉えるとすっきりする
#define P2V(a) (((void *) (a)) + KERNBASE)

#define V2P_WO(x) ((x) - KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE)    // same as P2V, but without casts
