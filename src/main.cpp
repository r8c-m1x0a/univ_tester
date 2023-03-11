//=====================================================================//
/*!	@file
	@brief	R8C UART サンプル @n
			・８ビット１ストップ・ビット
			P1_0: LED1 @n
			P1_1: LED2 @n
			P1_4: TXD(output) @n
			P1_5: RXD(input)
    @author 平松邦仁 (hira@rvf-rc45.net)
	@copyright	Copyright (C) 2017, 2021 Kunihito Hiramatsu @n
				Released under the MIT license @n
				https://github.com/hirakuni45/R8C/blob/master/LICENSE
*/
//=====================================================================//
#include <cstdio>

#include "common/renesas.hpp"

#include "common/fifo.hpp"

#include "common/vect.h"
#include "common/input.hpp"
#include "M120AN/system.hpp"
#include "M120AN/intr.hpp"
#include "M120AN/uart.hpp"

#pragma pack(1)
enum class U0MR_SMD : uint8_t {
  DISABLED = 0,
  SYNC = 1,
  BIT_LEN7 = 4,
  BIT_LEN8 = 5,
  BIT_LEN9 = 6
};

enum class U0MR_STPS : uint8_t {
  STOP_BIT_1 = 0,
  STOP_BIT_2 = 1
};

typedef union _u0mr_t {
  struct {
    unsigned int smd:3;
    unsigned int ckdir:1;
    unsigned int stps:1;
    unsigned int pry:1;
    unsigned int prye:1;
    unsigned int reserved0:1;
  } bits;
  uint8_t as_uint8;

  _u0mr_t(uint8_t u = 0) {
    this->as_uint8 = u;
  }

  _u0mr_t& with_smd(U0MR_SMD smd) {
    this->bits.smd = (uint8_t)smd;
    return *this;
  }

  _u0mr_t& with_stps(U0MR_STPS stps) {
    this->bits.stps = (uint8_t)stps;
    return *this;
  }

  void set(const _u0mr_t& u) volatile {
    this->as_uint8 = u.as_uint8;
  }
} u0mr_t;

enum class U0C0_CLK : uint8_t {
  DIV1 = 0,
  DIV8 = 1,
  DIV32 = 2
};

typedef union _u0c0_t {
  struct {
    unsigned int clk:2;
    unsigned int reserved0:1;
    unsigned int txept:1;
    unsigned int dfe:1;
    unsigned int nch:1;
    unsigned int ckpol:1;
    unsigned int uform:1;
  } bits;
  uint8_t as_uint8;

  _u0c0_t(uint8_t u) {
    this->as_uint8 = u;
  }

  _u0c0_t() {
    this->as_uint8 = 0;
  }

  void set_clk(U0C0_CLK clk) volatile {
    this->bits.clk = (uint8_t)clk;
  }
} u0c0_t;

typedef union _u0c1_t {
  struct {
    unsigned int te:1;
    unsigned int ti:1;
    unsigned int re:1;
    unsigned int ri:1;
    unsigned int u0irs:1;
    unsigned int u0rrm:1;
    unsigned int reserved0:2;
  } bits;
  uint8_t as_uint8;

  _u0c1_t(uint8_t u = 0) {
    this->as_uint8 = u;
  }

  _u0c1_t(const volatile _u0c1_t& u): _u0c1_t(u.as_uint8) {
  }

  void set(const _u0c1_t& u) volatile {
    this->as_uint8 = u.as_uint8;
  }

  _u0c1_t& with_te(bool enabled) {
    this->bits.te = enabled ? 1 : 0;
    return *this;
  }

  _u0c1_t& with_re(bool enabled) {
    this->bits.re = enabled ? 1 : 0;
    return *this;
  }

  void clr_err() volatile {
    this->bits.re = 0;
    this->bits.re = 1;
  }

  bool is_tx_buf_empty() volatile {
    return this->bits.ti == 1;
  }
} u0c1_t;

typedef union _u0ir_t {
  struct {
    unsigned int reserved0:2;
    unsigned int u0rie:1;
    unsigned int u0tie:1;
    unsigned int reserved1:2;
    unsigned int u0rif:1;
    unsigned int u0tif:1;
  } bits;
  uint8_t as_uint8;

  _u0ir_t(uint8_t u = 0) {
    this->as_uint8 = u;
  }

  _u0ir_t& with_rie(bool enabled) {
    this->bits.u0rie = enabled ? 1 : 0;
    return *this;
  }

  _u0ir_t& with_tie(bool enabled) {
    this->bits.u0tie = enabled ? 1 : 0;
    return *this;
  }

  void set_rie(bool enabled) volatile {
    this->bits.u0rie = enabled ? 1 : 0;
  }

  void set_tie(bool enabled) volatile {
    this->bits.u0tie = enabled ? 1 : 0;
  }

  void clr_tx_intr() volatile {
    this->bits.u0tif = 0;
  }

  void clr_rx_intr() volatile {
    this->bits.u0rif = 0;
  }

  void set(const _u0ir_t& u) volatile {
    this->as_uint8 = u.as_uint8;
  }
} u0ir_t;

