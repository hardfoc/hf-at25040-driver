#pragma once
/**
 * @file at25040.hpp
 * @brief Header-only Microchip AT25010/020/040B SPI EEPROM driver.
 *
 * @details SPI Mode 0, 8-bit frames. READ/WRITE use a 9-bit address with A8
 *          in bit 3 of the opcode. Probe is hardware-absent safe: floating
 *          MISO (0xFF) and MOSI echo of RDSR (0x05) are treated as absent.
 *          Do not clock the part from static constructors — call Probe from
 *          an explicit attach / console / HIL path.
 */
#include "at25040_registers.hpp"
#include "at25040_spi_interface.hpp"
#if __has_include("at25040_version.h")
#include "at25040_version.h"
#else
#define HF_AT25040_VERSION_STRING "1.0.0"
#endif

#include <cstddef>
#include <cstdint>

namespace at25040 {

enum class Status : uint8_t {
  Ok = 0,
  Timeout = 1,
  Absent = 2,
  Busy = 3,
  Arg = 4,
};

/**
 * @brief Block write protection level, BP1:BP0 in the status register (datasheet Table 6-4).
 *
 * Addresses for the AT25040B; on the AT25010B/020B the same levels protect the same fractions.
 */
enum class BlockProtect : uint8_t {
  None = 0,         ///< BP1:BP0 = 00: nothing protected
  UpperQuarter = 1, ///< 01: 0x180-0x1FF read-only
  UpperHalf = 2,    ///< 10: 0x100-0x1FF read-only
  All = 3,          ///< 11: 0x000-0x1FF read-only
};

template <typename Spi>
class Device {
public:
  explicit Device(Spi& spi) noexcept : spi_(spi) {}

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  /** Data bytes per READ transaction; Read() splits longer requests into chunks of this size. */
  static constexpr uint16_t kReadChunkBytes = 16;

  /** Default WaitReady() budget: twice the 5 ms maximum tWC (datasheet Table 4-3). */
  static constexpr uint32_t kWriteCycleTimeoutMs = 10;

  /** Delay between two RDSR polls in WaitReady() when the port has `delay_us()`. */
  static constexpr uint32_t kPollIntervalUs = 100;

  /**
   * RDSR polls per millisecond of WaitReady() budget when the port has no `delay_us()`. One RDSR is
   * 16 SCK periods plus tCS: 0.9 us at the 20 MHz maximum clock (datasheet Table 4-3), so this many
   * polls last at least 1 ms on any legal bus; on a slower bus the wait is proportionally longer.
   */
  static constexpr uint32_t kPollsPerMsWithoutDelay = 1112;

  [[nodiscard]] uint8_t LastRdsr() const noexcept {
    return last_rdsr_;
  }

  Status ReadStatus(uint8_t& sr) noexcept {
    uint8_t tx[2] = {kCmdRdsr, 0x00};
    uint8_t rx[2] = {0xFF, 0xFF};
    if (!spi_.transfer(tx, rx, 2, kTimeoutMs)) {
      last_rdsr_ = 0xFF;
      return Status::Timeout;
    }
    sr = rx[1];
    last_rdsr_ = sr;
    return Status::Ok;
  }

  /**
   * @brief Double RDSR. Present if both match, not 0xFF, not the 0x05 echo,
   *        and WIP is clear.
   */
  Status Probe() noexcept {
    uint8_t a = 0xFF;
    uint8_t b = 0xFF;
    if (ReadStatus(a) != Status::Ok) {
      return Status::Absent;
    }
    if (ReadStatus(b) != Status::Ok) {
      return Status::Absent;
    }
    last_rdsr_ = a;
    if (a != b || a == 0xFFu || a == kCmdRdsr) {
      return Status::Absent;
    }
    if ((a & kSrWip) != 0u) {
      return Status::Busy;
    }
    return Status::Ok;
  }

  /**
   * @brief READ `len` bytes starting at `addr` (sequential read, any length).
   *
   * The chip increments the address after every byte (datasheet 8707F, section 6, "Read Sequence"),
   * so one READ can stream the whole array. The driver splits the request into READ transactions of
   * at most kReadChunkBytes data bytes, so a port never sees more than 2 + kReadChunkBytes bytes,
   * and recomputes A8 for each one.
   *
   * @return Status::Arg for a null `dst`, `len == 0` or a range past 0x1FF (no transfer is
   *         made; the chip would roll over to 0x000). Status::Busy if RDSR shows a write cycle in
   *         progress (the chip would ignore the READ; nothing is read). Status::Timeout if a
   *         transfer failed: the bytes of the chunks before it are in `dst`.
   */
  Status Read(uint16_t addr, uint8_t* dst, uint16_t len) noexcept {
    if (dst == nullptr || len == 0u || addr >= kCapacityBytes ||
        len > static_cast<uint16_t>(kCapacityBytes - addr)) {
      return Status::Arg;
    }
    const Status ready = CheckReady();
    if (ready != Status::Ok) {
      return ready;
    }
    while (len > 0u) {
      const uint16_t n = len < kReadChunkBytes ? len : kReadChunkBytes;
      uint8_t tx[2 + kReadChunkBytes] = {};
      uint8_t rx[2 + kReadChunkBytes] = {};
      tx[0] = Opcode(kCmdRead, addr);
      tx[1] = static_cast<uint8_t>(addr & 0xFFu);
      if (!spi_.transfer(tx, rx, static_cast<std::size_t>(2u + n), kTimeoutMs)) {
        return Status::Timeout;
      }
      for (uint16_t i = 0; i < n; ++i) {
        dst[i] = rx[2u + i];
      }
      addr = static_cast<uint16_t>(addr + n);
      dst += n;
      len = static_cast<uint16_t>(len - n);
    }
    return Status::Ok;
  }

