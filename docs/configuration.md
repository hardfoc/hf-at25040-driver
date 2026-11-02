# Configuration

What you choose for an AT25040B: the part, the bus, write protection and the driver's fixed limits.

The driver has no run-time configuration structure. Everything below is a board decision, a compile-time constant
in [`inc/at25040_registers.hpp`](../inc/at25040_registers.hpp) or a value in the chip's status register.

## Board decisions

| Decision | Options | Where it shows |
| --- | --- | --- |
| Part | AT25010B (128 B), AT25020B (256 B), AT25040B (512 B) | Address range you may use ([Hardware](hardware.md#variants)) |
| SPI clock | 5, 10 or 20 MHz maximum, by VCC | Your port's bus setup |
| SPI mode | 0 or 3 | Your port's bus setup |
| WP | Tied high, or a GPIO | Hardware write protection of the whole chip |
| HOLD | Tied high | Must not float |

## Constants

| Constant | Value | Meaning |
| --- | --- | --- |
| `kCapacityBytes` | `512` | Upper bound for `Read()` and `WritePage()` ranges |
| `kPageSizeBytes` | `8` | Largest write, and the boundary a write may not cross |
| `kAddrA8Mask` | `0x08` | Bit of the READ / WRITE opcode that carries A8 |
| `kCmdWren` ... `kCmdWrite` | `0x06`, `0x04`, `0x05`, `0x01`, `0x03`, `0x02` | Instructions ([Protocol](protocol.md)) |
| `kSrWip`, `kSrWel`, `kSrBp0`, `kSrBp1` | `0x01`, `0x02`, `0x04`, `0x08` | Status register bits |

Two limits live inside `Device` and are not configurable: every transfer gets a 20 ms timeout, and `Read()` sends
at most 16 data bytes per READ transaction (`Device::kReadChunkBytes`), splitting longer reads.

## Block protection

The protection level is stored in the chip (BP1:BP0, non-volatile) and survives power cycles. Set it once in
production with WRSR, as the [API guide](api-guide.md#status-and-block-protection) shows, and read it at start-up
with `ReadStatus()` if your firmware depends on it.

A common layout for board identity data: write the ID and calibration into the top half (`0x100`-`0x1FF`), then set
level 2 so that half is read-only and the bottom half stays writable for settings.

## Version

`GetAt25040DriverVersion()` returns the version from
[`cmake/hf_at25040_build_settings.cmake`](../cmake/hf_at25040_build_settings.cmake), written into
`at25040_version.h` at configure time.