enum class PRCR_UNLOCK_REG : uint8_t {
  LOCKED = 0,
  UNLOCKED = 1
};

typedef struct _prcr_t {
  unsigned int prc0:1;
  unsigned int prc1:1;
  unsigned int reserved0:1;
  unsigned int prc3:1;
  unsigned int prc4:1;
  unsigned int reserved1:3;
} prcr_t;

typedef union _u0rb_t {
  struct {
    unsigned int data:7;
    unsigned int reserved:5;
    unsigned int oer:1;
    unsigned int fer:1;
    unsigned int per:1;
    unsigned int sum:1;
  } b7;

  struct {
    unsigned int data:8;
    unsigned int reserved:4;
    unsigned int oer:1;
    unsigned int fer:1;
    unsigned int per:1;
    unsigned int sum:1;
  } b8;

  struct {
    unsigned int data:9;
    unsigned int reserved:3;
    unsigned int oer:1;
    unsigned int fer:1;
    unsigned int per:1;
    unsigned int sum:1;
  } b9;

  uint16_t as_uint16;

  _u0rb_t(uint16_t u16) {
    this->as_uint16 = u16;
  }

  bool is_ovr_err() volatile {
    return this->b8.oer == 1;
  }

  bool is_frm_err() volatile {
    return this->b8.fer == 1;
  }

  bool is_prty_err() volatile {
    return this->b8.per == 1;
  }

  bool is_sum_err() volatile {
    return this->b8.sum == 1;
  }

  uint8_t recv_b7() volatile {
    return this->b7.data;
  }

  uint8_t recv_b8() volatile {
    return this->b8.data;
  }

  uint16_t recv_b9() volatile {
    return this->b9.data;
  }
} u0rb_t;

typedef struct {
  unsigned int hocoe:1;
  unsigned int locodis:1;
  unsigned int reserved:6;

  void use_hs_osc(bool enabled) volatile {
    this->hocoe = enabled ? 1 : 0;
  }

  void use_ls_osc(bool enabled) volatile {
    this->locodis = enabled ? 0 : 1;
  }
} ococr_t;

enum class SCKCR_HSCKSEL : uint8_t {
  EXT_CLK_SRC = 0,
  ON_CHIP_CLK_SRC = 1
};

typedef struct {
  unsigned int phissel:3;
  unsigned int reserved0:2;
  unsigned int waitm:1;
  unsigned int hscksel:1;
  unsigned int reserved1:1;

  void set_clk_src(SCKCR_HSCKSEL sel) volatile {
    this->hscksel = (uint8_t)sel;
  }
} sckcr_t;

enum class CKSTPR_SCKSEL : uint8_t {
  LOW_SPEED = 0,
  HIGH_SPEED = 1
};

typedef struct {
  unsigned int stpm:1;
  unsigned int wckstp:1;
  unsigned int pscstp:1;
  unsigned int reserved:4;
  unsigned int scksel:1;

  void set_base_clk(CKSTPR_SCKSEL sel) volatile {
    this->scksel = (uint8_t)sel;
  }
} ckstpr_t;

enum class P14SEL : uint8_t {
  IO_AN4 = 0,
  TXD0 = 1,
  RXD0 = 2,
  INT0 = 3,
  TRCIOB = 4
};

enum class P15SEL : uint8_t {
  IO = 0,
  RXD0 = 1,
  TRJIO = 2,
  INT1 = 3,
  VCOUT1 = 4
};

enum class P16SEL : uint8_t {
  IO_IVREF1 = 0,
  CLK0 = 1,
  TRJO = 2,
  TRCIOB = 3
};

enum class P17SEL : uint8_t {
  IO_AN7_IVCMP1 = 0,
  INT1 = 1,
  TRJIO = 2,
  TRCCLK = 3
};

typedef struct {
  unsigned int p14sel:2;
  unsigned int p15sel:2;
  unsigned int p16sel:2;
  unsigned int p17sel:2;

  void assign_pin(P14SEL p14) volatile {
    this->p14sel = uint8_t(p14) & 3;
  }

  void assign_pin(P15SEL p15) volatile {
    this->p15sel = uint8_t(p15) & 3;
  }

  void assign_pin(P16SEL p16) volatile {
    this->p16sel = uint8_t(p16);
  }

  void assign_pin(P17SEL p17) volatile {
    this->p17sel = uint8_t(p17);
  }
} pmh1_t;

