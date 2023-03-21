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

#include "fifo.hpp"
#include "delay.hpp"
#include "common/vect.h"
#include "r8c-m1xa-io.h"

typedef utils::fifo<uint8_t, 16> TX_BUFF;  // 送信バッファ
typedef utils::fifo<uint8_t, 16> RX_BUFF;  // 受信バッファ

static TX_BUFF send_buf;
static RX_BUFF recv_buf;
static volatile bool send_stall;

static void isend() {
  if (send_buf.length()) {
    io.u0tbl = send_buf.get();
  } else {
    send_stall = true;
  }

  io.u0ir.bits.is_tx_itr_requested = false;
}

static void irecv() {
  u0rb_t u0rb = io.u0rb.clone();
  ///< フレーミングエラー/パリティエラー状態確認
  if(u0rb.b8.is_ovr_err || u0rb.b8.is_frm_err || u0rb.b8.is_prity_err || u0rb.b8.is_sum_err) {
    io.u0c1.clr_err();
  } else {
    recv_buf.put(u0rb.recv_b8());
  }

  io.u0ir.bits.is_rx_itr_requested = false;
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
  if (send_stall && send_buf.length() > 0) {
    while (! io.u0c1.bits.is_tx_buf_empty) sleep();
    uint8_t c = send_buf.get();
    send_stall = false;
    io.u0tbl = c;
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
  // UART の設定 (P1_4: TXD0[out], P1_5: RXD0[in])
  // ※シリアルライターでは、RXD 端子は、P1_6 となっているので注意！
  io.pm1.set_b4_func(PM1_B4_FUNC::TXD0);
  io.pmh1e.bits.is_b4_trciob = false;

  io.pm1.set_b5_func(PM1_B5_FUNC::RXD0);
  io.pmh1e.bits.is_b5_vcout1 = false;

  io.mstcr.bits.is_uart_standby = false;

  io.u0c0.set_clk_div(U0C0_CLK::DIV1);
  io.u0brg = 10;

  io.u0mr.set(u0mr_t()
                .with_smd(U0MR_SMD::BIT_LEN8)
                .with_stps(U0MR_STPS::STOP_BIT_1));

  io.u0c1.set(u0c1_t().with_tx_enabled(true).with_rx_enabled(true));

  io.ilvl8.set_uart_tx(ITR_LEVEL::LEVEL_1);
  io.ilvl9.set_uart_rx(ITR_LEVEL::LEVEL_1);

  io.u0ir.set(u0ir_t().with_rx_itr_enabled(true).with_tx_itr_enabled(true));

  send_stall = true;
}

static void init_device() {
  // クロック関係レジスタ・プロテクト解除
  io.prcr.bits.p0_enabled = true;

  // 高速オンチップオシレーターへ切り替え(20MHz)
  // ※ F_CLK を設定する事（Makefile内）
  io.ococr.bits.is_hosc_enabled = true;
  io.sckcr.set_clk_src(SCKCR_HSCKSEL::ON_CHIP_CLK_SRC);
  io.ckstpr.set_base_clk(CKSTPR_SCKSEL::HIGH_SPEED);

  init_uart();
}

int main(int argc, char *argv[]) {
  init_device();

  while(1) {
    for (int i = 0; i < 100000; ++i) {
      print(i);
      utils::delay::milli_second(500);
    }
  }
}
