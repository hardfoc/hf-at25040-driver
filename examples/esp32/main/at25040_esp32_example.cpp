/**
 * @file at25040_esp32_example.cpp
 * @brief AT25040B on an ESP32-C6 with the ESP-IDF spi_master driver.
 *
 * The port below is complete: one full-duplex spi_master transaction per transfer() (CS held low
 * for the whole call), SPI mode 0, MSB first. The driver builds every instruction itself, including
 * the READ / WRITE opcode with address bit A8 in bit 3 (datasheet 8707F, Table 6-1), so the port
 * does not use the ESP-IDF command or address phases: tx[0] is the opcode, tx[1] the address byte.
 *
 * Wiring (ESP32-C6 DevKitC, the pins of the ESP-IDF EEPROM example), 3.3 V:
 *   SCK GPIO6, SI (MOSI) GPIO7, SO (MISO) GPIO2, CS GPIO10, WP and HOLD to VCC.
 */
#include "at25040.hpp"

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstring>

namespace {

const char* TAG = "at25040";

constexpr spi_host_device_t kHost = SPI2_HOST;
constexpr int kPinSclk = 6;
constexpr int kPinMosi = 7;
constexpr int kPinMiso = 2;
constexpr int kPinCs = 10;
constexpr int kClockHz = 5 * 1000 * 1000; // 10 MHz max at 2.5 V to 5.5 V (Table 4-3)

/** ESP-IDF spi_master port for at25040::Device. */
class Esp32At25040Spi : public at25040::SpiInterface<Esp32At25040Spi> {
public:
  /** Initialise the bus and add the EEPROM as a device. Call once before using the driver. */
  esp_err_t Init() noexcept {
    spi_bus_config_t bus = {};
    bus.mosi_io_num = kPinMosi;
    bus.miso_io_num = kPinMiso;
    bus.sclk_io_num = kPinSclk;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = 64; // the driver never sends more than 2 + 16 bytes
    esp_err_t err = spi_bus_initialize(kHost, &bus, SPI_DMA_DISABLED);
    if (err != ESP_OK) {
      return err;
    }
    spi_device_interface_config_t dev = {};
    dev.mode = 0; // CPOL 0, CPHA 0; the chip also supports mode 3
    dev.clock_speed_hz = kClockHz;
    dev.spics_io_num = kPinCs;
    dev.queue_size = 1;
    dev.cs_ena_pretrans = 1;  // tCSS >= 100 ns: one SCK period at 5 MHz is 200 ns
    dev.cs_ena_posttrans = 1; // tCSH >= 100 ns
    return spi_bus_add_device(kHost, &dev, &dev_);
  }

  /**
   * One CS-low transaction of `len` bytes, full duplex: rx[i] is the byte clocked in while tx[i]
   * went out. Polling mode: at most 18 bytes, a few tens of microseconds, so the driver's
   * `timeout_ms` is not needed here.
   */
  bool transfer(const uint8_t* tx, uint8_t* rx, std::size_t len, uint32_t timeout_ms) noexcept {
    (void)timeout_ms;
    if (dev_ == nullptr || len == 0) {
      return false;
    }
    spi_transaction_t t = {};
    t.length = len * 8; // bits
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    const esp_err_t err = spi_device_polling_transmit(dev_, &t);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "SPI transfer failed: %s", esp_err_to_name(err));
      return false;
    }
    return true;
  }

  /** Optional timing hook: WaitReady() sleeps here (100 us) between status polls. */
  void delay_us(uint32_t us) noexcept {
    esp_rom_delay_us(us);
  }

private:
  spi_device_handle_t dev_ = nullptr;
};

const char* StatusName(at25040::Status st) {
  switch (st) {
  case at25040::Status::Ok:
    return "Ok";
  case at25040::Status::Timeout:
    return "Timeout";
  case at25040::Status::Absent:
    return "Absent";
  case at25040::Status::Busy:
    return "Busy";
  case at25040::Status::Arg:
    return "Arg";
  }
  return "?";
}

Esp32At25040Spi spi;                         // the bus is only touched from app_main()
uint8_t image[at25040::kCapacityBytes] = {}; // whole-array read buffer, off the task stack

} // namespace

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "AT25040 driver %s", at25040::GetAt25040DriverVersion());
  const esp_err_t err = spi.Init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "SPI init failed: %s", esp_err_to_name(err));
    return;
  }

  at25040::Device<Esp32At25040Spi> eeprom(spi);

  // A write the previous firmware started may still be running after a reset.
  (void)eeprom.WaitReady();
  const at25040::Status probe = eeprom.Probe();
  ESP_LOGI(TAG, "Probe: %s (RDSR 0x%02X)", StatusName(probe), eeprom.LastRdsr());
  if (probe != at25040::Status::Ok) {
    ESP_LOGW(TAG, "no AT25040B: check SO/SI/SCK/CS wiring and VCC; WP and HOLD high");
    return;
  }

  at25040::BlockProtect level = at25040::BlockProtect::None;
  if (eeprom.GetBlockProtection(level) == at25040::Status::Ok) {
    ESP_LOGI(TAG, "block protection level %u", static_cast<unsigned>(level));
  }

  // 12 bytes at 0x0FC: three pages (0x0F8, 0x100, 0x108), on both sides of the A8 boundary.
  uint8_t data[12];
  for (std::size_t i = 0; i < sizeof(data); ++i) {
    data[i] = static_cast<uint8_t>(0xA0u + i);
  }
  at25040::Status st = eeprom.Write(0x0FC, data, sizeof(data));
  ESP_LOGI(TAG, "Write(0x0FC, 12): %s", StatusName(st));

  uint8_t back[sizeof(data)] = {};
  st = eeprom.Read(0x0FC, back, sizeof(back));
  const bool same = std::memcmp(back, data, sizeof(data)) == 0;
  ESP_LOGI(TAG, "Read(0x0FC, 12): %s, %s", StatusName(st),
           same ? "data matches" : "data differs (WP low or block protected?)");

  st = eeprom.Read(0, image, sizeof(image));
  ESP_LOGI(TAG, "Read(0, 512): %s", StatusName(st));
  if (st == at25040::Status::Ok) {
    ESP_LOG_BUFFER_HEX(TAG, &image[0x0F0], 32);
  }

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
