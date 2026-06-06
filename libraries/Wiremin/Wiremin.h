/*
 * Wiremin.h — Minimal I2C driver for CH32V003
 *
 * Header-only. No dynamic allocation. No std::function. No Stream.
 * Direct I2C1 register access. Default pins: PC1=SDA, PC2=SCL.
 *
 * Include this file in ONE .ino/.cpp only (ISR handlers are non-static).
 * Do NOT use together with Wire.h (both define I2C1_EV/ER_IRQHandler).
 *
 * ── Master API ──────────────────────────────────────────────────────────────
 *   Wiremin_begin()                          — init I2C1 at 100 kHz (master)
 *   Wiremin_begin_fast()                     — init I2C1 at 400 kHz (master)
 *   Wiremin_begin_hz(hz)                     — init I2C1 at custom Hz (master)
 *   Wiremin_end()                            — de-init I2C1
 *   Wiremin_write(addr7, data, len)          — send bytes to device
 *   Wiremin_read(addr7, buf, len)            — receive bytes from device
 *   Wiremin_write_reg(addr7, reg, data, len) — write starting at register
 *   Wiremin_read_reg(addr7, reg, buf, len)   — read starting at register (repeated START)
 *   Wiremin_probe(addr7)                     — true if device ACKs (for scanning)
 *
 * ── Slave API ───────────────────────────────────────────────────────────────
 *   Wiremin_slave_begin(addr7)               — init I2C1 as slave at given address
 *   Wiremin_slave_set(reg, val)              — write to shared register (from TinyVM / main)
 *   Wiremin_slave_get(reg)                   — read from shared register
 *
 *   Shared registers are updated automatically by ISR when the master
 *   reads or writes this device.  Transaction formats:
 *     Master writes:  START → addr+W → reg_idx → [value …] → STOP
 *     Master reads:   START → addr+W → reg_idx → rSTART → addr+R → [data …] → NACK+STOP
 */
#pragma once
#include <Arduino.h>

// ── Configuration ─────────────────────────────────────────────────────────────
// APB1 clock in Hz (CH32V003 has no APB1 prescaler → equals F_CPU).
#ifndef WIREMIN_PCLK1_HZ
  #ifdef F_CPU
    #define WIREMIN_PCLK1_HZ  ((uint32_t)F_CPU)
  #else
    #define WIREMIN_PCLK1_HZ  48000000UL
  #endif
#endif

// Poll-loop limit before declaring a timeout (master mode).
#ifndef WIREMIN_TIMEOUT
  #define WIREMIN_TIMEOUT  50000UL
#endif

// Number of slave shared registers (1–128, keep ≤ RAM budget).
#ifndef WIREMIN_SLAVE_REGS
  #define WIREMIN_SLAVE_REGS  16
#endif

// ── Master internal state ────────────────────────────────────────────────────
static uint16_t _wm_ckcfgr; // saved for SWRST recovery
static uint8_t  _wm_ctlr2;  // FREQ field, saved for SWRST recovery

// ── Slave internal state ─────────────────────────────────────────────────────
static volatile uint8_t _wm_slave_regs[WIREMIN_SLAVE_REGS]; // shared register array
static volatile uint8_t _wm_slave_reg_ptr;  // current register index (set by master)
static volatile uint8_t _wm_slave_rx_first; // 1 = next received byte is register index

// ── Master helpers ────────────────────────────────────────────────────────────

static bool _wm_wait_s1(uint16_t bit) {
  for (uint32_t t = WIREMIN_TIMEOUT; t; --t)
    if (I2C1->STAR1 & bit) return true;
  return false;
}

static bool _wm_wait_idle(void) {
  for (uint32_t t = WIREMIN_TIMEOUT; t; --t)
    if (!(I2C1->STAR2 & 2u)) return true;
  // SWRST recovery
  I2C1->CTLR1  = (1u<<15);
  I2C1->CTLR1  = 0;
  I2C1->CTLR2  = _wm_ctlr2;
  I2C1->CKCFGR = _wm_ckcfgr;
  I2C1->CTLR1  = (1u<<10) | 1u;
  return false;
}

// ── GPIO / clock shared setup ─────────────────────────────────────────────────

static void _wm_hw_init(void) {
  RCC->APB2PCENR |= (1u<<4);   // GPIOC clock
  RCC->APB1PCENR |= (1u<<21);  // I2C1 clock
  // PC1(SDA) and PC2(SCL): AF open-drain 50 MHz → CNF=11, MODE=11 → 0xF per pin
  GPIOC->CFGLR = (GPIOC->CFGLR & ~((0xFu<<4)|(0xFu<<8)))
               | (0xFu<<4) | (0xFu<<8);
  // Reset I2C1
  RCC->APB1PRSTR |=  (1u<<21);
  RCC->APB1PRSTR &= ~(1u<<21);
}

