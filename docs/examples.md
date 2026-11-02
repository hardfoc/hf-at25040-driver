# Examples

The ESP32 application that exercises the driver, and how to build it.

It targets an ESP32-C6 DevKit with an AT25040B on SPI2 (SCK GPIO6, SI GPIO7, SO GPIO2, CS GPIO10, WP and HOLD
to 3.3 V). The port is a complete ESP-IDF `spi_master` implementation. The app probes the chip, prints the block
protection level, writes 12 bytes across three pages and the A8 boundary, reads them back and dumps part of the
array. Without a chip it stops after `Probe()` reports `Absent`. CI builds the app below on each pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `at25040_esp32_example` :material-star:{ title="featured" } | AT25040 SPI EEPROM on ESP32-C6: spi_master port, probe, multi-page write and read back | [`at25040_esp32_example.cpp`](../examples/esp32/main/at25040_esp32_example.cpp) | yes |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh at25040_esp32_example Release
./scripts/flash_app.sh flash_monitor at25040_esp32_example Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
. $IDF_PATH/export.sh                                 # ESP-IDF v5.5
./scripts/build_app.sh list                           # every app and build type
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image.

## What to read in an example

| You want to | Read |
| --- | --- |
| See the port class an ESP-IDF build needs | `Esp32At25040Spi` in `at25040_esp32_example.cpp`: bus setup, `transfer()`, `delay_us()` |
| Change the pins or the clock | The `kPin...` and `kClockHz` constants at the top of `at25040_esp32_example.cpp` |
| See how the component is pulled into an IDF project | [`components/hf_at25040/CMakeLists.txt`](../examples/esp32/components/hf_at25040/CMakeLists.txt) |
| Read, write and wait in full | [`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp) (compiled on the host) |
