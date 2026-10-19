// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
//
// Modified from
//   https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/examples-api-use/clock.cc
//
// This code is public domain - the source file is GPL v2
//  Note also the led-matrix library this depends on is GPL v2

#include "led-matrix.h"
#include "graphics.h"

#include "clock_utils.h"
#include "configuration.h"
#include "timezones.h"
#include "color_temp.h"
#include "FileReader.h"
#include "yaml_config.h"

#include <algorithm>
#include <getopt.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace rgb_matrix;

// strftime output buffer; formats are short, e.g. "%H:%M:%S"
static const size_t TIME_TEXT_MAX = 80;

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

static Color to_color(const RGBColor &c) {
  return Color(c.r, c.g, c.b);
}

struct ColorScheme {
  Color overnight;
  Color day;
  Color evening;
  Color dimmed;
  Color own_tz;
  Color temp;
};

static ColorScheme colors_from_config(const PiClockConfig &config) {
  return ColorScheme{
    to_color(config.color_overnight), to_color(config.color_day),
    to_color(config.color_evening),   to_color(config.color_dimmed),
    to_color(config.color_own_tz),    to_color(config.color_temp),
  };
}

static Color zone_color(const struct tm &tm, bool is_my_timezone, bool is_dimmed,
                        const PiClockConfig &config, const ColorScheme &colors) {
  if (is_dimmed)
    return colors.dimmed;
  if (config.highlight_own_tz && is_my_timezone)
    return colors.own_tz;
  if (tm.tm_hour >= config.evening_start_hour)
    return colors.evening;
  if (tm.tm_hour >= config.day_start_hour)
    return colors.day;
  return colors.overnight;
}

// Local time, text and color of every configured city at `when`, in display order
static std::vector<TZData> snapshot_zones(time_t when, bool is_dimmed, const std::string &my_timezone,
                                          const PiClockConfig &config, const ColorScheme &colors) {
  std::vector<TZData> zones;
  zones.reserve(config.cities.size());
  for (const CityEntry &city : config.cities) {
    TZData zone;
    zone.tzString  = city.timezone;
    zone.tzDisplay = city.display;
    set_timezone(city.timezone);
    localtime_r(&when, &zone.tm);
    char text[TIME_TEXT_MAX + 1];
    strftime(text, sizeof(text), config.date_format.c_str(), &zone.tm);
    zone.timeText = text;
    zone.tzColor  = zone_color(zone.tm, city.timezone == my_timezone, is_dimmed, config, colors);
    zones.push_back(zone);
  }

  std::sort(zones.begin(), zones.end(), [](const TZData &lhs, const TZData &rhs) {
    return shows_before(lhs.tm, lhs.tzString, rhs.tm, rhs.tzString);
  });
  return zones;
}

static bool is_night(time_t when, const std::string &my_timezone, const PiClockConfig &config) {
  struct tm my_tm;
  set_timezone(my_timezone);
  localtime_r(&when, &my_tm);
  return my_tm.tm_hour < config.dim_end_hour || my_tm.tm_hour >= config.dim_start_hour;
}

// Per-row vertical nudge from display.row_offsets (top row = 0); unlisted rows get 0
static int row_y_offset(const std::vector<int> &offsets, size_t row) {
  return row < offsets.size() ? offsets[row] : 0;
}

void paint_airport_code(int x, int y, int letter_spacing, FrameCanvas *canvas, const Font &font, const TZData &zone) {
  DrawText(canvas, font, x, y + font.baseline(), zone.tzColor, NULL, zone.tzDisplay.c_str(), letter_spacing);
}

void paint_city_name(int x, int y, int letter_spacing, FrameCanvas *canvas, const Font &font, const TZData &zone) {
  const std::string city = city_window(zone.tzString, zone.tm.tm_min * 60 + zone.tm.tm_sec);
  DrawText(canvas, font, x, y + font.baseline(), zone.tzColor, NULL, city.c_str(), letter_spacing);
}