// ── Master initialization ─────────────────────────────────────────────────────

static void Wiremin_begin_hz(uint32_t hz) {
  _wm_hw_init();

  uint8_t  freq_mhz = (uint8_t)(WIREMIN_PCLK1_HZ / 1000000UL);
  uint16_t ccr;

  if (hz <= 100000UL) {
    ccr = (uint16_t)(WIREMIN_PCLK1_HZ / (2UL * hz));
    if (ccr < 4u) ccr = 4u;
    _wm_ckcfgr = ccr;
  } else {
    ccr = (uint16_t)(WIREMIN_PCLK1_HZ / (3UL * hz));
    if (ccr < 1u) ccr = 1u;
    _wm_ckcfgr = (uint16_t)(0x8000u | ccr);
  }
  _wm_ctlr2 = freq_mhz;

  I2C1->CTLR1  = 0;
  I2C1->CTLR2  = freq_mhz;
  I2C1->CKCFGR = _wm_ckcfgr;
  I2C1->CTLR1  = (1u<<10) | 1u; // ACK=1, PE=1
}

static inline void Wiremin_begin(void)      { Wiremin_begin_hz(100000UL); }
static inline void Wiremin_begin_fast(void) { Wiremin_begin_hz(400000UL); }

static void Wiremin_end(void) {
  NVIC_DisableIRQ(I2C1_EV_IRQn);
  NVIC_DisableIRQ(I2C1_ER_IRQn);
  I2C1->CTLR1 = 0;
  RCC->APB1PRSTR |=  (1u<<21);
  RCC->APB1PRSTR &= ~(1u<<21);
  RCC->APB1PCENR &= ~(1u<<21);
  // Restore PC1 and PC2 to input-floating (CNF=01, MODE=00 → 0x4)
  GPIOC->CFGLR = (GPIOC->CFGLR & ~((0xFu<<4)|(0xFu<<8)))
               | (0x4u<<4) | (0x4u<<8);
}

// ── Slave initialization ──────────────────────────────────────────────────────

static void Wiremin_slave_begin(uint8_t addr7) {
  _wm_hw_init();

  uint8_t freq_mhz = (uint8_t)(WIREMIN_PCLK1_HZ / 1000000UL);
  // Use 100 kHz timing for slave (clock speed is driven by master)
  uint16_t ccr = (uint16_t)(WIREMIN_PCLK1_HZ / (2UL * 100000UL));
  if (ccr < 4u) ccr = 4u;

  I2C1->CTLR1  = 0;
  I2C1->CTLR2  = freq_mhz;
  I2C1->CKCFGR = ccr;
  // Own address (7-bit mode): bits [7:1], bit 14 always 1 per RM
  I2C1->OADDR1 = (1u<<14) | (addr7 << 1);
  I2C1->CTLR1  = (1u<<10) | 1u; // ACK=1, PE=1

  // Enable event + error interrupts (ITBUFEN=0 to avoid TXE storm in slave RX mode)
  I2C1->CTLR2 |= (1u<<9) | (1u<<8); // ITEVTEN=1, ITERREN=1

  // ISR attribute is plain __attribute__((interrupt)) — NOT WCH-Interrupt-fast.
  // Using fast mode preempts the rv003usb USB handler (MIE=0 during USB bit-sampling)
  // and corrupts USB HID packets. Standard interrupt respects MIE. (See twi.c comment.)
  NVIC_EnableIRQ(I2C1_EV_IRQn);
  NVIC_EnableIRQ(I2C1_ER_IRQn);

  // Init shared state
  for (uint8_t i = 0; i < WIREMIN_SLAVE_REGS; i++) _wm_slave_regs[i] = 0;
  _wm_slave_reg_ptr  = 0;
  _wm_slave_rx_first = 1;
}

// Shared register access (safe for single-byte reads/writes on RISC-V)
static inline void    Wiremin_slave_set(uint8_t reg, uint8_t val) {
  if (reg < WIREMIN_SLAVE_REGS) _wm_slave_regs[reg] = val;
}
static inline uint8_t Wiremin_slave_get(uint8_t reg) {
  return (reg < WIREMIN_SLAVE_REGS) ? _wm_slave_regs[reg] : 0;
}

// ── Slave ISR ─────────────────────────────────────────────────────────────────
//
// ITBUFEN=0: only ADDR, BTF, STOPF events trigger I2C1_EV_IRQHandler.
// (TXE/RXNE fire silently; BTF and STOPF are used to read/write DR.)
//
// Write transaction (master → slave):
//   ADDR(W) → [BTF with 2+ bytes pending] → STOPF (last byte in DR)
//
// Read transaction (master ← slave):
//   ADDR(W) → rSTART → ADDR(R) [with RXNE from reg_idx] → BTF per extra byte → AF (NACK)

