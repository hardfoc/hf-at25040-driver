---
layout: default
title: Quick start
parent: "📚 Documentation"
---

# Quick start

Implement `at25040::SpiInterface<Derived>::transfer`, construct
`at25040::Device<Derived>`, call `Probe()` only after the board is attached.

On Flux the EEPROM shares SPI with the ADS7952 and uses a dedicated CS
(`CS_ID`). The product manager constructs `At25040Handler` with that `BaseSpi*`.
