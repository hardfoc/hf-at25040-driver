#include "at25040.hpp"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "at25040";

class Esp32At25040Spi : public at25040::SpiInterface<Esp32At25040Spi> {
 public:
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t) {
    (void)tx;
    (void)rx;
    (void)len;
    return false; /* wire to spi_device_transmit in a board-specific port */
  }
};

extern "C" void app_main(void) {
  Esp32At25040Spi spi;
  at25040::Device<Esp32At25040Spi> eep(spi);
  ESP_LOGI(TAG, "version %s probe=%d", at25040::GetAt25040DriverVersion(),
           static_cast<int>(eep.Probe()));
  vTaskDelay(pdMS_TO_TICKS(1000));
}
