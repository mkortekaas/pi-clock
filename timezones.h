// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-

#pragma once

#include <string>

// The system's own timezone: $TZ if set, else the first line of timezone_file.
// Empty when neither is available.
std::string get_timezone(const char *timezone_file = "/etc/timezone");
void set_timezone(const std::string &timezone);
