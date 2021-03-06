// The I/O APIC manages hardware interrupts for an SMP system.
// http://www.intel.com/design/chipsets/datashts/29056601.pdf
// See also picirq.c.

#include "types.h"
#include "defs.h"
#include "traps.h"

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0xb0  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

// volatile: ioapic変数に関する処理をコンパイラの最適化の対象外とする
volatile struct ioapic *ioapic;

// IO APIC MMIO structure: write reg, then read or write data.
// I/O APIC: I/O割り込みを受付け，リダイレクトテーブルを参照してLAPICに通知する
struct ioapic {
  uint reg;
  uint pad[3];
  uint data;
};

static uint
ioapicread(int reg)
{
  ioapic->reg = reg;
  return ioapic->data;
}

static void
ioapicwrite(int reg, uint data)
{
  // 仕様書で定義されている番号を入れてやると，IOAPICが内部で対応するエントリにdataを代入してくれる
  // ioapicがvolatileでない場合は，順番が入れ替わる可能性がある
  ioapic->reg = reg;
  ioapic->data = data;
}

void
ioapicinit(void)
{
  int i, id, maxintr;

  // MMIOによって仮想アドレスにマッピングされたIOAPICのアドレスをioapicにセットする
  ioapic = (volatile struct ioapic*)IOAPIC;
  // IOAPICVERの[23:16]を取り出す
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  // IOAPICのuniqueなIDの取り出し
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
    cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  // 全てのIRQをDISABLEDにセットする
  // CPUは割り当てない
  // T_IRQ0: 
  // RED_TABLE: リダイレクトテーブルのベースアドレス
  // RED_TABLE+2*i: i番目のリダイレクトエントリ
  for(i = 0; i <= maxintr; i++){
	// リダイレクトテーブルのエントリ()
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
	// CPUの指定は行わない
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

// IOAPICのIRQ(どの割り込みかを指定)に対して，cpuのlapicの番号を割り当てる
// IOAPICに用意されている割り込みを指定し，それに対してcpuを割り当てる
void
ioapicenable(int irq, int cpunum)
{
  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}