typedef struct {
  unsigned int p14sel:1;
  unsigned int reserved0:1;
  unsigned int p15sel:1;
  unsigned int reserved1:5;

  void assign_pin(P14SEL p14) volatile {
    this->p14sel = p14 == P14SEL::TRCIOB ? 1 : 0;
  }

  void assign_pin(P15SEL p15) volatile {
    this->p15sel = p15 == P15SEL::VCOUT1 ? 1 : 0;
  }
} pmh1e_t;

typedef union _mstcr_t {
  struct {
    unsigned int msttrj:1;
    unsigned int msttrb:1;
    unsigned int reserved0:2;
    unsigned int mstad:1;
    unsigned int msttrc:1;
    unsigned int mstuart:1;
    unsigned int reserved1:1;
  } bits;
  uint8_t as_uint8;
  
  _mstcr_t(uint8_t u = 0) {
    this->as_uint8 = u;
  }

  _mstcr_t& activating_timer_rj2(bool is_active) {
    this->bits.msttrj = is_active ? 0 : 1;
    return *this;
  }

  void activate_timer_rj2(bool is_active) volatile {
    this->bits.msttrj = is_active ? 0 : 1;
  }

  _mstcr_t& activating_timer_rb2(bool is_active) {
    this->bits.msttrb = is_active ? 0 : 1;
    return *this;
  }

  void activate_timer_rb2(bool is_active) volatile {
    this->bits.msttrb = is_active ? 0 : 1;
  }

  _mstcr_t& activating_ad(bool is_active) {
    this->bits.mstad = is_active ? 0 : 1;
    return *this;
  }

  void activate_ad(bool is_active) volatile {
    this->bits.mstad = is_active ? 0 : 1;
  }
    
  _mstcr_t& activating_time_rc(bool is_active) {
    this->bits.msttrc = is_active ? 0 : 1;
    return *this;
  }

  void activate_timer_rc(bool is_active) volatile {
    this->bits.msttrc = is_active ? 0 : 1;
  }

  _mstcr_t& activating_uart(bool is_active) {
    this->bits.mstuart = is_active ? 0 : 1;
    return *this;
  }

  void activate_uart(bool is_active) volatile {
    this->bits.mstuart = is_active ? 0 : 1;
  }
} mstcr_t;

typedef struct {
  struct { // 0x00010
    unsigned int reserved0:3;
    unsigned int srst:1;
    unsigned int reserved1:4;
  } pm0;

  uint8_t padding0; // 0x00011
  mstcr_t mstcr; // 0x00012
  prcr_t prcr; // 0x00013

  uint8_t padding1[12]; // 0x00014
  uint8_t padding2[1]; // 0x00020

  ococr_t ococr; // 0x00021
  sckcr_t sckcr; // 0x00022

  uint8_t padding3; // 0x00023

  ckstpr_t ckstpr; // 0x00024

  uint8_t padding4[11]; // 0x00025
  uint8_t padding5[16]; // 0x00030
  uint8_t padding6[16]; // 0x00040
  uint8_t padding7[16]; // 0x00050
  uint8_t padding8[16]; // 0x00060
  uint8_t padding9[16]; // 0x00070

  u0mr_t u0mr; // 0x00080
  uint8_t u0brg; // 0x00081
  uint8_t u0tbl; // 0x00082
  uint8_t u0tbh; // 0x00083

  u0c0_t u0c0; // 0x00084
  u0c1_t u0c1; // 0x00085
  u0rb_t u0rb; // 0x00086
  u0ir_t u0ir; // 0x00088

  uint8_t padding10[7]; // 0x00089
  uint8_t padding11[16]; // 0x00090
  uint8_t padding12[9]; // 0x000A0

  struct { // 0x000A9
    unsigned int b0:1;
    unsigned int b1:1;
    unsigned int b2:1;
    unsigned int b3:1;
    unsigned int b4:1;
    unsigned int b5:1;
    unsigned int b6:1;
    unsigned int b7:1;
  } pd1;

  uint8_t padding13[5];

  struct { // 0x000AF
    unsigned int b0:1;
    unsigned int b1:1;
    unsigned int b2:1;
    unsigned int b3:1;
    unsigned int b4:1;
    unsigned int b5:1;
    unsigned int b6:1;
    unsigned int b7:1;
  } p1;

  uint8_t padding14[16]; // 0x000B0
  uint8_t padding15[9]; // 0x000C0

  pmh1_t pmh1; // 0x000C9

  uint8_t padding16[6]; // 0x000CA
  uint8_t padding17[1]; // 0x000D0

  pmh1e_t pmh1e; // 0x000D1
} IO_t;
#pragma pack()

