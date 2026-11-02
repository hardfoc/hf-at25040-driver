# Troubleshooting

Symptoms you may see while bringing up an AT25040B, their causes and the fix for each.

Start with `LastRdsr()` after a failed `Probe()`: it tells a dead bus (`0xFF`), a looped-back bus (`0x05`) and a
working chip apart without a logic analyser.

| Symptom | Cause and fix |
| --- | --- |
| `Probe()` returns `Absent`, `LastRdsr()` is `0xFF` | Nothing drives MISO: wiring, CS on the wrong pin, no VCC. Fix: Check SO (pin 2) to MISO, CS (pin 1), VCC (pin 8) |
| Same, right after an MCU reset | The chip is finishing a write cycle the old firmware started: all status bits read 1. Fix: Call `WaitReady()` before `Probe()` |
| `Probe()` returns `Absent`, `LastRdsr()` is `0x05` | MISO echoes MOSI one byte late. Fix: Look for a short between SI and SO, or a missing device on that CS |
| `Probe()` returns `Absent`, the two reads differ | Bus contention or noise. Fix: Another device drives MISO while this CS is low; lower the clock |
| Calls return `Busy` | RDSR shows a write cycle in progress: one started outside the driver (an MCU reset during a write, a WRITE or WRSR sent through your port). Fix: `WaitReady()`, then retry |
| `Read()` and the writes return `Busy`, `Probe()` returns `Absent` with `0xFF` | Nothing drives MISO, which then reads RDY = 1. Fix: Check SO (pin 2), CS (pin 1), VCC |
| `SetBlockProtection()` returns `Ok`, `GetBlockProtection()` shows the old level | WP is low: the chip ignores WRSR. Fix: WP (pin 3) high |
| Every call returns `Timeout` | Your port's `transfer()` returned `false`. Fix: Bus not initialised, or the bus lock timed out |
| `WritePage()` returns `Ok`, data unchanged | WP low or the address is block-protected: the chip ignores the write. Fix: WP (pin 3) high; `ReadStatus()` BP bits |
| `WritePage()` or `WaitReady()` returns `Timeout`, other calls work | RDY still 1 after 10 ms: MISO reads `0xFF` (floating), or the chip is damaged. Fix: Check SO (pin 2) and VCC; `LastRdsr()` shows the last status byte |
| `WritePage()` returns `Arg` | More than 8 bytes, or the write crosses an 8-byte page. Fix: Use `Write()`, which splits at page boundaries ([API guide](api-guide.md#write)) |
| `Read()` returns `Arg` | Null buffer, zero length, or `addr + len` past `0x200`. Fix: Keep the range inside the array |
| Data at `0x100` and up appears at `0x000` | An AT25010B or AT25020B: no A8. Fix: Check the part marking; keep to its size |
| Reads fine, writes fail at high clock | Clock above the limit for your VCC. Fix: 5 MHz at 1.8 V, 10 MHz at 2.5 V, 20 MHz at 4.5 V |
| Random failures, worse with long wires | HOLD floating. Fix: Tie HOLD (pin 7) high |
