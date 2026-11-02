# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++17 library with no dependencies.

## Requirements

- A C++17 compiler: GCC 7+, Clang 5+ or a current MSVC.
- No RTOS, heap or exceptions: every call is `noexcept` and returns `at25040::Status`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-at25040-driver.git external/hf-at25040-driver
git submodule update --init external/hf-at25040-driver
```

Only `inc/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodule
(`examples/esp32/scripts`); you do not need it to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-at25040-driver)
target_link_libraries(my_app PRIVATE hf_at25040)   # or the alias hf::at25040
```

`hf_at25040` is an `INTERFACE` target: it carries the include directories (`inc/` and the generated version
header) and requires C++17 (`cxx_std_17`), so your target does not need to set either.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_AT25040_BUILD_TESTS` | `OFF` | Builds the host unit tests and the documentation snippets in `tests/` ([Testing and CI](testing.md)) |

## Installed package

`cmake --install` copies the headers (`at25040.hpp`, `at25040_registers.hpp`, `at25040_spi_interface.hpp` and the
generated `at25040_version.h`) to `include/` and a CMake package to `lib/cmake/hf_at25040/`, which provides the
same target name as the build tree:

```bash
cmake -S external/hf-at25040-driver -B build-at25040
cmake --install build-at25040 --prefix /opt/hardfoc
```

```cmake
find_package(hf_at25040 CONFIG REQUIRED)            # CMAKE_PREFIX_PATH=/opt/hardfoc
target_link_libraries(my_app PRIVATE hf::at25040)
```

Without CMake, add `inc/` to the include path. `at25040.hpp` then falls back to a built-in version string.

## ESP-IDF

The ESP-IDF component wrapper is [`examples/esp32/components/hf_at25040/`](../examples/esp32/components/hf_at25040/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_AT25040_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the path is
wrong. Your component then declares `REQUIRES hf_at25040`.

## Inside hf-core

In the HardFOC core, the driver is a submodule of `hf-core-drivers` and is switched on with `HF_CORE_ENABLE_AT25040`,
which also compiles the `At25040Handler`:

```cmake
set(HF_CORE_ENABLE_AT25040 ON)
include("${HF_CORE_DRIVER_EXT}/hf-at25040-driver/cmake/hf_at25040_build_settings.cmake")
```

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_at25040_build_settings.cmake`](../cmake/hf_at25040_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | `INTERFACE` target for host builds and CI, install and `hf::at25040` export |
| ESP-IDF | [`examples/esp32/components/hf_at25040/CMakeLists.txt`](../examples/esp32/components/hf_at25040/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `at25040_version.h` at configure time. Read it at
run time:

```cpp
const char* v = at25040::GetAt25040DriverVersion();   // "1.0.0"
```

## Next

[Quickstart](quickstart.md) wires the chip and stores a few bytes; [Porting](porting.md) writes the SPI interface
for your MCU.
