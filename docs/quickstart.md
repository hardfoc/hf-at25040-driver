# Quickstart

Store four bytes in an AT25040B and read them back: wire six signals, write a one-method port class, call four
driver functions.

## 1. Wire it

| MCU | AT25040B pin | Notes |
| --- | --- | --- |
| SPI SCLK | SCK (6) | Mode 0 or 3; start at 1 MHz |
| SPI MOSI | SI (5) | |
| SPI MISO | SO (2) | High impedance while CS is high |
| GPIO or hardware CS | CS (1) | Active low; one transaction per CS-low period |
| 3.3 V (1.8 V to 5.5 V) | VCC (8) | Decouple with 100 nF close to the pin |
| 3.3 V or GPIO | WP (3) | High to allow writes; low blocks every write |
| 3.3 V | HOLD (7) | Keep high unless you pause transfers |
| GND | GND (4) | Common ground with the MCU |

The [Hardware](hardware.md) page has the bus timing, the other packages and the protection options.

## 2. Add the driver

Add the repository as a submodule and link the `hf_at25040` CMake target (or add `inc/` to your include path and
build with C++17). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-at25040-driver.git external/hf-at25040-driver
```

## 3. Write the port

The port is the only platform code: one method that runs one SPI transaction. Replace the `platform_*` call with
your SDK's.

```cpp title="my_spi.hpp"
--8<-- "tests/docs/my_spi.hpp:port"
```

[Porting](porting.md) explains the contract and shows how to check a port.

## 4. Store and read back

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `Probe()` reads the status register twice. It returns `Ok` only when both reads agree, look like a real status
   byte and show no write in progress.
2. `WritePage()` sends WREN, then WRITE with the address and up to eight bytes inside one page. The chip starts its
   internal write cycle when CS goes high.
3. `WritePage()` then polls the status register until bit 0 (RDY) clears, at most 10 ms: during the write cycle the
   chip ignores everything except RDSR.
4. `Read()` sends READ and the address and returns the bytes asked for, from any range inside the array.

This example is built by the host CI, so it
compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `Probe()` returns `Absent`, `LastRdsr()` is `0xFF` | MISO wiring, CS reaching pin 1, VCC; a write cycle still running |
| `Probe()` returns `Absent`, `LastRdsr()` is `0x05` | MISO is connected to MOSI, or the wrong CS is low |
| Writes return `Ok` but the data does not change | WP is low, the address is block-protected, or you read before the write cycle ended |
| Data at `0x100` and up appears at `0x000` | The part is an AT25010B or AT25020B, not an AT25040B |

[Troubleshooting](troubleshooting.md) has the full list.

## Next

- [API guide](api-guide.md): longer reads and writes, status and block protection, errors.
- [Hardware](hardware.md): timing, power and protection pins.
- [Examples](examples.md): the ESP32 application.
