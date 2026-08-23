#pragma once
/**
 * @file at25040_registers.hpp
 * @brief AT25010/020/040B SPI EEPROM opcodes and status bits (Microchip).
 */
#include <cstdint>

namespace at25040 {

inline constexpr uint8_t kCmdWren = 0x06;
inline constexpr uint8_t kCmdWrdi = 0x04;
inline constexpr uint8_t kCmdRdsr = 0x05;
inline constexpr uint8_t kCmdWrsr = 0x01;
inline constexpr uint8_t kCmdRead = 0x03;
inline constexpr uint8_t kCmdWrite = 0x02;

/** A8 of the 9-bit address is placed in bit 3 of READ/WRITE. */
inline constexpr uint8_t kAddrA8Mask = 0x08;

inline constexpr uint8_t kSrWip = 0x01;
inline constexpr uint8_t kSrWel = 0x02;
inline constexpr uint8_t kSrBp0 = 0x04;
inline constexpr uint8_t kSrBp1 = 0x08;

/** AT25040B: 512 bytes, 8-byte page. */
inline constexpr uint16_t kCapacityBytes = 512;
inline constexpr uint16_t kPageSizeBytes = 8;

}  // namespace at25040
