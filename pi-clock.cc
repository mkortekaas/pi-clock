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
          "\t-d [d|l]          : Layout is: D(own) or L(eft/right)\n"
          "\t-c                : Show city name vs airport code\n"
          "\t-C <file>         : city config filename\n"
          "\t-s <spacing>      : Spacing pixels between words (Default: 2)\n"
          "\t-S <spacing>      : Spacing pixels between letters (Default: 0)\n"
          "\t-F <date-format>  : Date format (Default is HH:MM:SS via %%H:%%M:%%S)\n"
          "\t-D                : Dim brightness downward at night\n"
          "\t-H                : Highlight your own timezone\n"
          );

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

void paint_temp(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, rgb_matrix::Font *font, int temp) {
  Color colorGreen(0, 255, 0);
  char token[12];
  sprintf(token, "%d", temp);
  while (token != NULL) {
    rgb_matrix::DrawText(canvas, *font, x, y + font->baseline(), colorGreen, NULL, token, letter_spacing);
    x += font->CharacterWidth('@') * strlen(token) + space_spacing;
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

  // background color will be black 
  Color bg_color(0, 0, 0);

  char *bdf_font_file = NULL;
  int x_orig = 0;
  int y_orig = 0;
  uint8_t brightness = 100;
  bool dim_display = false;
  bool highlight_own_tz = false;
  int letter_spacing = 0;
  int space_spacing = 2;
  bool city_name = false;
  char layout_choice = 'd';
  char *date_format = NULL;
  char *city_file = NULL;

  int opt;
  while ((opt = getopt(argc, argv, "x:y:f:b:S:cC:d:s:F:DH")) != -1) {
    switch (opt) {
    case 'b': brightness = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'S': letter_spacing = atoi(optarg); break;
    case 'c': city_name = true; break;
    case 'C': city_file = strdup(optarg); break;
    case 'd': layout_choice = optarg[0]; break;
    case 's': space_spacing = atoi(optarg); break;
    case 'F': date_format = strdup(optarg); break;
    case 'D': dim_display = true; break;
    case 'H': highlight_own_tz = true; break;
    default:
      return usage(argv[0]);
    }
  }

  if (date_format && strlen(date_format) > 0) {
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

  // This inits the city list from built in code if no file provided..
  if (read_city_list(city_file) == 0) {
    fprintf(stderr, "Couldn't load city list file '%s'\n", city_file);
    return 1;
  }

  RGBMatrix *matrix = rgb_matrix::CreateMatrixFromOptions(matrix_options,
                                                          runtime_opt);
  if (matrix == NULL)
    return 1;

  fprintf(stderr, "Matrix info: Total W&H=(%d,%d) build from (%d,%d)*%d\n",
    matrix->width(), matrix->height(),
    matrix_options.cols, matrix_options.rows,
    matrix_options.chain_length);

  matrix->SetBrightness(brightness);

  if (!dim_display && brightness == 100 && FullSaturation(colorRed) && FullSaturation(bg_color))
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

      // Because this runs 24/7 we dim the display at night - kinda like an iPhone's Night Shift mode
      // Ok, so by default, Night Shift turns on from sunset to sunrise - we dont' know when that is (currently)
      // So we pick 7am to 7pm for now for daylight hours

      set_timezone(my_timezone);
      localtime_r(&next_time.tv_sec, &my_tm);
      is_dimmed = (my_tm.tm_hour < 7 || my_tm.tm_hour >= 19) ? true : false;
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
        if (highlight_own_tz && tz[ii].isMyTimezone)
          tz[ii].tzColor = tzColorSet(tz[ii].tm.tm_hour, colorGreen, colorGreen, colorGreen);
        else
          tz[ii].tzColor = tzColorSet(tz[ii].tm.tm_hour, colorBlue, colorWhite, colorRed);
        if (dim_display) {
	  // Choosing index 0 and 3 is a first stab
          adjust_color(k_tempratures[is_dimmed?0:3], &tz[ii].tzColor);
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
      //      int show_temp = 1;
      //      if (show_temp) {
      //paint_temp(x, y, letter_spacing, space_spacing, offscreen, font, 99);
      //}

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
