/**
 * @file api_tasks.cpp
 * @brief The API guide's snippets, compiled by the host build (never run).
 */
#include "at25040.hpp"
#include "my_spi.hpp"

#include <cstdio>

using Eeprom = at25040::Device<MySpi>;

// --8<-- [start:probe]
bool attach(Eeprom& eeprom) {
  switch (eeprom.Probe()) {
  case at25040::Status::Ok:
    return true;
  case at25040::Status::Busy:   // a write cycle is still running: try again in 5 ms
  case at25040::Status::Absent: // no chip, floating MISO, MISO echoing MOSI, or a write cycle
                                // (0xFF)
  default:
    std::printf("no AT25040 (RDSR 0x%02X)\n", eeprom.LastRdsr());
    return false;
  }
}
// --8<-- [end:probe]

// --8<-- [start:wait]
// After an MCU reset the chip may still be finishing a write the old firmware started. WaitReady()
// polls RDSR until RDY clears (default budget 10 ms, twice the 5 ms tWC).
bool start_up(Eeprom& eeprom) {
  return eeprom.WaitReady() == at25040::Status::Ok && eeprom.Probe() == at25040::Status::Ok;
}
// --8<-- [end:wait]

// --8<-- [start:read]
// Read() streams any range inside the array; the driver splits it into READ transactions.
at25040::Status read_settings(Eeprom& eeprom, uint8_t (&settings)[64]) {
  return eeprom.Read(0x100, settings, sizeof(settings)); // 0x100-0x13F, A8 set by the driver
}
// --8<-- [end:read]

// --8<-- [start:write]
// Write() splits the range at 8-byte page boundaries and waits for each page's write cycle.
at25040::Status store_serial(Eeprom& eeprom, const uint8_t (&serial)[12]) {
  return eeprom.Write(0x0FC, serial, sizeof(serial)); // pages 0x0F8, 0x100 and 0x108
}
// --8<-- [end:write]

// --8<-- [start:status]
void print_status(Eeprom& eeprom) {
  uint8_t sr = 0;
  if (eeprom.ReadStatus(sr) != at25040::Status::Ok) {
    return;
  }
  const unsigned bp =
      (sr & (at25040::kSrBp1 | at25040::kSrBp0)) >> 2; // 0 none, 1 top 1/4, 2 top 1/2, 3 all
  std::printf("busy=%d write-enabled=%d protect-level=%u\n", (sr & at25040::kSrWip) != 0,
              (sr & at25040::kSrWel) != 0, bp);
}
// --8<-- [end:status]

// --8<-- [start:protect]
// Make the top half (0x100-0x1FF: board ID, calibration) read-only. WP must be high.
bool lock_identity(Eeprom& eeprom) {
  at25040::BlockProtect level = at25040::BlockProtect::None;
  if (eeprom.GetBlockProtection(level) == at25040::Status::Ok &&
      level == at25040::BlockProtect::UpperHalf) {
    return true; // already locked
  }
  if (eeprom.SetBlockProtection(at25040::BlockProtect::UpperHalf) != at25040::Status::Ok) {
    return false; // Busy, Timeout
  }
  // The chip ignores WRSR when WP is low, so read the level back.
  return eeprom.GetBlockProtection(level) == at25040::Status::Ok &&
         level == at25040::BlockProtect::UpperHalf;
}
// --8<-- [end:protect]

// --8<-- [start:disable]
// Clear the write-enable latch after a WREN you sent for your own instruction.
void abandon_write(Eeprom& eeprom) {
  (void)eeprom.WriteDisable(); // WRDI; Busy if a write cycle is still running
}
// --8<-- [end:disable]

// --8<-- [start:errors]
const char* describe(at25040::Status st) {
  switch (st) {
  case at25040::Status::Ok:
    return "ok";
  case at25040::Status::Timeout:
    return "SPI transfer failed, or write cycle too long";
  case at25040::Status::Absent:
    return "no chip answered";
  case at25040::Status::Busy:
    return "write cycle in progress, try again";
  case at25040::Status::Arg:
    return "bad pointer, length or address";
  }
  return "unknown";
}
// --8<-- [end:errors]