extern "C" __attribute__((interrupt)) void I2C1_EV_IRQHandler(void) {
  uint16_t s1 = I2C1->STAR1;

  // ── ADDR: device was addressed ────────────────────────────────────────────
  if (s1 & (1u<<1)) {
    uint16_t s2 = I2C1->STAR2; // reading STAR2 clears ADDR flag

    if (s2 & (1u<<2)) {
      // TRA=1: master is reading from us (transmit mode)
      // There may be a pending received byte in DR from the preceding write phase
      // (e.g., register index sent before the repeated START).
      if (s1 & (1u<<6)) { // RXNE
        uint8_t d = (uint8_t)I2C1->DATAR;
        if (_wm_slave_rx_first) {
          _wm_slave_reg_ptr  = d;
          _wm_slave_rx_first = 0;
        }
        // If !rx_first: stale byte, discard (reg_ptr was already set by BTF handler)
      }
      // Send first response byte; reg_ptr auto-increments for burst reads
      I2C1->DATAR = (_wm_slave_reg_ptr < WIREMIN_SLAVE_REGS)
                    ? _wm_slave_regs[_wm_slave_reg_ptr++] : 0;
    } else {
      // TRA=0: master is writing to us (receive mode)
      _wm_slave_rx_first = 1;
    }
    return;
  }

  // ── BTF: byte transfer finished ───────────────────────────────────────────
  if (s1 & (1u<<2)) {
    if (I2C1->STAR2 & (1u<<2)) {
      // Transmit mode: DR is empty (byte was sent, master ACKed) → send next byte
      I2C1->DATAR = (_wm_slave_reg_ptr < WIREMIN_SLAVE_REGS)
                    ? _wm_slave_regs[_wm_slave_reg_ptr++] : 0;
    } else {
      // Receive mode: DR has a received byte (clock is being stretched)
      uint8_t d = (uint8_t)I2C1->DATAR; // reading DR clears BTF
      if (_wm_slave_rx_first) {
        _wm_slave_reg_ptr  = d;          // first byte = register index
        _wm_slave_rx_first = 0;
      } else {
        if (_wm_slave_reg_ptr < WIREMIN_SLAVE_REGS)
          _wm_slave_regs[_wm_slave_reg_ptr++] = d;
      }
    }
    return;
  }

  // ── STOPF: stop condition detected ───────────────────────────────────────
  if (s1 & (1u<<4)) {
    // Consume any byte still in DR (e.g., single-byte write or final byte of multi-write)
    if (s1 & (1u<<6)) { // RXNE
      uint8_t d = (uint8_t)I2C1->DATAR;
      if (_wm_slave_rx_first) {
        _wm_slave_reg_ptr  = d;
        _wm_slave_rx_first = 0;
      } else {
        if (_wm_slave_reg_ptr < WIREMIN_SLAVE_REGS)
          _wm_slave_regs[_wm_slave_reg_ptr++] = d;
      }
    }
    // Clear STOPF: write any value to CTLR1 after reading STAR1 (done above)
    I2C1->CTLR1 |= (1u<<10); // re-enable ACK (also satisfies the STOPF clear sequence)
    _wm_slave_rx_first = 1;   // ready for next transaction
    return;
  }
}

extern "C" __attribute__((interrupt)) void I2C1_ER_IRQHandler(void) {
  uint16_t s1 = I2C1->STAR1;
  // AF (bit 10): NACK from master signals end of read transaction — clear it
  if (s1 & (1u<<10))
    I2C1->STAR1 = s1 & ~(1u<<10);
  (void)I2C1->STAR2; // clear remaining error flags
}

// ── Master write ──────────────────────────────────────────────────────────────

static bool Wiremin_write(uint8_t addr7, const uint8_t *data, uint8_t len) {
  if (!_wm_wait_idle()) return false;

  I2C1->CTLR1 |= (1u<<8); // START
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; } // SB

  I2C1->DATAR = addr7 << 1; // write direction
  if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; } // ADDR
  (void)I2C1->STAR1; (void)I2C1->STAR2; // clear ADDR

  for (uint8_t i = 0; i < len; i++) {
    if (!_wm_wait_s1(1u<<7)) { I2C1->CTLR1 |= (1u<<9); return false; } // TXE
    I2C1->DATAR = data[i];
  }
  if (len) {
    if (!_wm_wait_s1(1u<<2)) { I2C1->CTLR1 |= (1u<<9); return false; } // BTF
  }
  I2C1->CTLR1 |= (1u<<9); // STOP
  return true;
}

// ── Probe (I2C address scan) ──────────────────────────────────────────────────

