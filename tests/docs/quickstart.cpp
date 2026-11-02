/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_spi.hpp"

// --8<-- [start:quickstart]
#include "at25040.hpp"

int store_and_read_back(MySpi& spi) {
  at25040::Device<MySpi> eeprom(spi);

  if (eeprom.Probe() != at25040::Status::Ok) { // 1. a chip is there and idle
    return -1;                                 // wiring, CS, power or SPI mode
  }

  const uint8_t serial[4] = {0x12, 0x34, 0x56, 0x78};
  if (eeprom.WritePage(0x010, serial, sizeof(serial)) != at25040::Status::Ok) { // 2. one page
    return -2;
  }

  uint8_t sr = 0xFF; // 3. wait for the write cycle (5 ms max)
  for (int i = 0; i < 100 && (sr & at25040::kSrWip) != 0; ++i) {
    platform_delay_us(100);
    if (eeprom.ReadStatus(sr) != at25040::Status::Ok) {
      return -3;
    }
  }

  uint8_t back[4] = {}; // 4. read it back
  if (eeprom.Read(0x010, back, sizeof(back)) != at25040::Status::Ok) {
    return -4;
  }
  return back[0] == serial[0] ? 0 : -5;
}
// --8<-- [end:quickstart]
