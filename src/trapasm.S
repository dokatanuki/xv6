#include "mmu.h"

  # vectors.S sends all traps here.
  # レジスタの値を保持する
.globl alltraps
alltraps:
  # int命令で積んでいないユーザのレジスタをすべて積む
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  # 汎用レジスタの値を積む(eax, ecx, edx, ebx, oesp, ebp, esi, edi)
  # eaxにはSystem Call Numberが入っている
  pushal
  
  # Set up data segments.
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es

  # Call trap(tf), where tf=%esp
  # 引数を積む, espはtrapの引数
  pushl %esp
  # 返り値を積んでtrapに飛ぶ
  # trapでは状況に応じてユーザスタックに積まれた引数を見て，対応する割り込みハンドラを実行する 
  call trap
  # 引数の破棄
  addl $4, %esp

  # Return falls through to trapret...
  # trapframeからユーザーのレジスタを復元して， iretを呼び出して特権レベルを下げてユーザ空間に戻る
  # TBC: 特権レベルが下がってるのに戻った後もカーネルスタックを使っているようにみえる 
.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret
