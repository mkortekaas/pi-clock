// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-

#include "clock_utils.h"

#include <tuple>

static const size_t CITY_WINDOW_CHARS = 3;

bool shows_before(const struct tm &lhs_tm, const std::string &lhs_name,
                  const struct tm &rhs_tm, const std::string &rhs_name) {
  // Times compare descending (swapped sides), names ascending
  return std::tie(rhs_tm.tm_hour, rhs_tm.tm_min, rhs_tm.tm_sec, lhs_name)
       < std::tie(lhs_tm.tm_hour, lhs_tm.tm_min, lhs_tm.tm_sec, rhs_name);
}

std::string city_window(const std::string &timezone, int seconds) {
  const size_t slash = timezone.find('/');
  const std::string city = (slash == std::string::npos) ? timezone : timezone.substr(slash + 1);
  if (city.size() <= CITY_WINDOW_CHARS)
    return city;

  const size_t positions = city.size() - CITY_WINDOW_CHARS + 1;
  return city.substr(static_cast<size_t>(seconds) % positions, CITY_WINDOW_CHARS);
}
