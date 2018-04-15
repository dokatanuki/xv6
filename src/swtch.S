# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

// TBC: mpmainから呼び出された際，espはどこを指しているのか？(Chapter2読了時点では不明)
// プロセスがまだひとつも動いていない状態であり，pgdirはkpgdirだと思われる(初期化コードをすべておっていないため，あくまで推測)
// oldで渡されているのはschedulerのスタック?
// espはpushやpop等で値が自動的に変化する特殊なレジスタであるため，ebpが用意されていて，スタックの内容を参照，変更できるように設計されている
// 本来，ebpは関数呼び出しの前後で同じである必要があり，return addressの上にebpを積んで，
// espをebpにコピーする操作が関数のはじめに入る
// espをebpで保持することで関数内で統一的にスタックの内容にアクセスできる, espはpop, push等で変化してしまうため，同じ値にアクセスするにしてもoffsetが変化してしまうため不便
// さらに，return addres, ebpのうえにローカル変数を確保していくときにも，ebpからのoffsetは変化しないため，ローカル変数へのアクセスも容易
// そして関数の終わりで，movl ebp espでespの復元，pop ebpでebpの値を復元
// これはebpの値を保持できることに加え，espの値がreturn addressを指すため，ローカル変数を消去する役割も担っている
// espはreturn addressを指しているため，retで戻れる
// 引数の分だけstackをあげる必要があるが，タイミングは呼び出し規則に依存する
// ここではcontextにてebpを保存するため，ebpをスタックに積む操作は行われていない
// espはreturn addressを指していて，引数へのアクセスは4byte上から始まる
// [x86アセンブリ言語での関数コール](https://vanya.jp.net/os/x86call/ "x86アセンブリ言語での関数コール")

.globl swtch
swtch:
  // 関数を呼び出す際，スタックにはargv[argc-1], argv[argc], ..., argv[0], return addressが順に積まれる
  // espより4byte上->argv[0]
  movl 4(%esp), %eax
  // espより8byte上->argv[1]
  movl 8(%esp), %edx

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  // eaxに現在のスタックの値を保持
  movl %esp, (%eax)
  // 新しいcontextへのポインタをespに読み出し
  // contextはkstackで設定されている
  movl %edx, %esp

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  // この段階で，popするとcontext->eipが取り出される
  ret
