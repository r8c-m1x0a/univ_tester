#include <cstdio>

#include "fifo.hpp"
#include "common/vect.h"
#include "r8c-m1xa-io.h"
#include "clock.h"
#include "buzz.h"

#define AUTO_POWER_OFF_MILLIS (int32_t(10) * 60 * 1000)

Clock<InternalClock20M> clock(InternalClock20M {
  SCKCR_PHISSEL::DIV_1
});

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
  if (u0rb.b8.is_ovr_err || u0rb.b8.is_frm_err || u0rb.b8.is_prity_err || u0rb.b8.is_sum_err) {
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

static void resume_tx() {
  if (send_stall && send_buf.length() > 0) {
    while (! io.u0c1.bits.is_tx_buf_empty) {
      asm("nop");
    }
    uint8_t c = send_buf.get();
    send_stall = false;
    io.u0tbl = c;
  }
}

void uart_putc(uint8_t c) {
  if(send_buf.length() >= (send_buf.size() * 7 / 8)) {
    resume_tx();
    while(send_buf.length() != 0) {
      asm("nop");
    }
  }
  send_buf.put(c);
  resume_tx();
}

static void print_uint16(uint16_t i) {
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
}

/*
static void print(uint16_t p, uint16_t m) {
  print_uint16(p);
  uart_putc('/');
  print_uint16(m);
  uart_putc('\r');
  uart_putc('\n');
}
*/

void init_uart() {
  // UART の設定 (P1_4: TXD0[out], P1_5: RXD0[in])
  // ※シリアルライターでは、RXD 端子は、P1_6 となっているので注意！
  io.pm1.bits.b4_func = PM1_B4_FUNC::TXD0;
  io.pmh1e.bits.is_b4_trciob = false;

  io.pm1.bits.b5_func = PM1_B5_FUNC::RXD0;
  io.pmh1e.bits.is_b5_vcout1 = false;

  io.mstcr.bits.is_uart_standby = false;

  io.u0c0.bits.clk_div = U0C0_CLK::DIV1;
  io.u0brg = 10;

  io.u0mr.set(u0mr_t()
                .with_smd(U0MR_SMD::BIT_LEN8)
                .with_stps(U0MR_STPS::STOP_BIT_1));

  io.u0c1.set(u0c1_t().with_tx_enabled(true).with_rx_enabled(true));

  io.ilvl8.bits.uart_tx = ITR_LEVEL::LEVEL_1;
  io.ilvl9.bits.uart_rx = ITR_LEVEL::LEVEL_1;

  io.u0ir.set(u0ir_t().with_rx_itr_enabled(true).with_tx_itr_enabled(true));

  io.pm3.bits.b7_func = PM3_B7_FUNC::TRCIOD;
  io.mstcr.bits.is_tmr_rc_standby = false;
  io.trcmr.bits.is_count_started = true;

  send_stall = true;
}

// https://www.renesas.com/jp/ja/document/mah/r8cm11a-group-r8cm12a-group-users-manual-hardware?r=1054126
// 15.3 Timer RC Operation
//
// TimerA, B, Cをdisable(I/O port)
// TimerDをPWM

// TRCOER:
//   EA:
//   EB: 1
//   EC:
//   ED: 0
//   PTO:
// 
// TRCMR:
//   PWMB: 0
//   PWMC: 0
//   PWMD: 1
//   PWM2: 1
//   BUFEA:
//   BUFEB:
//   CTS: 1で開始。0で停止
// 
// TRCIOR0:
//   IOA0: 0
//   IOA1: 0
//   IOA2: 0
//   IOB0: 0
//   IOB1: 0
//   IOB2: 0
// 
// TRCIOR1:
//   IOC0: 0
//   IOC1: 0
//   IOC2: 0
//   IOC3:
//   IOD0:
//   IOD1:
//   IOD2:
//   IOD3:

// System clock = 20Mhz
// 
// TRCCR1: f/32
//   CKS0: 0
//   CKS1: 0
//   CKS2: 1
//   CCLR: 1 // TRCGRAの値がカウンタに一致したらクリアする
//
// TRCGRA: 65535
// TRCGRD: TRCGRA/2 = 32767 (duty 50%)
//
// 20 * 1000 * 1000 / 32 / 65536 = 9.5Hz

// A/D Converter
// MSTCR:
//   MSTAD: 0
// ADCON0:
//   ADST: 0=stop, 1=start 設定中は0にする。初期値0
// ADMOD:
//   CKS: f1
// ADINSEL:
//   CH0: 1
//   ADGSEL0: 0
//   ADGSEL1: 0
//
// 変換開始:
// ADCON0:
//   ADST = 1
// 終了待ち:
//   ADST = 0
// 結果読み出し:
//   AD1

static void init_device() {
  clock.init(&io);
  init_uart();

  io.pd1.set(
    io.pd1.clone()
      .with_p1_7(PD_DIR::OUT)
      .with_p1_2(PD_DIR::OUT)
      .with_p1_3(PD_DIR::OUT)
  );

  io.pd4.set(
    io.pd4.clone()
      .with_p4_6(PD_DIR::OUT)
      .with_p4_7(PD_DIR::OUT)
  );

  io.trcoer.set(
    io.trcoer.clone().with_trciob(TRCOER_E::DISABLED_OR_HIGH_IMP).with_trciod(TRCOER_E::ENABLED)
  );
  io.trcmr.set(
    io.trcmr.clone()
      .with_trciob(TRCMR_MODE::TIMER).with_trcioc(TRCMR_MODE::TIMER)
      .with_trciod(TRCMR_MODE::PWM).with_pwm2(TRCMR_MODE2::TIMER_OR_PWM)
  );
  io.trcior0.set(
    io.trcior0.clone()
      .with_trcgra_ctrl(TRCIOR0_CTRL::OUT_COMP_DISABLED)
      .with_trcgrb_ctrl(TRCIOR0_CTRL::OUT_COMP_DISABLED)
  );
  io.trcior1.bits.trcgrc_ctrl = TRCIOR1_TRCGRC_CTRL::OUT_COMP_TRCIOA_DISABLED;
  io.trccr1.bits.trccnt_clear_mode = TRCCR1_CLEAR_MODE::CLEAR;
  io.trcmr.bits.is_count_started = false;

  // A/D
  io.mstcr.bits.is_ad_standby = false;
  io.admod.bits.cks = ADMOD_CKS::F1;
  io.adinsel.set(adinsel_t().with_ch0(1).with_adgsel(ADINSEL_ADGSEL::AN0_1));
}

static void set_output(bool plus) {
  io.p1.set(
    io.p1.clone().with_b2(plus).with_b3(! plus)
  );
}

uint16_t ad() {
  io.adcon0.ad_starts = true;
  while (io.adcon0.ad_starts) {
    asm("nop");
  }

  return io.ad1;
}

static inline bool is_on(uint16_t v) {
  return v < 800;
}

static void disp(uint16_t pv, uint16_t mv) {
  io.p4.bits.b6 = is_on(pv);
  io.p4.bits.b7 = is_on(mv);
}

static uint32_t current_count() {
  uint32_t c = io.trcgra;
  switch (io.trccr1.bits.source) {
    case TRCCR1_SOURCE::F2:
    c *= 2;

    case TRCCR1_SOURCE::F4:
    c *= 4;

    case TRCCR1_SOURCE::F8:
    c *= 8;

    case TRCCR1_SOURCE::F32:
    c *= 32;

    default:
    break;
  }

  return c;
}

static uint16_t changed_percent(uint32_t before, uint32_t after) {
  uint32_t diff = before < after ? after - before : before - after;
  return diff * 100 / before;
}

static void buzz(uint16_t v) {
  if (is_on(v)) {
    if (v < 300) v = 300;
    v -= 300;  // 0 <= v < 500

    uint32_t new_cnt = to_count(v);
    uint32_t cur_cnt = current_count();
    uint32_t diff_percent = changed_percent(new_cnt, cur_cnt);

    if (0 < diff_percent) {
      io.trcmr.bits.is_count_started = false;

      io.trccr1.bits.source = TRCCR1_SOURCE::F1;
      if (UINT16_MAX < new_cnt) {
        io.trccr1.bits.source = TRCCR1_SOURCE::F2;
        new_cnt /= 2;
      }

      if (UINT16_MAX < new_cnt) {
        io.trccr1.bits.source = TRCCR1_SOURCE::F4;
        new_cnt /= 2;
      }

      if (UINT16_MAX < new_cnt) {
        io.trccr1.bits.source = TRCCR1_SOURCE::F8;
        new_cnt /= 2;
      }

      if (UINT16_MAX < new_cnt) {
        io.trccr1.bits.source = TRCCR1_SOURCE::F32;
        new_cnt /= 4;
      }

      v = uint16_t(new_cnt);
      print_uint16(v);
      uart_putc('\r');
      uart_putc('\n');
      io.trcgra = v;
      io.trcgrd = v / 2;

      io.trcmr.bits.is_count_started = true;
    }
  } else {
    io.trcmr.bits.is_count_started = false;
  }
}

static inline uint16_t min(uint16_t u0, uint16_t u1) {
  return (u0 < u1) ? u0 : u1;
}

static void power_off() {
  io.p1.bits.b7 = false;
}

int main(int argc, char *argv[]) {
  init_device();
  io.p1.bits.b7 = true;

  io.p4.bits.b6 = true;
  io.p4.bits.b7 = true;

  int32_t auto_power_off_timer_millis = AUTO_POWER_OFF_MILLIS;

  while (1) {
    set_output(true);
    clock.busy_wait_ms(10);
    uint16_t plus_voltage = ad();

    set_output(false);
    clock.busy_wait_ms(10);
    uint16_t minus_voltage = ad();
//    print(plus_voltage, minus_voltage);

    auto_power_off_timer_millis -= 20;
    if (is_on(plus_voltage) || is_on(minus_voltage))
      auto_power_off_timer_millis = AUTO_POWER_OFF_MILLIS;

    if (auto_power_off_timer_millis < 0)
      power_off();

    disp(plus_voltage, minus_voltage);
    buzz(min(plus_voltage, minus_voltage));
  }
}