  /**
   * @brief Poll RDSR until RDY (bit 0) reads 0: the internal write cycle has ended.
   *
   * The chip ignores every instruction except RDSR during a write cycle, and RDY = 1 while it runs
   * (datasheet section 6, Write Sequence, and Table 6-3). If the port has an optional
   * `void delay_us(uint32_t us)` member, the driver waits kPollIntervalUs between polls; otherwise
   * it polls back to back, kPollsPerMsWithoutDelay times per millisecond of `timeout_ms`.
   *
   * @param timeout_ms Budget; the default covers the 5 ms maximum tWC with margin.
   * @return Status::Ok when ready, Status::Timeout if a transfer failed or RDY was still 1 (also
   * what a floating MISO, which reads 0xFF, looks like) at the end of the budget.
   */
  Status WaitReady(uint32_t timeout_ms = kWriteCycleTimeoutMs) noexcept {
    constexpr bool kHasDelay = detail::HasDelayUs<Spi>::value;
    const uint32_t polls = kHasDelay ? (timeout_ms * 1000u) / kPollIntervalUs + 1u
                                     : timeout_ms * kPollsPerMsWithoutDelay + 1u;
    for (uint32_t i = 0; i < polls; ++i) {
      uint8_t sr = 0xFF;
      if (ReadStatus(sr) != Status::Ok) {
        return Status::Timeout;
      }
      if ((sr & kSrWip) == 0u) {
        return Status::Ok;
      }
      if constexpr (kHasDelay) {
        if (i + 1u < polls) {
          spi_.delay_us(kPollIntervalUs);
        }
      }
    }
    return Status::Timeout;
  }

  /**
   * @brief WREN: set the write-enable latch (datasheet Table 6-1), before an instruction you send
   *        through your port. WritePage(), Write() and SetBlockProtection() send it themselves.
   * @return Status::Busy during a write cycle (the chip would ignore it), Status::Timeout if a
   *         transfer failed.
   */
  Status WriteEnable() noexcept {
    const Status ready = CheckReady();
    return ready != Status::Ok ? ready : SendInstruction(kCmdWren);
  }

  /**
   * @brief WRDI: clear the write-enable latch (datasheet Table 6-1), so no WRITE or WRSR is
   *        accepted until the next WREN. Independent of the WP pin.
   * @return Status::Busy during a write cycle, Status::Timeout if a transfer failed.
   */
  Status WriteDisable() noexcept {
    const Status ready = CheckReady();
    return ready != Status::Ok ? ready : SendInstruction(kCmdWrdi);
  }

  /**
   * @brief WREN + WRSR: set BP1:BP0 (datasheet section 6, Write Status Register, Table 6-4), then
   *        wait for the write cycle: the bits are non-volatile and take tWC like a WRITE.
   *
   * WP must be high; the chip ignores WRSR otherwise. Read the level back with
   * GetBlockProtection() if it matters.
   *
   * @return Status::Arg for a value outside BlockProtect, Status::Busy if a write cycle was
   *         already running, Status::Timeout if a transfer failed or the write cycle did not end
   *         in time.
   */
  Status SetBlockProtection(BlockProtect level) noexcept {
    const auto bits = static_cast<uint8_t>(level);
    if (bits > 3u) {
      return Status::Arg;
    }
    const Status ready = CheckReady();
    if (ready != Status::Ok) {
      return ready;
    }
    if (SendInstruction(kCmdWren) != Status::Ok) {
      return Status::Timeout;
    }
    const uint8_t tx[2] = {kCmdWrsr, static_cast<uint8_t>(bits << 2)};
    uint8_t rx[2] = {0xFF, 0xFF};
    if (!spi_.transfer(tx, rx, 2, kTimeoutMs)) {
      return Status::Timeout;
    }
    return WaitReady();
  }

