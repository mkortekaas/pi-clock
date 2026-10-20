// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Unit tests for the hardware-independent pieces. Build & run with: make test

#include "clock_utils.h"
#include "FileReader.h"
#include "timezones.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
      failures++;                                                       \
    }                                                                   \
  } while (0)

static struct tm make_tm(int hour, int min, int sec) {
  struct tm t = {};
  t.tm_hour = hour;
  t.tm_min  = min;
  t.tm_sec  = sec;
  return t;
}

static std::string temp_path(const char *name) {
  return std::string("/tmp/pi_clock_test_") + std::to_string(getpid()) + "_" + name;
}

static void write_file(const std::string &path, const std::string &content) {
  std::ofstream out(path);
  out << content;
}

// --- shows_before ---------------------------------------------------------

static void test_later_hour_shows_first() {
  CHECK(shows_before(make_tm(23, 0, 0), "A", make_tm(2, 0, 0), "B"));
  CHECK(!shows_before(make_tm(2, 0, 0), "A", make_tm(23, 0, 0), "B"));
}

static void test_minutes_then_seconds_break_hour_ties() {
  // India-style half-hour offsets share the hour
  CHECK(shows_before(make_tm(5, 30, 0), "A", make_tm(5, 0, 0), "B"));
  CHECK(shows_before(make_tm(5, 0, 10), "A", make_tm(5, 0, 5), "B"));
}

static void test_name_breaks_time_ties_ascending() {
  CHECK(shows_before(make_tm(5, 0, 0), "Asia/Kolkata", make_tm(5, 0, 0), "Asia/Tokyo"));
  CHECK(!shows_before(make_tm(5, 0, 0), "Asia/Tokyo", make_tm(5, 0, 0), "Asia/Kolkata"));
}

static void test_equal_entries_are_not_before_each_other() {
  // strict weak ordering: irreflexive
  CHECK(!shows_before(make_tm(5, 0, 0), "X", make_tm(5, 0, 0), "X"));
}

// --- city_window ----------------------------------------------------------

static void test_city_window_scrolls_one_char_per_second() {
  // "New_York" has 6 three-char positions: 0..5
  CHECK(city_window("America/New_York", 0) == "New");
  CHECK(city_window("America/New_York", 1) == "ew_");
  CHECK(city_window("America/New_York", 5) == "ork");
  CHECK(city_window("America/New_York", 6) == "New");  // wraps
}

static void test_city_window_short_city_shown_whole() {
  CHECK(city_window("Etc/UTC", 42) == "UTC");
  CHECK(city_window("X/AB", 7) == "AB");
}

static void test_city_window_without_slash_uses_whole_name() {
  CHECK(city_window("UTC", 9) == "UTC");
  CHECK(city_window("", 9) == "");
}

static void test_city_window_trailing_slash_is_empty() {
  CHECK(city_window("Europe/", 3) == "");
}

// --- FileReader -----------------------------------------------------------

static void test_file_reader_prefixes_file_content() {
  std::string path = temp_path("temp");
  write_file(path, "72F\n");
  FileReader reader(path, 5, "TEMP ");
  CHECK(reader.getValue() == "TEMP 72F\n");
  unlink(path.c_str());
}

static void test_file_reader_caches_within_interval() {
  std::string path = temp_path("cache");
  write_file(path, "72F\n");
  FileReader reader(path, 5, "");
  write_file(path, "99F\n");
  CHECK(reader.getValue() == "72F\n");
  unlink(path.c_str());
}

static void test_file_reader_rereads_when_interval_is_zero() {
  std::string path = temp_path("reread");
  write_file(path, "72F\n");
  FileReader reader(path, 0, "");
  write_file(path, "99F\n");
  CHECK(reader.getValue() == "99F\n");
  unlink(path.c_str());
}

static void test_file_reader_without_file_returns_prefix() {
  FileReader reader("", 5, "TEMP ");
  CHECK(reader.getValue() == "TEMP ");
}

static void test_file_reader_missing_file_returns_empty() {
  FileReader reader(temp_path("does_not_exist"), 5, "TEMP ");
  CHECK(reader.getValue() == "");
}

// --- get_timezone ---------------------------------------------------------

static void test_get_timezone_prefers_tz_env() {
  setenv("TZ", "Europe/Paris", 1);
  CHECK(get_timezone() == "Europe/Paris");
}

static void test_get_timezone_reads_file_when_tz_unset() {
  std::string path = temp_path("timezone");
  write_file(path, "America/Denver\n");
  unsetenv("TZ");
  CHECK(get_timezone(path.c_str()) == "America/Denver");
  unlink(path.c_str());
}

static void test_get_timezone_empty_when_nothing_found() {
  unsetenv("TZ");
  CHECK(get_timezone(temp_path("no_such_file").c_str()) == "");
}

int main() {
  test_later_hour_shows_first();
  test_minutes_then_seconds_break_hour_ties();
  test_name_breaks_time_ties_ascending();
  test_equal_entries_are_not_before_each_other();
  test_city_window_scrolls_one_char_per_second();
  test_city_window_short_city_shown_whole();
  test_city_window_without_slash_uses_whole_name();
  test_city_window_trailing_slash_is_empty();
  test_file_reader_prefixes_file_content();
  test_file_reader_caches_within_interval();
  test_file_reader_rereads_when_interval_is_zero();
  test_file_reader_without_file_returns_prefix();
  test_file_reader_missing_file_returns_empty();
  test_get_timezone_prefers_tz_env();
  test_get_timezone_reads_file_when_tz_unset();
  test_get_timezone_empty_when_nothing_found();

  if (failures) {
    fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  printf("All tests passed\n");
  return 0;
}
