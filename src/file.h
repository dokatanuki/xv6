struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  // このinodeを参照しているCのポインタの数
  // 0になったらメモリから退去させる
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  // addrs[NDIRECT]にはさらにaddrsの配列が格納されている
  uint addrs[NDIRECT+1];
};

// table mapping major device number to
// device functions
// デバイスのリストで，デバイスはinodeとして開かれている
// それぞれのデバイスに対するread, writeの挙動をセットしてやる
// e.g. mainn関数のconsoleinit
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
