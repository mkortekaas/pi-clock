// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Per-frame state for one displayed timezone
//

#pragma once

#include "graphics.h"

#include <ctime>
#include <string>

struct TZData {
  std::string tzString;
  std::string tzDisplay;
  std::string timeText;
  struct tm tm;
  rgb_matrix::Color tzColor;
};
