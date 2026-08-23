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

template <typename Spi>
class Device {
 public:
  explicit Device(Spi& spi) noexcept : spi_(spi) {}

  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

  [[nodiscard]] uint8_t LastRdsr() const noexcept { return last_rdsr_; }

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

  Status Read(uint16_t addr, uint8_t* dst, uint16_t len) noexcept {
    if (dst == nullptr || len == 0u ||
        (static_cast<uint32_t>(addr) + len) > kCapacityBytes) {
      return Status::Arg;
    }
    uint8_t cmd = kCmdRead;
    if ((addr & 0x100u) != 0u) {
      cmd = static_cast<uint8_t>(cmd | kAddrA8Mask);
    }
    constexpr uint16_t kMax = 16;
    if (len > kMax) {
      len = kMax;
    }
    uint8_t tx[2 + kMax] = {};
    uint8_t rx[2 + kMax] = {};
    tx[0] = cmd;
    tx[1] = static_cast<uint8_t>(addr & 0xFFu);
    const uint16_t n = static_cast<uint16_t>(2u + len);
    if (!spi_.transfer(tx, rx, n, kTimeoutMs)) {
      return Status::Timeout;
    }
    for (uint16_t i = 0; i < len; ++i) {
      dst[i] = rx[2u + i];
    }
    return Status::Ok;
  }

  Status WriteEnable() noexcept {
    uint8_t tx = kCmdWren;
    uint8_t rx = 0xFF;
    return spi_.transfer(&tx, &rx, 1, kTimeoutMs) ? Status::Ok : Status::Timeout;
  }

  /** @brief Single-page write (≤ 8 bytes, must not cross a page). */
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
    if (WriteEnable() != Status::Ok) {
      return Status::Timeout;
    }
    uint8_t cmd = kCmdWrite;
    if ((addr & 0x100u) != 0u) {
      cmd = static_cast<uint8_t>(cmd | kAddrA8Mask);
    }
    uint8_t tx[2 + kPageSizeBytes] = {};
    uint8_t rx[2 + kPageSizeBytes] = {};
    tx[0] = cmd;
    tx[1] = static_cast<uint8_t>(addr & 0xFFu);
    for (uint16_t i = 0; i < len; ++i) {
      tx[2u + i] = src[i];
    }
    if (!spi_.transfer(tx, rx, static_cast<std::size_t>(2u + len), kTimeoutMs)) {
      return Status::Timeout;
    }
    return Status::Ok;
  }

 private:
  static constexpr uint32_t kTimeoutMs = 20;

  Spi& spi_;
  uint8_t last_rdsr_{0xFF};
};

inline const char* GetAt25040DriverVersion() noexcept {
  return HF_AT25040_VERSION_STRING;
}

}  // namespace at25040
