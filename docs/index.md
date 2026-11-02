---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# AT25040 Driver

<p>A header-only C++17 driver for the Microchip AT25040B serial EEPROM (also AT25010B and AT25020B). It detects the
chip safely, reads and writes it in pages and reports every result as a status, on any MCU with SPI.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-at25040-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class with one method that runs an SPI transaction; it does
the rest: the instruction set, the ninth address bit, bounds and page checks, and a probe that tells a missing chip
from a present one.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">4-Kbit SPI serial EEPROM</span>
<span class="hf-chip__name">AT25040B</span>
<span class="hf-chip__vendor">Microchip (Atmel)</span>
</div>

Memory
:   512 x 8 bits (AT25040B); 128 x 8 (AT25010B), 256 x 8 (AT25020B); 8-byte pages

Interface
:   SPI mode 0 or 3, MSB first; 20 MHz at 4.5 V to 5.5 V, 10 MHz at 2.5 V, 5 MHz at 1.8 V

Supply
:   1.8 V to 5.5 V; 3.5 µA maximum standby at 5 V

Writes
:   Self-timed, 5 ms maximum per page; 1,000,000 cycles, 100-year retention

Protection
:   WP pin, write-enable latch, block protection of the top 1/4, top 1/2 or whole array

Package
:   8-lead SOIC, TSSOP, UDFN, XDFN, 8-ball VFBGA; -40 °C to +85 °C

[Product page and datasheet](https://www.microchip.com/en-us/product/AT25040B)
</div>

## What you get

- **One object per chip.** `at25040::Device<YourSpi>` with `Probe()`, `Read()`, `WritePage()`, `ReadStatus()` and
  `WriteEnable()`. See [API guide](api-guide.md).
- **Safe detection.** `Probe()` reads the status register twice and rejects a floating bus (`0xFF`) and a bus that
  echoes the command back. See [Hardware](hardware.md).
- **The ninth address bit handled.** Addresses `0x100` to `0x1FF` put A8 into the opcode for you. See
  [Protocol](protocol.md).
- **Checks before the bus.** A bad pointer, a zero length, a range past the array or a write that crosses a page
  returns `Status::Arg` without touching SPI.
- **Zero-overhead portability.** The port is a CRTP class: no virtual calls, no heap, no exceptions, no RTOS. See
  [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the chip. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the chip, write the port, store and read back four bytes.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, bus settings, timing, power and write protection.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Probe, read, write, wait and handle errors, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the one-method SPI interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and constant, generated from the headers.

</div>
