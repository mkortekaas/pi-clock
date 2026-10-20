// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Hardware-independent helpers (unit tested in test_pi_clock.cc)

#pragma once

#include <ctime>
#include <string>

// Display order: latest local time first, ties broken by timezone name.
// A strict weak ordering, suitable for std::sort.
bool shows_before(const struct tm &lhs_tm, const std::string &lhs_name,
                  const struct tm &rhs_tm, const std::string &rhs_name);

// The few characters of the city (text after the first '/' of the timezone)
// visible at this point; the window scrolls one character per second.
std::string city_window(const std::string &timezone, int seconds);
