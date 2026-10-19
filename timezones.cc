// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//

#include "timezones.h"

#include <fstream>
#include <stdlib.h>
#include <time.h>

std::string get_timezone(const char *timezone_file) {
  const char *env_tz = getenv("TZ");
  if (env_tz != NULL && env_tz[0] != '\0')
    return env_tz;

  std::ifstream file(timezone_file);
  std::string timezone;
  std::getline(file, timezone);
  return timezone;
}

void set_timezone(const std::string &timezone) {
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}
