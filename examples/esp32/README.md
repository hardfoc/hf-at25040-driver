# AT25040 ESP32 example

A complete ESP-IDF `spi_master` port for the driver (`Esp32At25040Spi` in
`main/at25040_esp32_example.cpp`) and an app that probes the chip, reads the block protection level, writes
12 bytes across three pages and the A8 boundary, and reads them back.

Wiring on an ESP32-C6 DevKitC (3.3 V): SCK GPIO6, SI GPIO7, SO GPIO2, CS GPIO10, WP and HOLD to VCC. Change the
`kPin...` constants to use other pins.

Apps are listed in `app_config.yml` and built with the `examples/esp32/scripts` submodule
(`hf-espidf-project-tools`):

```bash
./scripts/build_app.sh at25040_esp32_example Debug
```
