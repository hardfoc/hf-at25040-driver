/**
 * @file mock_at25040.hpp
 * @brief A model of the AT25040B on the SPI bus, for the host tests.
 *
 * Follows the datasheet (Atmel-8707F): opcodes 0000 A011 / 0000 A010 with A8 in bit 3, WEL set by
 * WREN and cleared at the end of a write cycle, 8-byte pages that wrap inside the page, reads that
 * wrap at the end of the array, and a status register that reads 0xFF during the internal write
 * cycle. Time is modelled: every transfer takes len * 8 SCK periods, MockAt25040Timed adds the
 * optional delay_us() hook, and a write cycle lasts write_cycle_us (tWC, 5 ms max in Table 4-3).
 */
#pragma once

#include "at25040.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

class MockAt25040 : public at25040::SpiInterface<MockAt25040> {
public:
  enum class Bus { Chip, Floating, Loopback, Broken };

  std::array<uint8_t, 512> mem{};
  uint8_t bp = 0; ///< BP1:BP0, non-volatile
  bool wel = false;
  uint32_t sck_hz = 1000000;      ///< bus clock: sets how long each transfer takes
  uint32_t write_cycle_us = 3000; ///< length of an internal write cycle (tWC)
  uint64_t now_ns = 0;            ///< model time
  uint64_t busy_until_ns = 0;     ///< end of the current write cycle, 0 when idle
  Bus bus = Bus::Chip;
  std::size_t transfers = 0;
  std::size_t max_len = 0; ///< longest transfer seen
  std::size_t writes = 0;  ///< write cycles started (WRITE and WRSR)
  std::size_t ignored = 0; ///< instructions other than RDSR sent during a write cycle
  uint8_t last_opcode = 0;

  MockAt25040() {
    mem.fill(0xFF);
  }

  /** True while an internal write cycle runs. */
  bool busy() noexcept {
    if (busy_until_ns != 0 && now_ns >= busy_until_ns) {
      busy_until_ns = 0;
      wel = false; // the chip returns to the write-disable state at the end of a write cycle
    }
    return busy_until_ns != 0;
  }

  /** Start a write cycle as a WRITE or WRSR would (also: one left running by a reset MCU). */
  void startWriteCycle() noexcept {
    busy_until_ns = now_ns + static_cast<uint64_t>(write_cycle_us) * 1000u;
    ++writes;
  }

  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t /*timeout_ms*/) noexcept {
    ++transfers;
    max_len = len > max_len ? len : max_len;
    const bool was_busy = busy();
    now_ns += static_cast<uint64_t>(len) * 8u * 1000000000u / sck_hz;
    if (bus == Bus::Broken) {
      return false;
    }
    for (std::size_t i = 0; i < len; ++i) {
      rx[i] = (bus == Bus::Loopback) ? (i == 0 ? 0xFF : tx[i - 1]) : 0xFF; // echo one byte late
    }
    if (bus != Bus::Chip || len == 0) {
      return true;
    }
    last_opcode = tx[0];
    const uint8_t op = static_cast<uint8_t>(tx[0] & 0xF7u); // bit 3: X or A8
    if (was_busy) { // only RDSR works during a write cycle, and every status bit reads 1
      if (op == at25040::kCmdRdsr && len >= 2) {
        rx[1] = 0xFF;
      } else {
        ++ignored;
      }
      return true;
    }
    const uint16_t a8 = (tx[0] & at25040::kAddrA8Mask) ? 0x100u : 0u;
    switch (op) {
    case at25040::kCmdWren:
      wel = true;
      break;
    case at25040::kCmdWrdi:
      wel = false;
      break;
    case at25040::kCmdRdsr:
      if (len >= 2) {
        rx[1] = static_cast<uint8_t>((bp << 2) | (wel ? at25040::kSrWel : 0u));
      }
      break;
    case at25040::kCmdWrsr:
      if (wel && len >= 2) {
        bp = static_cast<uint8_t>((tx[1] >> 2) & 0x3u);
        startWriteCycle();
      }
      break;
    case at25040::kCmdRead:
      if (len >= 2) {
        uint16_t addr = static_cast<uint16_t>(a8 | tx[1]);
        for (std::size_t i = 2; i < len; ++i) {
          rx[i] = mem[addr];
          addr = static_cast<uint16_t>((addr + 1u) % mem.size());
        }
      }
      break;
    case at25040::kCmdWrite:
      if (wel && len >= 3) {
        const uint16_t addr = static_cast<uint16_t>(a8 | tx[1]);
        const uint16_t page = static_cast<uint16_t>(addr & ~0x7u);
        for (std::size_t i = 2; i < len; ++i) {
          const uint16_t a = static_cast<uint16_t>(page | ((addr + (i - 2)) & 0x7u));
          if (!isProtected(a)) {
            mem[a] = tx[i];
          }
        }
        startWriteCycle();
      }
      break;
    default:
      break;
    }
    return true;
  }

private:
  bool isProtected(uint16_t a) const noexcept {
    switch (bp) {
    case 1:
      return a >= 0x180u;
    case 2:
      return a >= 0x100u;
    case 3:
      return true;
    default:
      return false;
    }
  }
};

/** The same chip behind a port that also has the optional delay_us() timing hook. */
class MockAt25040Timed : public MockAt25040 {
public:
  std::size_t delays = 0;

  void delay_us(uint32_t us) noexcept {
    ++delays;
    now_ns += static_cast<uint64_t>(us) * 1000u;
  }
};
