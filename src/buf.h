// diskのブロックのデータ管理
// bufはI/O処理が必要であるため，ロックの獲得に時間がかかる->sleeplock
struct buf {
  // データの状態を意味する(B_VALID, B_DIRTY)
  int flags;
  // デバイス番号
  uint dev;
  // セクター番号
  uint blockno;
  struct sleeplock lock;
  // reference count: 現在参照しているスレッドの数, buffer cacheの処理で利用される
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;
  struct buf *qnext; // disk queue
  uchar data[BSIZE];
};
#define B_VALID 0x2  // buffer has been read from disk
#define B_DIRTY 0x4  // buffer needs to be written to disk

