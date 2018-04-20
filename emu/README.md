# Emulate xv6 in VM

## Overview
Set up environment emulating xv6 by qemu.

## Requirement
- VirtualBox
- Vagrant

## Installation
1. Set up Vagrantfile and VirtualMachine
```
$ git clone https://github.com/dokatanuki/xv6/emu
$ ./setup_vm.sh
```

2. Login ubuntu and execute shell script
```
$ vagrant ssh
$ ./setup_buildtools.sh
```

3. Change into xv6\_public and modify Makefile
```
-#QEMU = 
+QEMU = /usr/bin/qemu-system-i386
```
You should disable compiler optimization.
```
-CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -02 ...
+CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-...
```

4. Emulate xv6
```
$ make qemu-nox
```

## ctags
If you use vim editor, I recommends to install ctags
```
$ sudo apt-get install ctags -y
```
Change into xv6\_public and generate tags
```
$ cd xv6_public
$ ctags -R
```
Then get .vimrc and open xv6 programs by vim and update plugin
```
$ curl https://raw.githubusercontent.com/dokatanuki/dotfiles/master/.vimrc > ~/.vimrc
$ vim

# in vim
:PlugInstall
```
You can use ctags in vim
```
# on the code you wanna check define
jump to define: <C-]>
back to prev line: <C-o>
```


## Note
### QEMU
- terminate qemu
```
<C-a> x
```

### xv6
- add command to xv6

Add your program.c in xv6\_public and modify Makefile
```
UPROGS=\
	_cat\
	_echo\
	.
	.
	.
	_program\
```
Remake xv6 then your program is added

- debug xv6
Emulate xv6
```
$ make qemu-nox-dbg
```
Open another terminal and run gdb
```
$ gdb kernel
(gdb) target remote localhost:26000
(gdb) source ./
(gdb) break main
(gdb) la regs
(gdb) la src
(gdb) cont
```


## Reference
[xv6のデバッグ環境をつくる](https://qiita.com/ksky/items/974ad1249cfb2dcf5437 "xv6のデバッグ環境をつくる")  
[Mac で xv6 を gdb でデバッグする](http://sairoutine.hatenablog.com/entry/2016/09/03/002354 "Mac で xv6 を gdb でデバッグする")
