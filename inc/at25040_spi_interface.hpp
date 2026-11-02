#pragma once
/**
 * @file at25040_spi_interface.hpp
 * @brief CRTP SPI contract for the AT25040 driver.
 */
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace at25040 {

/**
 * @tparam Derived Must implement
 *         `bool transfer(const uint8_t* tx, uint8_t* rx, size_t len, uint32_t timeout_ms)`.
 *         May implement `void delay_us(uint32_t us)` (sleep at least `us` microseconds): when it
 *         does, Device::WaitReady() sleeps between status polls instead of polling back to back.
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

namespace detail {

/** True when `T` has a callable `delay_us(uint32_t)` member (the optional timing hook). */
template <typename T, typename = void>
struct HasDelayUs : std::false_type {};
template <typename T>
struct HasDelayUs<T, std::void_t<decltype(std::declval<T&>().delay_us(uint32_t{0}))>>
    : std::true_type {};

} // namespace detail

} // namespace at25040
