# Changelog

All notable changes to this driver are listed here. Versions follow
[Semantic Versioning](https://semver.org/). Entries marked **BREAKING** need source changes in
code that used the affected API.

## Unreleased

### Added

- `WaitReady(timeout_ms)`: polls RDSR until RDY clears, `Timeout` after the budget (default
  `kWriteCycleTimeoutMs` = 10 ms, twice the 5 ms t<sub>WC</sub>).
- `Write(addr, src, len)`: any range inside the array, split at 8-byte page boundaries, waiting after each page.
- `WriteDisable()` (WRDI), `SetBlockProtection(level)` (WREN + WRSR + write cycle) and
  `GetBlockProtection(level)`, with `enum class at25040::BlockProtect { None, UpperQuarter, UpperHalf, All }`.
- `cmake --install` support: the headers, the generated `at25040_version.h` and a `find_package(hf_at25040)`
  package exporting `hf::at25040`, checked by the `hf_at25040_install_consumer` test.
- Optional port hook `void delay_us(uint32_t us)`, detected at compile time: `WaitReady()` sleeps 100 µs between
  polls when it is there, and polls a bounded number of times when it is not.
- Host unit tests (`-DHF_AT25040_BUILD_TESTS=ON`) against a model of the AT25040B, and a CI workflow.

### Changed

- **BREAKING** (behaviour): `Read()`, `WritePage()`, `Write()`, `WriteEnable()`, `WriteDisable()` and
  `SetBlockProtection()` read the status register first and return `Status::Busy` without sending anything else
  while a write cycle runs; the chip would ignore the instruction. `Busy` used to come only from `Probe()`. A read on
  a floating MISO (`0xFF`) now returns `Busy` instead of `Ok` with `0xFF` data.
- **BREAKING** (behaviour): `WritePage()` waits for the internal write cycle before it returns (up to 5 ms, at most
  10 ms before it gives up with `Timeout`). It used to return as soon as the data was sent, so a following call
  inside t<sub>WC</sub> was silently ignored by the chip. Polling loops after `WritePage()` still work but are no
  longer needed. `Timeout` now also means "write cycle did not end within the budget".
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
