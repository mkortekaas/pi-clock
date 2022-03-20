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
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Displays various timezones.\n");
  fprintf(stderr, "Options:\n");
  rgb_matrix::PrintMatrixFlags(stderr);
  fprintf(stderr,
          "\t-f <font-file>    : Use given font.\n"
          "\t-b <brightness>   : Sets brightness percent. Default: 100.\n"
          "\t-x <x-origin>     : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin>     : Y-Origin of displaying text (Default: 0)\n"
          "\t-G <spacing>      : Gap between columns in pixels (Default: 4)\n"
          "\t-d [d|l|m]        : Layout is:  D(own) or L(eft/right) or M(ark)\n"
          "\t-c                : Show city name vs airport code\n"
          "\t-s <spacing>      : Spacing pixels between words (Default: 2)\n"
          "\t-S <spacing>      : Spacing pixels between letters (Default: 0)\n"
          "\t-F <date-format>  : Date format (Default is HH:MM:SS via %%H:%%M:%%S)\n"
          );

  return 1;
}

static bool FullSaturation(const Color &c) {
    return (c.r == 0 || c.r == 255)
        && (c.g == 0 || c.g == 255)
        && (c.b == 0 || c.b == 255);
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

void set_timezone(const char *timezone) {
  setenv("TZ", timezone, 1);
  tzset();
}

//
// Set color based on hour
//
Color tzColorSet(int hour, Color overnight, Color day, Color evening) {
  Color retVal = overnight;
  if (hour > 7) retVal = day;
  if (hour > 17) retVal = evening;
  return retVal;
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

//
// MAIN
//
int main(int argc, char *argv[]) {
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;
  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                         &matrix_options, &runtime_opt)) {
    return usage(argv[0]);
  }

  // time format - default to H:M:S
  //   Probably should make this an argument...
  const char *tzFmtStr = "%H:%M:%S";

  // Create a number of base colors to utilize
  Color colorRed(255, 0, 0);
  Color colorGreen(0, 255, 0);
  Color colorBlue(0, 0, 255);
  Color colorWhite(255, 255, 255);
  Color colorRedGreen(255, 255, 0);
  Color colorGreenBlue(0, 255, 255);

  // background color will be black 
  Color bg_color(0, 0, 0);

  char *bdf_font_file = NULL;
  int x_orig = 0;
  int y_orig = 0;
  int brightness = 100;
  int letter_spacing = 0;
  int space_spacing = 2;
  int col_gap_spacing = 4;
  int city_name = 0;
  char layout_choice = 'd';
  char date_format[80+1] = "";

  int opt;
  while ((opt = getopt(argc, argv, "x:y:f:b:S:G:cd:s:F:")) != -1) {
    switch (opt) {
    case 'b': brightness = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'S': letter_spacing = atoi(optarg); break;
    case 'c': city_name = 1; break;
    case 'd': layout_choice = optarg[0]; break;
    case 's': space_spacing = atoi(optarg); break;
    case 'G': col_gap_spacing = atoi(optarg); break;
    case 'F': strncpy(date_format, optarg, 80); break;
    default:
      return usage(argv[0]);
    }
  }

  if (date_format[0]) {
    tzFmtStr = date_format;
  }


  if (layout_choice != 'd' && layout_choice != 'l' && layout_choice != 'm') {
    fprintf(stderr, "Bad -d option (only d=Downward, l=Left/Right, m=Mark's special)\n");
    return usage(argv[0]);
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font* font = new rgb_matrix::Font();
  if (!font->LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return 1;
  }
  if (brightness < 1 || brightness > 100) {
    fprintf(stderr, "Brightness is outside usable range.\n");
    return 1;
  }

  RGBMatrix *matrix = rgb_matrix::CreateMatrixFromOptions(matrix_options,
                                                          runtime_opt);
  if (matrix == NULL)
    return 1;

  matrix->SetBrightness(brightness);

  const bool all_extreme_colors = (brightness == 100)
      && FullSaturation(colorRed)
    && FullSaturation(bg_color);
  if (all_extreme_colors)
      matrix->SetPWMBits(1);

  FrameCanvas *offscreen = matrix->CreateFrameCanvas();

  // The time - we cheet and only get it once and expect clock_nanosleep() to keep us kosher
  struct timespec next_time;
  next_time.tv_sec = time(NULL);
  next_time.tv_nsec = 0;

  size_t tz_length = tz_wanted_len;
  TZData *tz = (TZData *)malloc(sizeof(TZData)*tz_length);
  for (size_t ii=0;ii<tz_length;ii=ii+1) {
    tz[ii].textBuffer = (char *)malloc(80+1);
    tz[ii].tzString = tz_wanted[ii].tzString;
    tz[ii].tzDisplay = tz_wanted[ii].tzDisplay;
  }

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {

    offscreen->Fill(bg_color.r, bg_color.g, bg_color.b);
    offscreen->Clear();

    set_timezone("UTC");

    for (size_t ii=0;ii<tz_length;ii=ii+1) {
        set_timezone(tz[ii].tzString);
        localtime_r(&next_time.tv_sec, &tz[ii].tm);
        strftime(tz[ii].textBuffer, 80, tzFmtStr, &tz[ii].tm);
        tz[ii].tzColor = tzColorSet(tz[ii].tm.tm_hour, colorBlue, colorWhite, colorRed);
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
            // move right
            x += font->CharacterWidth('@') * strlen(tz[ii].tzDisplay) + space_spacing;
            x += font->CharacterWidth('@') * strlen(tz[ii].textBuffer) + col_gap_spacing;
          }
          left_right = !left_right;
          break;

        case 'm':
          // Mark's strange setup - which should be fixed with --led-pixel-mapper= argument
          //
          // in this situation the hardware sees the boards as left/right but they are physically
          // mounted top/bottom - so the first 3 displays are correct and then reset the X position
          if (ii == 2) {
            x = 64 + x_orig;
            y = y_orig;
          } else if (ii > 2) {
            x = 64 + x_orig;
            y += font->height();
          } else {
            x = x_orig;
            y += font->height();
          }
          break;

        default:
          // Can't happen (ha!)
          break;
       }
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
