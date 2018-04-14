#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  // TBC: FSに関する操作ではじめに呼び出される
  begin_op();

  // namei(path): pathをOpenし、inodeを返す
  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  // inodeをロックし、必要に応じてディスクから読み出す
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  // readi(struct inode *ip, char *dst, uint off, uint n): ipのoffからnbyteだけdstに読み出す
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  // 対象ファイルがELF形式になっているか
  // elf.magicがELF_MAGICであるならば、対象のファイルはELF formatに則っていると判断できる
  if(elf.magic != ELF_MAGIC)
    goto bad;

  // カーネルのマッピング(KERNBASE以上のアドレスを物理アドレスと対応づける)
  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  // プログラムヘッダを読み込み(xv6では1つだが、他のシステムでは複数存在)
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
	// 仮想アドレス空間における[0, ph.vaddr+memsz]の領域を確保
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
	// 確保した[0, ph.vaddr+memsz]に対してプログラムを読みだす
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  // inodeのロックの解除
  iunlockput(ip);
  // FSのfinalize
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  // data領域を保護するためのguard page(マッピングなし)と、ユーザスタック
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  // guard pageのPTE_Uをクリア
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  // sp(スタックポインタ)をスタックの最上部にセット
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  // ユーザスタックに読み出すELFファイルのmain関数の値をセットする
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
	// sp = (sp - (文字列長+1)) & 111..1100
	// TBC: 4の倍数のアドレスから開始させるために下位2bitは切り捨てる
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
	// pgdirはsetupkvmでカーネル部分のマッピングは済んでいる
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  // ustackをstackにまるまる積んだ際の、スタックポインタの頂点
  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  // 現在実行中のpgdirを保持
  // curproc->pgdirに入っているのはpgdirの物理アドレス
  // つまり、P2V(oldpgdir)を行えば、カーネルの仮想アドレス空間からアクセス可能
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;  // あたかもmain関数が呼び出されたかのように自前でセットしたユーザスタックのスタックポインタ
  // pgdirを新しいものに変更
  // 変更後もkernelの仮想アドレス空間は生きている(setupkvmでセットしたため)
  switchuvm(curproc);
  // freevm内で、物理アドレスをカーネルの仮想アドレス空間の仮想アドレスに変換するため、ページテーブルが切り替わっていてもoldpgdirにアクセス可能
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
