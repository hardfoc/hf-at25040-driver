/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port replaces
 * them with its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux spidev). The class is what the driver needs:
 * transfer(), and the optional delay_us() timing hook.
 */
#pragma once

#include "at25040_spi_interface.hpp"

#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_spi_transfer(const uint8_t* tx, uint8_t* rx, std::size_t len,
                           uint32_t timeout_ms) noexcept;
void platform_delay_us(uint32_t us) noexcept;

// --8<-- [start:port]
#include "at25040_spi_interface.hpp"

class MySpi : public at25040::SpiInterface<MySpi> {
public:
  // One transaction: CS low, `len` bytes full duplex (SPI mode 0 or 3, MSB first), CS high.
  // rx[i] is the byte clocked in while tx[i] went out. Return false only on a bus error or timeout.
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t timeout_ms) noexcept {
    return platform_spi_transfer(tx, rx, len, timeout_ms);
  }

  // Optional: sleep at least `us` microseconds. With it, WaitReady() (and every write) sleeps 100
  // us between status polls; without it, the driver polls back to back for a bounded count.
  void delay_us(uint32_t us) noexcept {
    platform_delay_us(us);
  }
};
// --8<-- [end:port]
