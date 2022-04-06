// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//

#include "timezones.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

static const char *timezone_file = "/etc/timezone";

char *get_timezone() {
  char *timezone;
  timezone = getenv("TZ");
  if (timezone != NULL && timezone[0] != '\0')
    return timezone;
  const int fd = open(timezone_file, O_RDONLY);
  if (fd == -1)
    return NULL;
  static char timezone_s[80+1];
  const int n = read(fd, timezone_s, 80);
  close(fd);
  if (n <= 0)
    return NULL;
	char *p = strchr(timezone_s, '\n');
	if (p) *p = '\0';
  return timezone_s;
}

void set_timezone(const char *timezone) {
  setenv("TZ", timezone, 1);
  tzset();
}

