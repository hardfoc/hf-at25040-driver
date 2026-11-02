# API guide

How to do the common jobs with the driver: detect the chip, read, write, wait, protect and handle errors.

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
| `WritePage(addr, src, len)` | WREN + WRITE: 1 to 8 bytes in one page; starts a write cycle |
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

`WritePage()` returns as soon as the data is sent; the chip then needs up to 5 ms (t<sub>WC</sub>) and ignores
everything except RDSR until it is done. Poll the WIP bit:

```cpp
--8<-- "tests/docs/api_tasks.cpp:wait"
```

At 1 MHz one RDSR takes 16 µs on the bus, so a tight loop works too; the delay just keeps the bus free for other
devices.

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
because the chip would wrap inside the page and overwrite its start. Each page costs one write cycle, so writing the
whole 512-byte array takes 64 cycles, at most 320 ms.

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
| `Timeout` | Your port's `transfer()` returned `false` |
| `Absent` | `Probe()` only: no plausible status register ([Protocol](protocol.md#probe)) |
| `Busy` | `Probe()` only: WIP set in a status byte that is not `0xFF` |
| `Arg` | `Read()`, `WritePage()`: null pointer, zero length, range past the array, or a page crossing (nothing is sent) |

The chip itself reports nothing about a refused write (WP low, latch clear, protected block). The only check is to
read the data back after the write cycle.
