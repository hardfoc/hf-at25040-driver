#pragma once
/**
 * @file at25040_spi_interface.hpp
 * @brief CRTP SPI contract for the AT25040 driver.
 */
#include <cstddef>
#include <cstdint>

namespace at25040 {

/**
 * @tparam Derived Must implement
 *         `bool transfer(const uint8_t* tx, uint8_t* rx, size_t len, uint32_t timeout_ms)`.
 */
template <typename Derived>
class SpiInterface {
 public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t timeout_ms) {
    return static_cast<Derived*>(this)->transfer(tx, rx, len, timeout_ms);
  }

  SpiInterface(const SpiInterface&) = delete;
  SpiInterface& operator=(const SpiInterface&) = delete;

 protected:
  SpiInterface() = default;
  SpiInterface(SpiInterface&&) = default;
  SpiInterface& operator=(SpiInterface&&) = default;
  ~SpiInterface() = default;
};

}  // namespace at25040
