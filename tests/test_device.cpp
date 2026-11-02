/**
 * @file test_device.cpp
 * @brief Host tests: the driver against the AT25040B model in mock_at25040.hpp.
 */
#include "mock_at25040.hpp"

#include <cstdio>
#include <cstring>

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

void probe_finds_an_idle_chip() {
  MockAt25040 chip;
  Device eep(chip);
  CHECK(eep.Probe() == Status::Ok);
  CHECK(eep.LastRdsr() == 0x00);
}

void probe_reports_absent_on_floating_miso_and_loopback() {
  MockAt25040 chip;
  Device eep(chip);
  chip.bus = MockAt25040::Bus::Floating;
  CHECK(eep.Probe() == Status::Absent);
  chip.bus = MockAt25040::Bus::Loopback; // MISO echoes MOSI one byte late: RDSR reads 0x05
  CHECK(eep.Probe() == Status::Absent);
  chip.bus = MockAt25040::Bus::Broken;
  CHECK(eep.Probe() == Status::Absent);
}

void probe_during_a_write_cycle_reads_0xff() {
  // The datasheet: all status bits read 1 during the internal write cycle, so Probe() sees 0xFF.
  MockAt25040 chip;
  Device eep(chip);
  const uint8_t b = 0x5A;
  CHECK(eep.WritePage(0, &b, 1) == Status::Ok);
  CHECK(eep.Probe() == Status::Absent);
}

void read_puts_a8_in_the_opcode() {
  MockAt25040 chip;
  for (std::size_t i = 0; i < chip.mem.size(); ++i) {
    chip.mem[i] = static_cast<uint8_t>(i ^ (i >> 8));
  }
  Device eep(chip);
  uint8_t buf[4] = {};
  CHECK(eep.Read(0x1FC, buf, 4) == Status::Ok);
  CHECK(chip.last_opcode == 0x0B);
  for (int i = 0; i < 4; ++i) {
    CHECK(buf[i] == chip.mem[0x1FCu + i]);
  }
  CHECK(eep.Read(0x0FC, buf, 4) == Status::Ok);
  CHECK(chip.last_opcode == 0x03);
}

void fill_pattern(MockAt25040& chip) {
  for (std::size_t i = 0; i < chip.mem.size(); ++i) {
    chip.mem[i] = static_cast<uint8_t>((i * 7u) ^ (i >> 8));
  }
}

void read_checks_arguments() {
  MockAt25040 chip;
  Device eep(chip);
  uint8_t buf[32];
  std::memset(buf, 0xAA, sizeof(buf));
  CHECK(eep.Read(0, nullptr, 1) == Status::Arg);
  CHECK(eep.Read(0, buf, 0) == Status::Arg);
  CHECK(eep.Read(0x1FF, buf, 2) == Status::Arg); // would roll over to 0x000
  CHECK(eep.Read(0x200, buf, 1) == Status::Arg);
  CHECK(eep.Read(0x1F0, buf, 32) == Status::Arg);
  CHECK(chip.transfers == 0);
  CHECK(buf[0] == 0xAA);
}

void read_returns_every_byte_requested() {
  // Sequential read: the address increments after each byte (8707F section 6, Read Sequence).
  MockAt25040 chip;
  fill_pattern(chip);
  Device eep(chip);
  uint8_t buf[32];
  std::memset(buf, 0xAA, sizeof(buf));
  CHECK(eep.Read(0, buf, 32) == Status::Ok);
  CHECK(std::memcmp(buf, &chip.mem[0], 32) == 0);
  CHECK(eep.Read(0x1FF, buf, 1) == Status::Ok);
  CHECK(buf[0] == chip.mem[0x1FF]);
}

void read_streams_the_whole_array_across_a8() {
  MockAt25040 chip;
  fill_pattern(chip);
  Device eep(chip);
  uint8_t all[512] = {};
  CHECK(eep.Read(0, all, 512) == Status::Ok);
  CHECK(std::memcmp(all, chip.mem.data(), 512) == 0);
  // A range across 0x0FF/0x100 needs A8 set in the opcode of the chunk that starts at 0x100 or
  // above.
  uint8_t mid[40] = {};
  CHECK(eep.Read(0x0F0, mid, 40) == Status::Ok);
  CHECK(std::memcmp(mid, &chip.mem[0x0F0], 40) == 0);
  CHECK(chip.last_opcode == 0x0B);
}

void read_reports_a_failed_chunk() {
  MockAt25040 chip;
  Device eep(chip);
  chip.bus = MockAt25040::Bus::Broken;
  uint8_t buf[40] = {};
  CHECK(eep.Read(0, buf, 40) == Status::Timeout);
}

void write_page_returns_before_the_write_cycle_ends() {
  MockAt25040 chip;
  Device eep(chip);
  const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CHECK(eep.WritePage(0x108, data, 8) == Status::Ok);
  CHECK(chip.last_opcode == 0x0A);
  CHECK(std::memcmp(&chip.mem[0x108], data, 8) == 0);
  uint8_t sr = 0;
  CHECK(eep.ReadStatus(sr) == Status::Ok);
  CHECK((sr & at25040::kSrWip) != 0); // the caller polls for the end of the write cycle
  for (int i = 0; i < 10 && (sr & at25040::kSrWip) != 0; ++i) {
    CHECK(eep.ReadStatus(sr) == Status::Ok);
  }
  CHECK(sr == 0x00); // ready, WEL cleared by the chip
}

void write_page_rejects_page_crossings() {
  MockAt25040 chip;
  Device eep(chip);
  const uint8_t data[9] = {};
  CHECK(eep.WritePage(0x004, data, 5) == Status::Arg);
  CHECK(eep.WritePage(0x000, data, 9) == Status::Arg);
  CHECK(eep.WritePage(0x000, nullptr, 1) == Status::Arg);
  CHECK(chip.transfers == 0);
}

} // namespace

int main() {
  probe_finds_an_idle_chip();
  probe_reports_absent_on_floating_miso_and_loopback();
  probe_during_a_write_cycle_reads_0xff();
  read_puts_a8_in_the_opcode();
  read_checks_arguments();
  read_returns_every_byte_requested();
  read_streams_the_whole_array_across_a8();
  read_reports_a_failed_chunk();
  write_page_returns_before_the_write_cycle_ends();
  write_page_rejects_page_crossings();
  if (failures != 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
