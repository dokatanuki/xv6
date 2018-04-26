# Emulate xv6 in VM

## Overview
QEMUを用いてxv6をエミュレートする．

## Requirement
- VirtualBox
- Vagrant

## Install and setup environment
1. `適当な作業用ディレクトリに移動する`
```sh
$ cd ~/workspace/xv6_emu
```

2. `セットアップ用のshell scriptを取得する`
```sh
$ curl https://raw.githubusercontent.com/dokatanuki/xv6/master/emu/setup_vm.sh > setup_vm.sh
$ curl https://raw.githubusercontent.com/dokatanuki/xv6/master/emu/setup_buildtools.sh > setup_buildtools.sh 
```

3. `Vagrant(VirtualBox)のセットアップを行う`
```sh
$ bash setup_vm.sh
```

4. `仮想環境にログインし，ツールをインストールする`
```sh
$ vagrant ssh
$ bash setup_buildtools.sh
```

5. `Makefileを修正する`
```sh
$ cd ~/xv6_public
$ vim Makefile
```
修正箇所は以下の通りである．  
```
- #QEMU = 
+ QEMU = /usr/bin/qemu-system-i386
```
```
- CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -02 ...
+ CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -fvar-tracking -fvar-...
```

## Note
### QEMU
`QEMUを終了する`
```
<C-a> x
```

`xv6をqemu上で起動する`
```
$ make qemu-nox
```

`xv6をgdbでデバッグする`
1. xv6\_publicディレクトリ内でqemuを実行する．  
```
$ make qemu-nox-dbg
```

2. 別のターミナルからvagrant sshで接続し，xv6\_publicディレクトリで以下のコマンド群を実行する．  
```
$ gdb kernel
(gdb) target remote localhost:26000
(gdb) source ./
(gdb) break main
(gdb) la regs
(gdb) la src
(gdb) cont
```
プログラムの実行順が不規則である場合，コンパイラの最適化を外し忘れている可能性がある．  

### gdb tips
`ソースコード，レジスタを表示する`
```
(gdb) la src
(gdb) la regs
```

`ローカル変数，実引数を表示する`
```
(gdb) info locals
(gdb) info args
```

`バックトレース(現在実行している処理にたどり着く過程)`
```
(gdb) bt
```

`式を評価して表示`
```
(gdb) p hoge
````

### xv6
`xv6にコマンドを追加する`  

追加するコマンドがprogram.cに記述されているとする．  
program.cをxv6\_publicに追加し，MakefileのUPROGSにprogramを追加する．  
```
UPROGS=\
	_cat\
	_echo\
	.
	.
	.
	_program\
```

## Reference
[xv6のデバッグ環境をつくる](https://qiita.com/ksky/items/974ad1249cfb2dcf5437 "xv6のデバッグ環境をつくる")  
[Mac で xv6 を gdb でデバッグする](http://sairoutine.hatenablog.com/entry/2016/09/03/002354 "Mac で xv6 を gdb でデバッグする")