  /**
   * @brief RDSR, decoded: the current block protection level (datasheet Table 6-4).
   * @return Status::Busy during a write cycle (the status reads 0xFF; `level` is not changed),
   *         Status::Timeout if the transfer failed.
   */
  Status GetBlockProtection(BlockProtect& level) noexcept {
    uint8_t sr = 0xFF;
    if (ReadStatus(sr) != Status::Ok) {
      return Status::Timeout;
    }
    if ((sr & kSrWip) != 0u) {
      return Status::Busy;
    }
    level = static_cast<BlockProtect>((sr & (kSrBp1 | kSrBp0)) >> 2);
    return Status::Ok;
  }

  /**
   * @brief WREN + WRITE of 1 to 8 bytes inside one 8-byte page, then wait for the write cycle.
   *
   * Only the three low address bits increment during a WRITE, so bytes past the end of a page would
   * wrap to its start (datasheet section 6, Write Sequence, p. 9): a range that crosses a page is
   * refused. Programming starts when CS goes high; the call then polls RDSR (WaitReady()) and
   * returns when the chip is ready again, so the next call is not ignored.
   *
   * @return Status::Arg for a null `src`, a length of 0 or above kPageSizeBytes, a page
   *         crossing or a range past the array (nothing is sent). Status::Busy if RDSR shows a
   *         write cycle already in progress (nothing is written). Status::Timeout if a transfer
   *         failed or the write cycle did not end within kWriteCycleTimeoutMs.
   */
  Status WritePage(uint16_t addr, const uint8_t* src, uint16_t len) noexcept {
    if (src == nullptr || len == 0u || len > kPageSizeBytes) {
      return Status::Arg;
    }
    const uint16_t page_off = static_cast<uint16_t>(addr % kPageSizeBytes);
    if ((page_off + len) > kPageSizeBytes) {
      return Status::Arg;
    }
    if ((static_cast<uint32_t>(addr) + len) > kCapacityBytes) {
      return Status::Arg;
    }
    const Status ready = CheckReady();
    if (ready != Status::Ok) {
      return ready;
    }
    if (SendInstruction(kCmdWren) != Status::Ok) {
      return Status::Timeout;
    }
    uint8_t tx[2 + kPageSizeBytes] = {};
    uint8_t rx[2 + kPageSizeBytes] = {};
    tx[0] = Opcode(kCmdWrite, addr);
    tx[1] = static_cast<uint8_t>(addr & 0xFFu);
    for (uint16_t i = 0; i < len; ++i) {
      tx[2u + i] = src[i];
    }
    if (!spi_.transfer(tx, rx, static_cast<std::size_t>(2u + len), kTimeoutMs)) {
      return Status::Timeout;
    }
    return WaitReady();
  }

  /**
   * @brief Write any range inside the array: split at 8-byte page boundaries, one WritePage() (and
   *        one write cycle) per page.
   *
   * @return Status::Arg for a null `src`, `len == 0` or a range past 0x1FF (nothing is sent);
   *         otherwise the first non-Ok status of a page write (Status::Busy, Status::Timeout).
   *         Pages before it are written.
   */
  Status Write(uint16_t addr, const uint8_t* src, uint16_t len) noexcept {
    if (src == nullptr || len == 0u || addr >= kCapacityBytes ||
        len > static_cast<uint16_t>(kCapacityBytes - addr)) {
      return Status::Arg;
    }
    while (len > 0u) {
      const uint16_t room = static_cast<uint16_t>(kPageSizeBytes - (addr % kPageSizeBytes));
      const uint16_t n = len < room ? len : room;
      const Status st = WritePage(addr, src, n);
      if (st != Status::Ok) {
        return st;
      }
      addr = static_cast<uint16_t>(addr + n);
      src += n;
      len = static_cast<uint16_t>(len - n);
    }
    return Status::Ok;
  }

private:
  static constexpr uint32_t kTimeoutMs = 20;

  /** READ/WRITE opcode with address bit A8 in bit 3 (datasheet Table 6-1, note 1). */
  static constexpr uint8_t Opcode(uint8_t cmd, uint16_t addr) noexcept {
    return ((addr & 0x100u) != 0u) ? static_cast<uint8_t>(cmd | kAddrA8Mask) : cmd;
  }

  /**
   * RDSR before an instruction the chip ignores during a write cycle ("all commands will be
   * ignored except the RDSR instruction", datasheet section 6, Write Sequence): Busy when RDY = 1.
   */
  Status CheckReady() noexcept {
    uint8_t sr = 0xFF;
    if (ReadStatus(sr) != Status::Ok) {
      return Status::Timeout;
    }
    return (sr & kSrWip) != 0u ? Status::Busy : Status::Ok;
  }

  /** One-byte instruction (WREN, WRDI). */
  Status SendInstruction(uint8_t cmd) noexcept {
    uint8_t rx = 0xFF;
    return spi_.transfer(&cmd, &rx, 1, kTimeoutMs) ? Status::Ok : Status::Timeout;
  }

  Spi& spi_;
  uint8_t last_rdsr_{0xFF};
};

inline const char* GetAt25040DriverVersion() noexcept {
  return HF_AT25040_VERSION_STRING;
}

} // namespace at25040
