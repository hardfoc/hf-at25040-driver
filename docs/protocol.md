# Protocol

The AT25040B's instruction set, status register and addressing, and which driver call uses each instruction.

Every instruction starts with CS going low and ends with CS going high. Bytes go MSB first. An invalid opcode leaves
SO in high impedance until the next CS falling edge.

## Instructions

| Instruction | Opcode | What it does |
| --- | --- | --- |
| WREN | `0x06` | Sets the write-enable latch; WP must be high. `WriteEnable()`, `WritePage()`, `SetBlockProtection()` |
| WRDI | `0x04` | Clears the write-enable latch, regardless of WP. `WriteDisable()` |
| RDSR | `0x05` | Reads the status register, also during a write cycle. `ReadStatus()`, `Probe()`, `WaitReady()`, `GetBlockProtection()`, and the busy check before every other instruction |
| WRSR | `0x01` | Writes BP1:BP0 (non-volatile, takes a write cycle). `SetBlockProtection()` |
| READ | `0x03`, `0x0B` with A8 | Address byte, then data out; the address increments. `Read()` |
| WRITE | `0x02`, `0x0A` with A8 | Address byte, then 1 to 8 data bytes in one page. `WritePage()` |

The datasheet writes the opcodes as `0000 X110` (WREN) to `0000 A010` (WRITE); the constants are `kCmdWren`,
`kCmdWrdi`, `kCmdRdsr`, `kCmdWrsr`, `kCmdRead` and `kCmdWrite`. `A` is address bit A8 on the AT25040B
(`kAddrA8Mask = 0x08`); `X` is ignored. During a write cycle every instruction except RDSR is ignored, so the driver
sends RDSR first and returns `Status::Busy` instead of an instruction the chip would drop.

## Status register

<div class="hf-registers" markdown>

| Bit | Name | Meaning |
| --- | --- | --- |
| `0` | `RDY` (`kSrWip`) | 1: write cycle in progress |
| `1` | `WEN` (`kSrWel`) | 1: write-enable latch set |
| `2` | `BP0` (`kSrBp0`) | Block protection, low bit |
| `3` | `BP1` (`kSrBp1`) | Block protection, high bit |
| `7:4` | | 0 when ready |

</div>

During an internal write cycle all eight bits read 1, so a busy chip returns `0xFF`. A ready chip returns `0x0X`:
only the low four bits can be set.

## Addressing and wrap-around

- **READ**: the address increments after each byte and rolls over from `0x1FF` to `0x000`.
- **WRITE**: only the low three address bits increment, so a write wraps inside its 8-byte page and overwrites the
  start of the page.

The driver never lets either wrap happen: `Read()` refuses a range past `0x1FF`, `WritePage()` refuses a write that
crosses a page, both with `Status::Arg`.

## Probe

`Probe()` sends RDSR twice and decides from the two status bytes:

| Status bytes | Result | Typical cause |
| --- | --- | --- |
| A transfer failed | `BusError` | Bus error in the port |
| The two bytes differ | `Absent` | Noise, a contended MISO |
| `0xFF` | `Absent` | Nothing drives MISO (pull-up or floating), or a write cycle is running |
| `0x05` | `Absent` | MISO follows MOSI one byte late: the RDSR opcode comes back |
| RDY set, not `0xFF` | `Busy` | Not seen on an AT25040B, which reads `0xFF` while busy |
| Anything else | `Ok` | A ready chip: `0x00` to `0x0E` |

`LastRdsr()` returns the first status byte of the last probe, for logging. A chip in its write cycle and a floating
MISO both read `0xFF`, so `Probe()` cannot tell them apart; call `WaitReady()` first when a write may still be
running (after an MCU reset).
