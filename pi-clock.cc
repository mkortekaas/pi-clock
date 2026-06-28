// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Modified from 
//   https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/examples-api-use/clock.cc
//
// This code is public domain - the source file is GPL v2
//  Note also the led-matrix library this depends on is GPL v2

#include "led-matrix.h"
#include "graphics.h"

#include "configuration.h"
#include "timezones.h"
#include "color_temp.h"
#include "FileReader.h"
#include "yaml_config.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [-c <config.yaml>]\n", progname);
  fprintf(stderr, "Displays various timezones.\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "\t-c <config-file> : YAML config file (default: config.yaml)\n");
  return 1;
}

static bool FullSaturation(const Color &c) {
  return (c.r == 0 || c.r == 255)
      && (c.g == 0 || c.g == 255)
      && (c.b == 0 || c.b == 255);
}

void adjust_color(int temp, Color *c)
{
  uint8_t r,g, b;

  k_to_rgb(temp, &r, &g, &b);
  c->r = float(c->r) * (float(r)/256.0);
  c->g = float(c->g) * (float(g)/256.0);
  c->b = float(c->b) * (float(b)/256.0);
}

//
// Sorting helper - Hour & Minute & Second based
//
int time_sorter(const void *a, const void *b) {
  TZData *lhs = (TZData *)a;
  TZData *rhs = (TZData *)b;
  if (lhs->tm.tm_hour != rhs->tm.tm_hour)
      return lhs->tm.tm_hour < rhs->tm.tm_hour;
  if (lhs->tm.tm_min != rhs->tm.tm_min)
      return lhs->tm.tm_min < rhs->tm.tm_min;
  if (lhs->tm.tm_sec != rhs->tm.tm_sec)
      return lhs->tm.tm_sec < rhs->tm.tm_sec;
  // final breaker is the name
  return strcmp(lhs->tzString, rhs->tzString);
}


void paint_airport_code(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, rgb_matrix::Font *font, TZData *this_tz) {
  rgb_matrix::DrawText(canvas, *font, x, y + font->baseline(), this_tz->tzColor, NULL, this_tz->tzDisplay, letter_spacing);
}

void paint_city_name(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, rgb_matrix::Font *font, TZData *this_tz) {
  char *p, city[80+1];
  p = index((char *)this_tz->tzString, '/');
  strncpy(city, p+1, 80);
  int l = strlen(city);
  int offset = (this_tz->tm.tm_min * 60 + this_tz->tm.tm_sec) % (l-2);
  city[offset+3] = '\0';
  rgb_matrix::DrawText(canvas, *font, x, y + font->baseline(), this_tz->tzColor, NULL, &city[offset], letter_spacing);
}

void paint_time(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, rgb_matrix::Font *font, TZData *this_tz) {
  // we handle spaces special - we space them based on space_spacing
  char *token = strtok(this_tz->textBuffer, " ");
  while (token != NULL) {
    rgb_matrix::DrawText(canvas, *font, x, y + font->baseline(), this_tz->tzColor, NULL, token, letter_spacing);
    x += font->CharacterWidth('@') * strlen(token) + space_spacing;
    token = strtok(NULL, " ");
  }
}

void paint_temp(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, rgb_matrix::Font *font, FileReader fileReader, Color color) {
  std::string str = fileReader.getValue();
  std::istringstream iss(str);
  std::string token;
  while (iss >> token) {
    rgb_matrix::DrawText(canvas, *font, x, y + font->baseline(), color, NULL, token.c_str(), letter_spacing);
    x += font->CharacterWidth('@') * token.length() + space_spacing;
  }
}


