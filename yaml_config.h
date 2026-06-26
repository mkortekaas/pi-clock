#pragma once
#include <string>
#include <vector>
#include <cstdint>

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
    bool        highlight_own_tz = false;

    // Cities
    std::vector<CityEntry> cities;

    // Temperature
    bool        show_temp     = false;
    std::string temp_file;
    int         temp_interval = 5;
};

bool load_yaml_config(const char *filename, PiClockConfig *config);