// Draw space-separated words; the gap between words is space_spacing pixels
// rather than the font's own space glyph
void paint_words(int x, int y, int letter_spacing, int space_spacing, FrameCanvas *canvas, const Font &font,
                 const std::string &text, const Color &color) {
  std::istringstream iss(text);
  std::string token;
  while (iss >> token) {
    DrawText(canvas, font, x, y + font.baseline(), color, NULL, token.c_str(), letter_spacing);
    x += font.CharacterWidth('@') * token.length() + space_spacing;
  }
}

// Returns an error message, or empty when the config is usable
static std::string validate_config(const PiClockConfig &config) {
  if (config.layout != 'd' && config.layout != 'l')
    return std::string("Bad layout '") + config.layout + "' in config (only d=Downward, l=Left/Right)";
  if (config.font_file.empty())
    return "display.font must be set in config";
  if (config.brightness < 1 || config.brightness > 100)
    return "display.brightness must be 1-100";
  if (config.cities.empty())
    return "No cities defined in config";
  return "";
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

  const std::string config_error = validate_config(config);
  if (!config_error.empty()) {
    fprintf(stderr, "%s\n", config_error.c_str());
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
  int   x_orig               = config.x_origin;
  int   y_orig               = config.y_origin;
  bool  dim_display          = config.dim_at_night;
  int   letter_spacing       = config.letter_spacing;
  int   space_spacing        = config.space_spacing;
  bool  city_name     = config.show_city_name;
  char  layout_choice = config.layout;
  bool  show_temp            = config.show_temp;

  const ColorScheme colors = colors_from_config(config);

  // background color will be black
  Color bg_color(0, 0, 0);

  FileReader temperatureFileReader(config.temp_file, config.temp_interval, config.temp_prefix);

  rgb_matrix::Font font;
  if (!font.LoadFont(config.font_file.c_str())) {
    fprintf(stderr, "Couldn't load font '%s'\n", config.font_file.c_str());
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

  matrix->SetBrightness(config.brightness);

  if (!dim_display && config.brightness == 100 && FullSaturation(colors.dimmed) && FullSaturation(bg_color))
    matrix->SetPWMBits(1);

  FrameCanvas *offscreen = matrix->CreateFrameCanvas();

  // The time - we cheat and only get it once and expect clock_nanosleep() to keep us kosher
  struct timespec next_time;
  next_time.tv_sec = time(NULL);
  next_time.tv_nsec = 0;

  const std::string my_timezone = get_timezone();
  if (my_timezone.empty())
    fprintf(stderr, "Could not determine local timezone ($TZ or /etc/timezone); dimming uses UTC\n");

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {

    const bool is_dimmed = dim_display && is_night(next_time.tv_sec, my_timezone, config);

    offscreen->Clear();
    offscreen->Fill(bg_color.r, bg_color.g, bg_color.b);

    // Sorted fresh each frame; the order only changes every hour (or 1/2 for India)
    const std::vector<TZData> zones = snapshot_zones(next_time.tv_sec, is_dimmed, my_timezone, config, colors);

    // == keeping in mind row is spread across 2 panels
    // == if you change font/sizing you will need to play with spacing

    int x = x_orig;
    int y = y_orig;
    int left_right = 0;
    size_t row = 0;

    // paint the screen but don't display it till after the loop
    // we are actually painting the upcomming time vs the time now

    for (const TZData &zone : zones) {
      int y_row = y + row_y_offset(config.row_offsets, row);
      if (city_name)
        paint_city_name(x, y_row, letter_spacing, offscreen, font, zone);
      else
        paint_airport_code(x, y_row, letter_spacing, offscreen, font, zone);

      x += font.CharacterWidth('@') * zone.tzDisplay.length() + space_spacing;
      paint_words(x, y_row, letter_spacing, space_spacing, offscreen, font, zone.timeText, zone.tzColor);

      // deal with layout
      switch (layout_choice) {
        case 'd':
          // simple - just head downwards! - don't change x
          x = x_orig;
          y += font.height();
          row += 1;
          break;

        case 'l':
          // only increment y every other entry; but x swaps around a lot
          if (left_right) {
            // move down
            x = x_orig;
            y += font.height();
            row += 1;
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
      Color tempColor = is_dimmed ? colors.dimmed : colors.temp;
      paint_words(x, y + row_y_offset(config.row_offsets, row), letter_spacing, space_spacing, offscreen, font,
                  temperatureFileReader.getValue(), tempColor);
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
