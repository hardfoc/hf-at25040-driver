---
layout: default
title: "HardFOC AT25040 Driver"
description: "Header-only Microchip AT25010/020/040B SPI EEPROM driver"
nav_order: 1
permalink: /
---

# HF-AT25040 Driver

**Microchip [AT25040B](https://www.microchip.com/en-us/product/AT25040B) SPI EEPROM (512 × 8) — also AT25010B / AT25020B**

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Docs](https://img.shields.io/badge/docs-GitHub%20Pages-blue)](https://n3b3x.github.io/hf-at25040-driver/)

Header-only C++17 template `at25040::Device<Spi>` for the 9-bit-address SPI
EEPROM used as board-ID storage on the Flux analog front-end (second CS on
the ADS7952 bus).

- SPI Mode 0, 8-bit frames
- READ/WRITE place A8 in bit 3 of the opcode
- `Probe()` is absent-safe (0xFF MISO and RDSR echo → absent)
- **Do not** clock the part from static constructors

The hf-core handler is `At25040Handler` (`HF_CORE_ENABLE_AT25040`). Product
identity parsing stays in the product HAL (`AccessoryIdentityManager`).

PGA and DAC AFE parts should copy this repository’s layout (CMake settings,
Jekyll docs, Doxygen, ESP32 example, reusable CI workflows) rather than
growing a `managers/afe/` folder.

## Quick start

```cpp
class MySpi : public at25040::SpiInterface<MySpi> {
 public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t timeout_ms);
};

MySpi spi;
at25040::Device<MySpi> eep(spi);
if (eep.Probe() == at25040::Status::Ok) {
  uint8_t id[8]{};
  (void)eep.Read(0, id, sizeof(id));
}
```

## Documentation

See [`docs/`](docs/) — installation, protocol, CMake, examples, datasheet links.

## License

GPL-3.0-or-later, consistent with sibling `hf-*-driver` repositories.
