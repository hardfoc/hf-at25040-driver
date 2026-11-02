/**
 * @file mock_at25040.hpp
 * @brief A model of the AT25040B on the SPI bus, for the host tests.
 *
 * Follows the datasheet (Atmel-8707F): opcodes 0000 A011 / 0000 A010 with A8 in bit 3, WEL set by
 * WREN and cleared at the end of a write cycle, 8-byte pages that wrap inside the page, reads that
 * wrap at the end of the array, and a status register that reads 0xFF during the internal write
 * cycle (modelled as a number of RDSR polls instead of time).
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
  int busy_polls = 0;        ///< RDSR polls left in the current write cycle
  int write_cycle_polls = 3; ///< length of a write cycle, in RDSR polls
  Bus bus = Bus::Chip;
  std::size_t transfers = 0;
  uint8_t last_opcode = 0;

  MockAt25040() {
    mem.fill(0xFF);
  }

  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t /*timeout_ms*/) noexcept {
    ++transfers;
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
    if (busy_polls > 0) {                                   // only RDSR works during a write cycle
      if (op == at25040::kCmdRdsr && len >= 2) {
        --busy_polls;
        rx[1] = 0xFF;
        if (busy_polls == 0) {
          wel = false;
        }
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
  void startWriteCycle() noexcept {
    busy_polls = write_cycle_polls;
  }

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
