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
// Poll RDSR until the write cycle ends. tWC is 5 ms max; 100 x 100 us leaves a margin.
at25040::Status wait_ready(Eeprom& eeprom) {
  for (int i = 0; i < 100; ++i) {
    uint8_t sr = 0xFF;
    if (eeprom.ReadStatus(sr) != at25040::Status::Ok) {
      return at25040::Status::Timeout;
    }
    if ((sr & at25040::kSrWip) == 0) {
      return at25040::Status::Ok;
    }
    platform_delay_us(100);
  }
  return at25040::Status::Busy;
}
// --8<-- [end:wait]

// --8<-- [start:read]
// Read() moves at most 16 bytes per call, so read longer blocks in chunks.
at25040::Status read_block(Eeprom& eeprom, uint16_t addr, uint8_t* dst, uint16_t len) {
  while (len > 0) {
    const uint16_t n = len < 16 ? len : 16;
    const at25040::Status st = eeprom.Read(addr, dst, n);
    if (st != at25040::Status::Ok) {
      return st;
    }
    addr = static_cast<uint16_t>(addr + n);
    dst += n;
    len = static_cast<uint16_t>(len - n);
  }
  return at25040::Status::Ok;
}
// --8<-- [end:read]

// --8<-- [start:write]
// WritePage() writes inside one 8-byte page. Split a longer write at page boundaries and wait after
// each page.
at25040::Status write_block(Eeprom& eeprom, uint16_t addr, const uint8_t* src, uint16_t len) {
  while (len > 0) {
    const uint16_t room =
        static_cast<uint16_t>(at25040::kPageSizeBytes - (addr % at25040::kPageSizeBytes));
    const uint16_t n = len < room ? len : room;
    at25040::Status st = eeprom.WritePage(addr, src, n);
    if (st == at25040::Status::Ok) {
      st = wait_ready(eeprom);
    }
    if (st != at25040::Status::Ok) {
      return st;
    }
    addr = static_cast<uint16_t>(addr + n);
    src += n;
    len = static_cast<uint16_t>(len - n);
  }
  return at25040::Status::Ok;
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

// --8<-- [start:raw]
// Block protection (WRSR) is not wrapped by the driver: send it through your port.
// level: 0 none, 1 = 0x180-0x1FF, 2 = 0x100-0x1FF, 3 = whole array. WP must be high.
at25040::Status set_protection(Eeprom& eeprom, MySpi& spi, uint8_t level) {
  if (level > 3) {
    return at25040::Status::Arg;
  }
  if (eeprom.WriteEnable() != at25040::Status::Ok) {
    return at25040::Status::Timeout;
  }
  const uint8_t tx[2] = {at25040::kCmdWrsr, static_cast<uint8_t>(level << 2)};
  uint8_t rx[2] = {};
  if (!spi.transfer(tx, rx, sizeof(tx), 20)) {
    return at25040::Status::Timeout;
  }
  return wait_ready(eeprom); // BP1/BP0 are non-volatile: a WRSR takes a write cycle
}
// --8<-- [end:raw]

// --8<-- [start:errors]
const char* describe(at25040::Status st) {
  switch (st) {
  case at25040::Status::Ok:
    return "ok";
  case at25040::Status::Timeout:
    return "SPI transfer failed";
  case at25040::Status::Absent:
    return "no chip answered";
  case at25040::Status::Busy:
    return "write cycle in progress";
  case at25040::Status::Arg:
    return "bad pointer, length or address";
  }
  return "unknown";
}
// --8<-- [end:errors]
