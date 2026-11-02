/**
 * @file main.cpp
 * @brief Compiles every public header from an installed copy of the driver.
 */
#include "at25040.hpp"
#include "at25040_registers.hpp"
#include "at25040_spi_interface.hpp"
#include "at25040_version.h"

#include <cstring>

class InstalledPort : public at25040::SpiInterface<InstalledPort> {
public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t timeout_ms) {
    (void)tx;
    (void)timeout_ms;
    std::memset(rx, 0, len);
    return true;
  }
};

template class at25040::Device<InstalledPort>;

// The generated version header was installed: the built-in fallback defines only the string.
static_assert(HF_AT25040_VERSION_MAJOR >= 1, "at25040_version.h from the install");