//
// MAIN
//
int main(int argc, char *argv[]) {
  const char *config_file = "config.yaml";

  int opt;
  while ((opt = getopt(argc, argv, "c:")) != -1) {
    switch (opt) {
    case 'c': config_file = optarg; break;
    default:  return usage(argv[0]);
    }
  }

  PiClockConfig config;
  if (!load_yaml_config(config_file, &config)) {
    fprintf(stderr, "Could not load config from '%s'\n", config_file);
    return 1;
  }

  // Validate
  if (config.layout != 'd' && config.layout != 'l') {
    fprintf(stderr, "Bad layout '%c' in config (only d=Downward, l=Left/Right)\n", config.layout);
    return 1;
  }
  if (config.font_file.empty()) {
    fprintf(stderr, "display.font must be set in config\n");
    return 1;
  }
  if (config.brightness < 1 || config.brightness > 100) {
    fprintf(stderr, "display.brightness must be 1-100\n");
    return 1;
  }

  // Build matrix options from config
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  matrix_options.hardware_mapping = config.hardware_mapping.c_str();
  matrix_options.rows             = config.rows;
  matrix_options.cols             = config.cols;
  matrix_options.chain_length     = config.chain_length;
  matrix_options.parallel         = config.parallel;
  if (!config.pixel_mapper.empty())
    matrix_options.pixel_mapper_config = config.pixel_mapper.c_str();
  runtime_opt.gpio_slowdown = config.gpio_slowdown;

  // Convenience aliases used throughout
  const char *tzFmtStr       = config.date_format.c_str();
  int   x_orig               = config.x_origin;
  int   y_orig               = config.y_origin;
  bool  dim_display          = config.dim_at_night;
  int   dim_start_hour       = config.dim_start_hour;
  int   dim_end_hour         = config.dim_end_hour;
  int   day_start_hour       = config.day_start_hour;
  int   evening_start_hour   = config.evening_start_hour;
  bool  highlight_own_tz     = config.highlight_own_tz;
  int   letter_spacing       = config.letter_spacing;
  int   space_spacing        = config.space_spacing;
  bool  city_name     = config.show_city_name;
  char  layout_choice = config.layout;
  bool  show_temp            = config.show_temp;
  char *temperature_file     = config.temp_file.empty() ? nullptr
                                 : const_cast<char *>(config.temp_file.c_str());
  int   temperature_interval = config.temp_interval;

  // Colors from config
  Color colorOvernight(config.color_overnight.r, config.color_overnight.g, config.color_overnight.b);
  Color colorDay      (config.color_day.r,       config.color_day.g,       config.color_day.b);
  Color colorEvening  (config.color_evening.r,   config.color_evening.g,   config.color_evening.b);
  Color colorDimmed   (config.color_dimmed.r,    config.color_dimmed.g,    config.color_dimmed.b);
  Color colorOwnTz    (config.color_own_tz.r,    config.color_own_tz.g,    config.color_own_tz.b);
  Color colorTemp     (config.color_temp.r,      config.color_temp.g,      config.color_temp.b);

  // background color will be black
  Color bg_color(0, 0, 0);

  FileReader temperatureFileReader(temperature_file, temperature_interval, config.temp_prefix);

  rgb_matrix::Font* font = new rgb_matrix::Font();
  if (!font->LoadFont(config.font_file.c_str())) {
    fprintf(stderr, "Couldn't load font '%s'\n", config.font_file.c_str());
    return 1;
  }

  if (config.cities.empty()) {
    fprintf(stderr, "No cities defined in config\n");
    return 1;
  }
  {
    size_t n = config.cities.size();
    TZDataWanted *p = (TZDataWanted *)malloc(sizeof(TZDataWanted) * (n + 1));
    for (size_t i = 0; i < n; i++) {
      p[i].tzString  = strdup(config.cities[i].timezone.c_str());
      p[i].tzDisplay = strdup(config.cities[i].display.c_str());
    }
    p[n].tzString  = nullptr;
    p[n].tzDisplay = nullptr;
    tz_wanted     = p;
    tz_wanted_len = n;
  }

  RGBMatrix *matrix = rgb_matrix::CreateMatrixFromOptions(matrix_options,
                                                          runtime_opt);
  if (matrix == NULL)
    return 1;

  fprintf(stderr, "Matrix info: Total W&H=(%d,%d) build from (%d,%d)*%d\n",
    matrix->width(), matrix->height(),
    matrix_options.cols, matrix_options.rows,
    matrix_options.chain_length);

  matrix->SetBrightness(config.brightness);

  if (!dim_display && config.brightness == 100 && FullSaturation(colorDimmed) && FullSaturation(bg_color))
    matrix->SetPWMBits(1);

  FrameCanvas *offscreen = matrix->CreateFrameCanvas();

  // The time - we cheat and only get it once and expect clock_nanosleep() to keep us kosher
  struct timespec next_time;
  next_time.tv_sec = time(NULL);
  next_time.tv_nsec = 0;

  const char *my_timezone = get_timezone();

  size_t tz_length = tz_wanted_len;
  TZData *tz = (TZData *)malloc(sizeof(TZData)*tz_length);
  for (size_t ii=0;ii<tz_length;ii=ii+1) {
    tz[ii].textBuffer = (char *)malloc(80+1);
    tz[ii].tzString = tz_wanted[ii].tzString;
    tz[ii].tzDisplay = tz_wanted[ii].tzDisplay;
    tz[ii].isMyTimezone = (strcmp(my_timezone, tz[ii].tzString) == 0) ? true : false;
  }

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {

    bool is_dimmed = false;

    if (dim_display) {
      struct tm my_tm;

      set_timezone(my_timezone);
      localtime_r(&next_time.tv_sec, &my_tm);
      is_dimmed = (my_tm.tm_hour < dim_end_hour || my_tm.tm_hour >= dim_start_hour);
      // uint8_t new_brightness = is_dimmed ? brightness/2 : brightness;
      // if (new_brightness != matrix->brightness()) {
      //   matrix->SetBrightness(new_brightness);
      // }
    }

    offscreen->Clear();
    offscreen->Fill(bg_color.r, bg_color.g, bg_color.b);

    for (size_t ii=0;ii<tz_length;ii=ii+1) {
      set_timezone(tz[ii].tzString);
      localtime_r(&next_time.tv_sec, &tz[ii].tm);
      strftime(tz[ii].textBuffer, 80, tzFmtStr, &tz[ii].tm);
      if (dim_display && is_dimmed) {
        tz[ii].tzColor = colorDimmed;
      } else if (highlight_own_tz && tz[ii].isMyTimezone) {
        tz[ii].tzColor = colorOwnTz;
      } else {
        int h = tz[ii].tm.tm_hour;
        tz[ii].tzColor = (h >= evening_start_hour) ? colorEvening
                       : (h >= day_start_hour)     ? colorDay
                                                   : colorOvernight;
      }
    }

    // set_timezone("UTC");

    // Sort in place - no need to worry about initial order; it's irrelvant
    // Plus, as the order only changes every hour (or 1/2 for India), the sort will be nothing most of the time
    qsort((void *)tz, tz_length, sizeof(TZData), time_sorter);

    // == keeping in mind row is spread across 2 panels
    // == if you change font/sizing you will need to play with spacing

    int x = x_orig;
    int y = y_orig;
    int left_right = 0;

    // paint the screen but don't display it till after the loop
    // we are actually painting the upcomming time vs the time now

    for (size_t ii=0;ii<tz_length;ii=ii+1) {
      if (city_name)
        paint_city_name(x, y, letter_spacing, space_spacing, offscreen, font, &tz[ii]);
      else
        paint_airport_code(x, y, letter_spacing, space_spacing, offscreen, font, &tz[ii]);

      x += font->CharacterWidth('@') * strlen(tz[ii].tzDisplay) + space_spacing;
      paint_time(x, y, letter_spacing, space_spacing, offscreen, font, &tz[ii]);

      // deal with layout 
      switch (layout_choice) {
        case 'd':
          // simple - just head downwards! - don't change x
          x = x_orig;
          y += font->height();
          break;

        case 'l':
          // only increment y every other entry; but x swaps around a lot
          if (left_right) {
            // move down
            x = x_orig;
            y += font->height();
          } else {
            // move right -- you want this to start at the halfway point of the total width
            x = matrix->width()/2 + x_orig;
          }
          left_right = !left_right;
          break;

        default:
          // Can't happen (ha!)
          break;
      }
    }

    // if we are displaying temp as the last line
    if (show_temp == true) {
      Color tempColor = (dim_display && is_dimmed) ? colorDimmed : colorTemp;
      paint_temp(x, y, letter_spacing, space_spacing, offscreen, font, temperatureFileReader, tempColor);
    }

    // Wait until we're ready to show it.
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_time, NULL);

    // Atomic swap with double buffer
    offscreen = matrix->SwapOnVSync(offscreen);

    // adjust second clock for all timezones and repeat
    next_time.tv_sec += 1;
  }

  // Finished. Shut down the RGB matrix.
  matrix->Clear();
  delete matrix;

  fprintf(stderr, "\n");  // Create a fresh new line after ^C on screen
  return 0;
}
