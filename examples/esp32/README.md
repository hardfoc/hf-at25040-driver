# AT25040 ESP32 example

Implements the CRTP SPI adapter and runs `Probe()` / `Read(0)`. Apps are
listed in `app_config.yml` and built with the `examples/esp32/scripts`
submodule (`hf-espidf-project-tools`), the same way as `hf-mcp9700-driver`:

```bash
./scripts/build_app.sh at25040_esp32_example Debug
```
