// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define NDIRECT 12
// バッファへのポインタの配列
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
// ディスク上のinodeの構造
struct dinode {
  // file, directory, もしくはデバイスなど
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  // このinodeを参照しているディレクトリの数(hardlinkではnlinkが増加, symbliclinkでは増加しない)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  // このinodeの実体ブロックへのアドレスの配列
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
// バッファにいくつのinodeが入るか
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
// inode iが格納されているbufferのblock no
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  // inum=0はfreeであることを示している
  ushort inum;
  char name[DIRSIZ];
};

