/**
 * @file test_bus_errors.cpp
 * @brief A transfer the port reports as failed is Status::BusError, never Status::Timeout:
 * Timeout is kept for a write cycle that did not end within the WaitReady() budget (RDY still 1),
 * and Absent for a bus that answers but not like an AT25040.
 */
#include "mock_at25040.hpp"

#include <cstdio>

namespace {

int failures = 0;

#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond);                         \
      ++failures;                                                                                  \
    }                                                                                              \
  } while (0)

using at25040::Status;
using Device = at25040::Device<MockAt25040>;

void every_call_reports_a_broken_bus_as_bus_error() {
  MockAt25040 chip;
  Device eep(chip);
  chip.bus = MockAt25040::Bus::Broken;
  uint8_t sr = 0;
  uint8_t buf[4] = {};
  const uint8_t b = 0x42;
  at25040::BlockProtect level = at25040::BlockProtect::None;
  CHECK(eep.ReadStatus(sr) == Status::BusError);
  CHECK(eep.Probe() == Status::BusError);
  CHECK(eep.Read(0, buf, 4) == Status::BusError);
  CHECK(eep.WaitReady() == Status::BusError);
  CHECK(eep.WriteEnable() == Status::BusError);
  CHECK(eep.WriteDisable() == Status::BusError);
  CHECK(eep.SetBlockProtection(at25040::BlockProtect::All) == Status::BusError);
  CHECK(eep.GetBlockProtection(level) == Status::BusError);
  CHECK(eep.WritePage(0, &b, 1) == Status::BusError);
  CHECK(eep.Write(0, &b, 1) == Status::BusError);
}

void a_failed_read_chunk_is_a_bus_error() {
  MockAt25040 chip;
  Device eep(chip);
  chip.fail_transfer_at = 2; // RDSR (CheckReady), first READ chunk, then this one
  uint8_t buf[40] = {};
  CHECK(eep.Read(0, buf, 40) == Status::BusError);
}

void a_failed_page_write_is_a_bus_error() {
  const uint8_t b = 0x42;
  for (long at = 0; at < 3; ++at) { // RDSR, WREN, WRITE
    MockAt25040 chip;
    Device eep(chip);
    chip.fail_transfer_at = at;
    CHECK(eep.WritePage(0, &b, 1) == Status::BusError);
  }
  MockAt25040 chip;
  Device eep(chip);
  chip.fail_transfer_at = 3; // the first RDSR poll of the write cycle
  CHECK(eep.WritePage(0, &b, 1) == Status::BusError);
}

void a_write_cycle_that_does_not_end_is_still_a_timeout() {
  MockAt25040 chip;
  Device eep(chip);
  chip.write_cycle_us = 1000000;
  chip.startWriteCycle();
  CHECK(eep.WaitReady(1) == Status::Timeout);
}

void a_floating_bus_is_still_absent() {
  MockAt25040 chip;
  Device eep(chip);
  chip.bus = MockAt25040::Bus::Floating;
  CHECK(eep.Probe() == Status::Absent);
}

} // namespace

int main() {
  every_call_reports_a_broken_bus_as_bus_error();
  a_failed_read_chunk_is_a_bus_error();
  a_failed_page_write_is_a_bus_error();
  a_write_cycle_that_does_not_end_is_still_a_timeout();
  a_floating_bus_is_still_absent();
  if (failures != 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
