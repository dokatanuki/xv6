// diskのブロックのデータ管理
// TBC: ディスク内の実体とのデータの整合性をいかにして保つのか
struct buf {
  // データの状態を意味する(B_VALID, B_DIRTY)
  int flags;
  // デバイス番号
  uint dev;
  // セクター番号
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE];
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

