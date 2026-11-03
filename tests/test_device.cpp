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
  chip.bus = MockAt25040::Bus::Broken; // the port reports the failure: not "absent"
  CHECK(eep.Probe() == Status::BusError);
}

void probe_during_a_write_cycle_reads_0xff() {
  // The datasheet: all status bits read 1 during the internal write cycle, so Probe() sees 0xFF.
  MockAt25040 chip;
  Device eep(chip);
  chip.startWriteCycle();
  CHECK(eep.Probe() == Status::Absent);
  CHECK(eep.LastRdsr() == 0xFF);
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
  CHECK(eep.Read(0, buf, 40) == Status::BusError);
}

void write_page_waits_for_the_write_cycle() {
  // Programming starts on CS high; RDY stays 1 for up to tWC (8707F section 6, Table 4-3).
  MockAt25040 chip;
  Device eep(chip);
  const uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CHECK(eep.WritePage(0x108, data, 8) == Status::Ok);
  CHECK(chip.writes == 1);
  CHECK(std::memcmp(&chip.mem[0x108], data, 8) == 0);
  CHECK(!chip.busy()); // the write cycle is over when WritePage() returns
  CHECK(chip.now_ns >= 3000000u);
  uint8_t sr = 0xFF;
  CHECK(eep.ReadStatus(sr) == Status::Ok);
  CHECK(sr == 0x00); // ready, WEL cleared by the chip
  CHECK(chip.ignored == 0);
}

void write_page_sleeps_through_the_port_delay_hook() {
  MockAt25040Timed chip;
  at25040::Device<MockAt25040Timed> eep(chip);
  const uint8_t b = 0x42;
  const std::size_t before = chip.transfers;
  CHECK(eep.WritePage(0x010, &b, 1) == Status::Ok);
  CHECK(!chip.busy());
  CHECK(chip.delays > 0);
  CHECK(chip.transfers - before < 40); // ~3 ms in 100 us steps, not a busy loop
  CHECK(chip.mem[0x010] == 0x42);
}

void wait_ready_times_out_on_a_stuck_write_cycle() {
  MockAt25040Timed chip;
  at25040::Device<MockAt25040Timed> eep(chip);
  chip.write_cycle_us = 1000000; // never ends within the budget
  chip.startWriteCycle();
  CHECK(eep.WaitReady() == Status::Timeout);
  CHECK(chip.busy());
  CHECK(chip.now_ns >= 10000000u); // waited the whole kWriteCycleTimeoutMs
  CHECK(chip.now_ns < 20000000u);

  MockAt25040 untimed; // no delay hook: a bounded number of polls
  Device eep2(untimed);
  untimed.write_cycle_us = 1000000;
  untimed.startWriteCycle();
  CHECK(eep2.WaitReady(1) == Status::Timeout);
  CHECK(untimed.transfers == Device::kPollsPerMsWithoutDelay + 1u);

  MockAt25040 floating; // MISO pulled up: RDY reads 1 forever
  Device eep3(floating);
  floating.bus = MockAt25040::Bus::Floating;
  CHECK(eep3.WaitReady(1) == Status::Timeout);
  floating.bus = MockAt25040::Bus::Broken;
  CHECK(eep3.WaitReady() == Status::BusError);
  CHECK(eep3.WaitReady(0) == Status::BusError); // one poll at least
}

void write_page_reports_a_write_cycle_that_does_not_end() {
  MockAt25040 chip;
  Device eep(chip);
  chip.write_cycle_us = 1000000;
  const uint8_t b = 0x42;
  CHECK(eep.WritePage(0, &b, 1) == Status::Timeout);
}

