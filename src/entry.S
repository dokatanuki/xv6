# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0
  .long magic
  .long flags
  .long (-magic-flags)

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
# _startはELF(Executable and Linkable Format)におけるエントリポイントとなるシンボル
# entry.S実行時はpagingが有効でないため、仮想アドレスがそのまま物理アドレスに対応している
# そのため、仮想アドレスを物理アドレスに変換して_startにセットしてやる
.globl _start
_start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages
  # cr4にCR4_PSEを立て、PSE(Page Size Extention)を有効にする
  # PSE: ページのサイズとして、規定の4KByte以外のサイズを指定できるようになる
  # 最初のページテーブル(entrypgdir)はスーパーページ(4MByte)を利用している
  # ひとつのエントリで連続した4MBを割り当てられる(トレードオフ)
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Set page directory
  # entrypgdirをcr3にセットする
  # cr3にセットするアドレスは物理アドレスである
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3
  # Turn on paging.
  # cr0にフラグを立て、PagingとWrite Protectionを有効にする
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.
  # espレジスタをstackの最上部にセットする
  # stackは上から下にのびる
  movl $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  # 上位のアドレス空間のmainへアセンブラの間接ジャンプで飛ぶ
  mov $main, %eax
  jmp *%eax

.comm stack, KSTACKSIZE
