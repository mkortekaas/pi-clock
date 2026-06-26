#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct RGBColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct CityEntry {
    std::string timezone;
    std::string display;
};

struct PiClockConfig {
    // Matrix hardware
    std::string hardware_mapping = "regular";
    std::string pixel_mapper     = "";
    int rows          = 32;
    int cols          = 64;
    int chain_length  = 1;
    int parallel      = 1;
    int gpio_slowdown = 1;

    // Display
    std::string font_file;
    int         x_origin         = 0;
    int         y_origin         = 0;
    uint8_t     brightness       = 100;
    int         letter_spacing   = 0;
    int         space_spacing    = 2;
    char        layout           = 'd';
    std::string date_format      = "%H:%M:%S";
    bool        show_city_name   = false;
    bool        dim_at_night     = false;
    int         dim_start_hour   = 21;
    int         dim_end_hour     = 6;
    int         day_start_hour   = 8;
    int         evening_start_hour = 18;
    bool        highlight_own_tz = false;

    // Colors
    RGBColor    color_overnight  = {0,   0,   255};
    RGBColor    color_day        = {255, 255, 255};
    RGBColor    color_evening    = {255, 0,   0};
    RGBColor    color_dimmed     = {255, 0,   0};
    RGBColor    color_own_tz     = {0,   255, 0};
    RGBColor    color_temp       = {0,   255, 0};

    // Cities
    std::vector<CityEntry> cities;

    // Temperature
    bool        show_temp     = false;
    std::string temp_file;
    int         temp_interval = 5;
    std::string temp_prefix   = "TEMP ";
};

bool load_yaml_config(const char *filename, PiClockConfig *config);
