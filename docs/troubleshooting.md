# Troubleshooting

Symptoms you may see while bringing up an AT25040B, their causes and the fix for each.

Start with `LastRdsr()` after a failed `Probe()`: it tells a dead bus (`0xFF`), a looped-back bus (`0x05`) and a
working chip apart without a logic analyser.

| Symptom | Cause and fix |
| --- | --- |
| `Probe()` returns `Absent`, `LastRdsr()` is `0xFF` | Nothing drives MISO: wiring, CS on the wrong pin, no VCC. Fix: Check SO (pin 2) to MISO, CS (pin 1), VCC (pin 8) |
| Same, right after a write | The chip is in its write cycle: all status bits read 1. Fix: Wait 5 ms after the last write, or poll `ReadStatus()` before probing |
| `Probe()` returns `Absent`, `LastRdsr()` is `0x05` | MISO echoes MOSI one byte late. Fix: Look for a short between SI and SO, or a missing device on that CS |
| `Probe()` returns `Absent`, the two reads differ | Bus contention or noise. Fix: Another device drives MISO while this CS is low; lower the clock |
| Every call returns `Timeout` | Your port's `transfer()` returned `false`. Fix: Bus not initialised, or the bus lock timed out |
| `WritePage()` returns `Ok`, data unchanged | WP low, the address is block-protected, or you read before the write cycle ended. Fix: WP (pin 3) high; `ReadStatus()` BP bits; wait for WIP to clear |
| Only the first write of a burst sticks | Writes sent during the previous write cycle are ignored. Fix: Wait for WIP after each `WritePage()` ([API guide](api-guide.md#wait-for-a-write-cycle)) |
| `WritePage()` returns `Arg` | More than 8 bytes, or the write crosses an 8-byte page. Fix: Split at page boundaries ([API guide](api-guide.md#write)) |
| `Read()` returns `Arg` | Null buffer, zero length, or `addr + len` past `0x200`. Fix: Keep the range inside the array |
| Data at `0x100` and up appears at `0x000` | An AT25010B or AT25020B: no A8. Fix: Check the part marking; keep to its size |
| Reads fine, writes fail at high clock | Clock above the limit for your VCC. Fix: 5 MHz at 1.8 V, 10 MHz at 2.5 V, 20 MHz at 4.5 V |
| Random failures, worse with long wires | HOLD floating. Fix: Tie HOLD (pin 7) high |
