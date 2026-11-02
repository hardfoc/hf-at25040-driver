# Testing and CI

How the driver is checked: host unit tests against a model of the chip, compiled documentation, ESP32 builds and
static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a model of the AT25040B on the SPI bus: the instruction set with A8 in the
opcode, the write-enable latch, page wrap-around, block protection, and a timed write cycle during which the status
register reads `0xFF` and every other instruction is ignored. Each transfer takes model time at the bus clock, and
a second model adds the optional `delay_us()` hook. The bus can also float, echo MOSI or fail.

```bash
cmake -S . -B build-host -DHF_AT25040_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_device.cpp`](../tests/test_device.cpp) | Probe decisions, A8 placement, argument checks, reads of any length across A8, page writes, multi-page `Write()`, waiting for the write cycle with and without a delay hook, wait timeouts, `Busy` before every instruction during a write cycle, WRDI and block protection |
| [`tests/mock_at25040.hpp`](../tests/mock_at25040.hpp) | The chip model |
| [`tests/install_consumer.cmake`](../tests/install_consumer.cmake) | `cmake --install` into the build tree, then a `find_package(hf_at25040)` consumer that links `hf::at25040` and includes every public header |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_spi.hpp), which the host build
compiles (target `hf_at25040_doc_snippets`) with the driver instantiated over the documented port. A change to
the API that breaks a page breaks the build first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml`, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

Once the example's port is connected, the check on a board is the one in [Porting](porting.md#check-your-port):
probe, write a page, wait, read back, and read on both sides of `0x100`.
