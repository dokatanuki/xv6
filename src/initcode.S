# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"
#include "traps.h"


# はじめに
# exec(init, argv)
.globl start
start:
  // システムコールの引数をユーザスタックに積む
  pushl $argv
  pushl $init
  // 戻り番地をゼロに指定する
  // eip
  pushl $0  // where caller pc would be
  // システムコールの番号をeaxに積む
  // カーネルモードに遷移
  movl $SYS_exec, %eax

  // TBC: 引数をスタックに積んでない？
  // 内部でよしなにやってる？
  // 戻り番地は？
  // int n: nはtrapnoであり，割り込みの種類
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

