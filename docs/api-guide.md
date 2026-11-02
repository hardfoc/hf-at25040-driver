# API guide

How to do the common jobs with the driver: detect the chip, wait, read, write, protect and handle errors.

Every snippet on this page is compiled by the host CI from [`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp),
with `Eeprom` standing for `at25040::Device<MySpi>`. The [API reference](api-reference.md) lists every member.

## The shape of the API

One `at25040::Device<Spi>` object per chip, constructed from a reference to your port. It is not copyable and
holds no buffers beyond the stack frame of each call.

| Call | What it does |
| --- | --- |
| `Probe()` | Two RDSR: decides whether a chip is fitted |
| `ReadStatus(sr)` | One RDSR: write-cycle polling, protection level |
| `Read(addr, dst, len)` | READ: any range inside the array, in transactions of up to 16 data bytes |
| `WritePage(addr, src, len)` | WREN + WRITE: 1 to 8 bytes in one page, then waits for the write cycle |
| `Write(addr, src, len)` | Any range inside the array: one `WritePage()` per 8-byte page |
| `WaitReady(timeout_ms)` | RDSR until RDY clears: the end of a write cycle (default 10 ms budget) |
| `WriteEnable()` | One WREN, before an instruction you send yourself (WRSR) |
| `LastRdsr()` | No transfer: the last status byte read, for logging |

Every call except `LastRdsr()` returns `at25040::Status` and never throws. Check it.

## Detect the chip

```cpp
--8<-- "tests/docs/api_tasks.cpp:probe"
```

Call `Probe()` from an explicit attach or start-up step, not from a static constructor: the SPI bus may not be up
yet. A write cycle in progress also reads as `Absent` (every status bit is 1 while busy), so probe before you write,
or wait 5 ms after the last write. [Protocol](protocol.md#probe) lists every decision.

## Wait for a write cycle

After a WRITE or WRSR the chip runs an internal write cycle of up to 5 ms (t<sub>WC</sub>) and ignores everything
except RDSR until it is done. `WritePage()` and `Write()` wait for it before they return, so the next call is never
ignored. `WaitReady()` is the same wait on its own: use it after an instruction you sent through your port, or at
start-up, when the firmware may have been reset in the middle of a write:

```cpp
--8<-- "tests/docs/api_tasks.cpp:wait"
```

`WaitReady()` polls RDSR until RDY (bit 0) reads 0 and returns `Timeout` when the budget runs out (default
`kWriteCycleTimeoutMs` = 10 ms, twice t<sub>WC</sub>). If your port has the optional `delay_us()` hook
([Porting](porting.md#optional-delay-hook)), the driver sleeps 100 µs between polls; without it, it polls back to
back for a bounded count that lasts at least the budget even at 20 MHz. A floating MISO reads `0xFF`, which looks
like a write cycle that never ends: it also returns `Timeout`.

## Read

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

`Read()` returns all `len` bytes. The chip increments the address after every byte, so one READ could stream the
whole array; the driver sends one READ per 16 data bytes (`Device::kReadChunkBytes`) so that a port never sees more
than 18 bytes, and sets A8 in each opcode for addresses `0x100` and up. A range that runs past `0x1FF` is refused
with `Status::Arg` before anything is sent: the chip would roll over to `0x000`. If a transfer fails part way,
`Read()` returns `Timeout` and `dst` holds the chunks read before it.

## Write

```cpp
--8<-- "tests/docs/api_tasks.cpp:write"
```

A page is 8 bytes on an 8-byte boundary (`kPageSizeBytes`). `WritePage()` refuses a write that would cross one,
because the chip would wrap inside the page and overwrite its start; `Write()` splits a longer range at the page
boundaries. Each page costs one write cycle, so writing the whole 512-byte array takes 64 cycles, at most 320 ms.
If a page fails, `Write()` returns its status and the pages before it are written.

## Status and block protection

```cpp
--8<-- "tests/docs/api_tasks.cpp:status"
```

The driver does not wrap WRSR. Send it through your port after `WriteEnable()`:

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

| Level (BP1:BP0) | Read-only addresses (AT25040B) |
| --- | --- |
| 0 (`00`) | None |
| 1 (`01`) | `0x180`-`0x1FF` |
| 2 (`10`) | `0x100`-`0x1FF` |
| 3 (`11`) | `0x000`-`0x1FF` |

The bits are non-volatile. A write to a protected address is ignored by the chip and still returns `Ok` from
`WritePage()`: read back what matters.

## Handle errors

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Status | Meaning |
| --- | --- |
| `Ok` | The transactions completed. It does not prove a write was stored |
| `Timeout` | Your port's `transfer()` returned `false`, or a write cycle did not end within the `WaitReady()` budget |
| `Absent` | `Probe()` only: no plausible status register ([Protocol](protocol.md#probe)) |
| `Busy` | `Probe()` only: WIP set in a status byte that is not `0xFF` |
| `Arg` | `Read()`, `WritePage()`, `Write()`: null pointer, zero length, range past the array, or a page crossing (nothing is sent) |

The chip itself reports nothing about a refused write (WP low, latch clear, protected block). The only check is to
read the data back after the write cycle.