static bool Wiremin_probe(uint8_t addr7) {
  if (!_wm_wait_idle()) return false;

  I2C1->CTLR1 |= (1u<<8);
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; }

  I2C1->DATAR = addr7 << 1;
  bool ack = _wm_wait_s1(1u<<1);
  (void)I2C1->STAR1;
  if (ack) (void)I2C1->STAR2;
  I2C1->CTLR1 |= (1u<<9); // STOP
  return ack;
}

// ── Master read ───────────────────────────────────────────────────────────────

static bool Wiremin_read(uint8_t addr7, uint8_t *buf, uint8_t len) {
  if (!len) return true;
  if (!_wm_wait_idle()) return false;

  I2C1->CTLR1 |= (1u<<10) | (1u<<8); // ACK=1, START
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; }

  I2C1->DATAR = (addr7 << 1) | 1u; // read direction

  if (len == 1) {
    I2C1->CTLR1 &= ~(1u<<10); // ACK=0 before ADDR clear
    if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
    (void)I2C1->STAR1; (void)I2C1->STAR2;
    I2C1->CTLR1 |= (1u<<9); // STOP
    if (!_wm_wait_s1(1u<<6)) return false; // RXNE
    buf[0] = (uint8_t)I2C1->DATAR;
    return true;
  }

  if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
  (void)I2C1->STAR1; (void)I2C1->STAR2;

  for (uint8_t i = 0; i < len; i++) {
    if (i == len - 1) {
      I2C1->CTLR1 &= ~(1u<<10); // ACK=0 for last byte
      I2C1->CTLR1 |= (1u<<9);   // STOP
    }
    if (!_wm_wait_s1(1u<<6)) return false; // RXNE
    buf[i] = (uint8_t)I2C1->DATAR;
  }
  return true;
}

// ── Register helpers (master) ─────────────────────────────────────────────────

// Write: START → addr+W → reg → data[0..len-1] → STOP
static bool Wiremin_write_reg(uint8_t addr7, uint8_t reg, const uint8_t *data, uint8_t len) {
  if (!_wm_wait_idle()) return false;

  I2C1->CTLR1 |= (1u<<8);
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; }
  I2C1->DATAR = addr7 << 1;
  if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
  (void)I2C1->STAR1; (void)I2C1->STAR2;

  if (!_wm_wait_s1(1u<<7)) { I2C1->CTLR1 |= (1u<<9); return false; } // TXE
  I2C1->DATAR = reg;
  for (uint8_t i = 0; i < len; i++) {
    if (!_wm_wait_s1(1u<<7)) { I2C1->CTLR1 |= (1u<<9); return false; }
    I2C1->DATAR = data[i];
  }
  if (!_wm_wait_s1(1u<<2)) { I2C1->CTLR1 |= (1u<<9); return false; } // BTF
  I2C1->CTLR1 |= (1u<<9); // STOP
  return true;
}

// Read with repeated START: START → addr+W → reg → rSTART → addr+R → buf[0..len-1] → STOP
static bool Wiremin_read_reg(uint8_t addr7, uint8_t reg, uint8_t *buf, uint8_t len) {
  if (!len) return false;
  if (!_wm_wait_idle()) return false;

  // Write phase: send register address (no STOP)
  I2C1->CTLR1 |= (1u<<8);
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; }
  I2C1->DATAR = addr7 << 1;
  if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
  (void)I2C1->STAR1; (void)I2C1->STAR2;
  if (!_wm_wait_s1(1u<<7)) { I2C1->CTLR1 |= (1u<<9); return false; } // TXE
  I2C1->DATAR = reg;
  if (!_wm_wait_s1(1u<<2)) { I2C1->CTLR1 |= (1u<<9); return false; } // BTF

  // Repeated START → read phase
  I2C1->CTLR1 |= (1u<<10) | (1u<<8); // ACK=1, START
  if (!_wm_wait_s1(1u<<0)) { I2C1->CTLR1 |= (1u<<9); return false; }
  I2C1->DATAR = (addr7 << 1) | 1u;

  if (len == 1) {
    I2C1->CTLR1 &= ~(1u<<10);
    if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
    (void)I2C1->STAR1; (void)I2C1->STAR2;
    I2C1->CTLR1 |= (1u<<9);
    if (!_wm_wait_s1(1u<<6)) return false;
    buf[0] = (uint8_t)I2C1->DATAR;
    return true;
  }

  if (!_wm_wait_s1(1u<<1)) { (void)I2C1->STAR1; I2C1->CTLR1 |= (1u<<9); return false; }
  (void)I2C1->STAR1; (void)I2C1->STAR2;

  for (uint8_t i = 0; i < len; i++) {
    if (i == len - 1) {
      I2C1->CTLR1 &= ~(1u<<10);
      I2C1->CTLR1 |= (1u<<9);
    }
    if (!_wm_wait_s1(1u<<6)) return false;
    buf[i] = (uint8_t)I2C1->DATAR;
  }
  return true;
}
