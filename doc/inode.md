# File System

cacheはあくまで副産物
データアクセスをいかに抽象化するか

## xv6におけるディスクの階層
1. disk (iderw)
	- block単位でディスクに読み書きするためのインタフェース
	- xv6では512[byte/block]

2. buf (bread, bwrite, brelse)
	- memory上に読み出されたdiskに存在する実体のコピーであり，bufを通してデータを操作することで，ディスクとメモリにおけるデータ齟齬を意識することなく高速に処理をすることが可能となる
	- bufはblockのデータおよびそのblockに関するメタデータをもつ構造体で定義される
	- device number, sector numberでblockを一意に特定

3. log (begin\_op, end\_op)
	- system callはbegin\_op, end\_opでwrapされている(transaction)
	- log\_writeはbwriteのプロキシ, bread-\>modify data-\>log\_write-\>brelse
	- end\_opにおけるcommitでlog\_writeされてきたbwriteを一括して実行する
	- crashしてもデータの整合性を維持する(transaction)
	- 複数の書き換え処理を一つの処理として捉えることができる

4. inode (iget, ilock, iupdate, iunlock, iput)
	- memory上に読み出されたdiskに存在するinodeの実体のコピーであり，inodeによってblockを意味のあるまとまりとして管理する
	- inodeのメタデータを管理する際に利用される

5. inode content (readi, writei)
	- inodeが管理しているblock群を読み書きする際に利用される
	- writeiではlog\_writeが使用される

6. pathname ()
	- pathでinodeにアクセスすることを可能とするインタフェース

7. file descriptor
	- あらゆるリソースを抽象化し，file descriptorとして表現する
	- file descriptorはどのようなtypeであったとしてもfilewriteという関数で書き込みを行える