void write_splits_at_page_boundaries() {
  // 8-byte pages: only A2:A0 increment during a WRITE (8707F section 6, Write Sequence).
  MockAt25040 chip;
  Device eep(chip);
  uint8_t data[20];
  for (int i = 0; i < 20; ++i) {
    data[i] = static_cast<uint8_t>(0x80 + i);
  }
  CHECK(eep.Write(0x0FC, data, 20) == Status::Ok); // 0x0FC-0x10F: pages 0x0F8, 0x100, 0x108
  CHECK(chip.writes == 3);
  CHECK(chip.ignored == 0);
  CHECK(std::memcmp(&chip.mem[0x0FC], data, 20) == 0);
  CHECK(chip.mem[0x0FB] == 0xFF);
  CHECK(chip.mem[0x110] == 0xFF);
  CHECK(chip.max_len <= 2u + at25040::kPageSizeBytes);

  CHECK(eep.Write(0, nullptr, 1) == Status::Arg);
  CHECK(eep.Write(0, data, 0) == Status::Arg);
  CHECK(eep.Write(0x1FC, data, 5) == Status::Arg);
  CHECK(chip.writes == 3);
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

void operations_report_busy_during_a_write_cycle() {
  // During a write cycle only RDSR is accepted and it reads 0xFF (8707F section 6, Table 6-3).
  MockAt25040 chip;
  fill_pattern(chip);
  Device eep(chip);
  chip.write_cycle_us = 1000000;
  chip.startWriteCycle(); // e.g. left running by an MCU reset
  uint8_t buf[4];
  std::memset(buf, 0xAA, sizeof(buf));
  const uint8_t data[2] = {1, 2};
  at25040::BlockProtect level = at25040::BlockProtect::All;
  CHECK(eep.Read(0, buf, 4) == Status::Busy);
  CHECK(buf[0] == 0xAA);
  CHECK(eep.WritePage(0, data, 2) == Status::Busy);
  CHECK(eep.Write(0, data, 2) == Status::Busy);
  CHECK(eep.WriteEnable() == Status::Busy);
  CHECK(eep.WriteDisable() == Status::Busy);
  CHECK(eep.SetBlockProtection(at25040::BlockProtect::None) == Status::Busy);
  CHECK(eep.GetBlockProtection(level) == Status::Busy);
  CHECK(level == at25040::BlockProtect::All);
  CHECK(chip.ignored == 0); // nothing but RDSR went out
  CHECK(chip.writes == 1);

  chip.now_ns = chip.busy_until_ns; // the write cycle ends
  CHECK(eep.Read(0, buf, 4) == Status::Ok);
  CHECK(std::memcmp(buf, &chip.mem[0], 4) == 0);
}

void write_disable_clears_the_latch() {
  // WRDI, Table 6-1: 0000 X100, resets the write enable latch.
  MockAt25040 chip;
  Device eep(chip);
  uint8_t sr = 0;
  CHECK(eep.WriteEnable() == Status::Ok);
  CHECK(eep.ReadStatus(sr) == Status::Ok);
  CHECK(sr == at25040::kSrWel);
  CHECK(eep.WriteDisable() == Status::Ok);
  CHECK(chip.last_opcode == 0x04);
  CHECK(eep.ReadStatus(sr) == Status::Ok);
  CHECK(sr == 0x00);
  chip.bus = MockAt25040::Bus::Broken;
  CHECK(eep.WriteDisable() == Status::BusError);
}

void block_protection_round_trip() {
  // WRSR sets BP1:BP0 (Table 6-4) and takes a write cycle: the bits are non-volatile.
  MockAt25040 chip;
  Device eep(chip);
  at25040::BlockProtect level = at25040::BlockProtect::All;
  CHECK(eep.GetBlockProtection(level) == Status::Ok);
  CHECK(level == at25040::BlockProtect::None);

  CHECK(eep.SetBlockProtection(at25040::BlockProtect::UpperHalf) == Status::Ok);
  CHECK(chip.bp == 2);
  CHECK(chip.writes == 1);
  CHECK(!chip.busy()); // waited for the WRSR write cycle
  CHECK(eep.GetBlockProtection(level) == Status::Ok);
  CHECK(level == at25040::BlockProtect::UpperHalf);

  const uint8_t b = 0x11;
  CHECK(eep.WritePage(0x100, &b, 1) == Status::Ok); // ignored by the chip: protected
  CHECK(chip.mem[0x100] == 0xFF);
  CHECK(eep.WritePage(0x0FF, &b, 1) == Status::Ok);
  CHECK(chip.mem[0x0FF] == 0x11);

  CHECK(eep.SetBlockProtection(at25040::BlockProtect::UpperQuarter) == Status::Ok);
  CHECK(chip.bp == 1);
  CHECK(eep.SetBlockProtection(at25040::BlockProtect::All) == Status::Ok);
  CHECK(chip.bp == 3);
  CHECK(eep.SetBlockProtection(at25040::BlockProtect::None) == Status::Ok);
  CHECK(chip.bp == 0);

  const std::size_t before = chip.transfers;
  CHECK(eep.SetBlockProtection(static_cast<at25040::BlockProtect>(4)) == Status::Arg);
  CHECK(chip.transfers == before);
  chip.bus = MockAt25040::Bus::Broken;
  CHECK(eep.SetBlockProtection(at25040::BlockProtect::All) == Status::BusError);
  CHECK(eep.GetBlockProtection(level) == Status::BusError);
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
  write_page_waits_for_the_write_cycle();
  write_page_sleeps_through_the_port_delay_hook();
  wait_ready_times_out_on_a_stuck_write_cycle();
  write_page_reports_a_write_cycle_that_does_not_end();
  write_splits_at_page_boundaries();
  operations_report_busy_during_a_write_cycle();
  write_disable_clears_the_latch();
  block_protection_round_trip();
  write_page_rejects_page_crossings();
  if (failures != 0) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