volatile IO_t *pIO = reinterpret_cast<volatile IO_t*>(0x10);

typedef utils::fifo<uint8_t, 16> TX_BUFF;  // 送信バッファ
typedef utils::fifo<uint8_t, 16> RX_BUFF;  // 受信バッファ

static TX_BUFF send_buf;
static RX_BUFF recv_buf;
static volatile bool send_stall;

static void isend() {
  using namespace device;

  if (send_buf.length()) {
    pIO->u0tbl = send_buf.get();
  } else {
    send_stall = true;
  }

  pIO->u0ir.clr_tx_intr();
}

static void irecv() {
  using namespace device;

  u0rb_t u0rb = u0rb_t(pIO->u0rb.as_uint16);
  ///< フレーミングエラー/パリティエラー状態確認
  if(u0rb.is_ovr_err() || u0rb.is_frm_err() || u0rb.is_prty_err() || u0rb.is_sum_err()) {
    pIO->u0c1.clr_err();
  } else {
    recv_buf.put(u0rb.recv_b8());
  }

  pIO->u0ir.clr_rx_intr();
}

extern "C" {
  void UART0_TX_intr(void) {
    isend();
  }

  void UART0_RX_intr(void) {
    irecv();
  }
};

static void sleep() {
  asm("nop");
}

static void resume_tx() {
  using namespace device;

  if (send_stall && send_buf.length() > 0) {
    while (! pIO->u0c1.is_tx_buf_empty()) sleep();
    uint8_t c = send_buf.get();
    send_stall = false;
    pIO->u0tbl = c;
  }
}

void uart_putc(uint8_t c) {
  if(send_buf.length() >= (send_buf.size() * 7 / 8)) {
    resume_tx();
    while(send_buf.length() != 0) {
      sleep();
    }
  }
  send_buf.put(c);
  resume_tx();
}

static void print(int i) {
  int n = i / 10000;
  uart_putc('0' + n);
  i -= (n * 10000);

  n = i / 1000;
  uart_putc('0' + n);
  i -= (n * 1000);

  n = i / 100;
  uart_putc('0' + n);
  i -= (n * 100);

  n = i / 10;
  uart_putc('0' + n);
  i -= (n * 10);

  uart_putc('0' + i);
  uart_putc('\r');
  uart_putc('\n');
}

void init_uart() {
  using namespace device;

  // UART の設定 (P1_4: TXD0[out], P1_5: RXD0[in])
  // ※シリアルライターでは、RXD 端子は、P1_6 となっているので注意！
  pIO->pmh1.assign_pin(P14SEL::TXD0);
  pIO->pmh1e.assign_pin(P14SEL::TXD0);

  pIO->pmh1.assign_pin(P15SEL::RXD0);
  pIO->pmh1e.assign_pin(P15SEL::RXD0);

  pIO->mstcr.activate_uart(true);

  pIO->u0c0.set_clk(U0C0_CLK::DIV1);
  pIO->u0brg = 10;

  pIO->u0mr.set(u0mr_t()
                .with_smd(U0MR_SMD::BIT_LEN8)
                .with_stps(U0MR_STPS::STOP_BIT_1));

  pIO->u0c1.set(u0c1_t().with_te(true).with_re(true));

  ILVL8.B45 = 1;
  ILVL9.B01 = 1;

  pIO->u0ir.set(u0ir_t().with_rie(true).with_tie(true));

  send_stall = true;
}

static void init_device() {
  using namespace device;

  // クロック関係レジスタ・プロテクト解除
  pIO->prcr.prc0 = (uint8_t)PRCR_UNLOCK_REG::UNLOCKED;

  // 高速オンチップオシレーターへ切り替え(20MHz)
  // ※ F_CLK を設定する事（Makefile内）
  pIO->ococr.use_hs_osc(true);
  pIO->sckcr.set_clk_src(SCKCR_HSCKSEL::ON_CHIP_CLK_SRC);
  pIO->ckstpr.set_base_clk(CKSTPR_SCKSEL::HIGH_SPEED);

  init_uart();
}

int main(int argc, char *argv[]) {
  using namespace device;

  init_device();

  while(1) {
    for (int i = 0; i < 100000; ++i) {
      print(i);
      utils::delay::milli_second(500);
    }
  }
}
