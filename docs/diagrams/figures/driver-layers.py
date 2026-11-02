"""The three layers of the driver: hardware-agnostic core, communication interface, platform port.

Reads: From top to bottom. Your application talks to one at25040::Device<Spi> object, whose public member functions
(listed from inc/at25040.hpp) turn a call into AT25040 instructions: RDSR, WREN, READ and WRITE with A8 in the
opcode. The communication interface (at25040::SpiInterface in inc/at25040_spi_interface.hpp) is a CRTP contract with
one method. The platform port is the only code you write: a class whose transfer() runs one SPI transaction with
chip select held low, and optionally delay_us() for the write-cycle wait. Below it is the hardware.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/at25040.hpp"
COMM = "inc/at25040_spi_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    body = re.split(r"^\s*public:", drv, maxsplit=1, flags=re.M)[1]
    body = re.split(r"^\s*private:", body, maxsplit=1, flags=re.M)[0]
    methods = []
    for name in re.findall(r"^  (?:\[\[nodiscard\]\] )?[\w:]+ (\w+)\([^)]*\)(?: const)? noexcept", body, re.M):
        if name != "Device" and name not in methods:
            methods.append(name)
    comm = (ROOT / COMM).read_text()
    hooks = [n for n in ("transfer",) if re.search(rf"\b{n}\(", comm)]
    optional = [n for n in ("delay_us",) if re.search(rf"\b{n}\(", comm)]
    return {
        "methods": [m + "()" for m in methods],
        "port_hooks": hooks,
        "optional_hooks": optional,
        "sources": [f"{DRIVER}:Device members", f"{COMM}:SpiInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic AT25040 Device and its member functions, which "
        "use the CRTP SPI interface, which calls the one transfer method of the platform port you write, which drives "
        "the SPI bus and chip select of the AT25040B.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a class with one method "
        "and an optional delay.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["board ID, calibration constants, settings ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, d["methods"], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="at25040::Device<Spi>, inc/at25040.hpp")
    comm = f.card(0, reg.b + 40, W, "Communication interface",
                  ["at25040::SpiInterface<Derived>: one full-duplex transaction per call,",
                   "instructions and A8 placed by the driver, bounds and page checks before the bus"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)",
                  ["  ".join(h + "(tx, rx, len, timeout_ms)" for h in d["port_hooks"])]
                  + [h + "(us)  optional: sleep between write-cycle polls" for h in d.get("optional_hooks", [])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "AT25040B",
                ["SPI mode 0 or 3, up to 5 / 10 / 20 MHz  /  CS, SO, SI, SCK, WP, HOLD"], tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
