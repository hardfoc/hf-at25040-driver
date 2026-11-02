# Changelog

All notable changes to this driver are listed here. Versions follow
[Semantic Versioning](https://semver.org/). Entries marked **BREAKING** need source changes in
code that used the affected API.

## Unreleased

### Added

- Host unit tests (`-DHF_AT25040_BUILD_TESTS=ON`) against a model of the AT25040B, and a CI workflow.

### Changed

- `Read()` returns every byte requested. It used to shorten any `len` above 16 to 16 and still return `Ok`; it
  now splits the range into READ transactions of up to `Device::kReadChunkBytes` (16) data bytes, recomputes A8
  for each, and returns `Arg` for a range past `0x1FF`. Code that read in 16-byte chunks keeps working.

- Documentation moved to the HardFOC MkDocs platform: standard page set (quickstart, hardware, architecture, API
  guide, porting, testing), code snippets compiled by the host build from `tests/docs/`, Doxygen under `/api/`,
  versioned with mike. The Jekyll site files and the doxygen-awesome submodule are gone.

## 1.0.0

### Added

- `at25040::Device<Spi>`: `Probe()`, `ReadStatus()`, `Read()`, `WriteEnable()`, `WritePage()`, `LastRdsr()`.
- `at25040::SpiInterface<Derived>` CRTP contract, opcodes and status bits in `at25040_registers.hpp`.
- CMake build settings with a generated version header, ESP-IDF component wrapper and ESP32 example.
