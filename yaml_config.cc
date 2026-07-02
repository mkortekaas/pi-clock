#include "yaml_config.h"
#include <yaml-cpp/yaml.h>
#include <stdio.h>

static RGBColor parse_color(const YAML::Node &node, const RGBColor &def)
{
    if (!node || !node.IsMap()) return def;
    RGBColor c;
    c.r = static_cast<uint8_t>(node["r"].as<int>(def.r));
    c.g = static_cast<uint8_t>(node["g"].as<int>(def.g));
    c.b = static_cast<uint8_t>(node["b"].as<int>(def.b));
    return c;
}

bool load_yaml_config(const char *filename, PiClockConfig *config)
{
    YAML::Node doc;
    try {
        doc = YAML::LoadFile(filename);
    } catch (const YAML::Exception &e) {
        fprintf(stderr, "Failed to parse config '%s': %s\n", filename, e.what());
        return false;
    }

    if (doc["matrix"]) {
        auto m = doc["matrix"];
        config->hardware_mapping = m["hardware_mapping"].as<std::string>(config->hardware_mapping);
        config->pixel_mapper     = m["pixel_mapper"].as<std::string>(config->pixel_mapper);
        config->rows             = m["rows"].as<int>(config->rows);
        config->cols             = m["cols"].as<int>(config->cols);
        config->chain_length     = m["chain_length"].as<int>(config->chain_length);
        config->parallel         = m["parallel"].as<int>(config->parallel);
        config->gpio_slowdown    = m["slowdown_gpio"].as<int>(config->gpio_slowdown);
    }

    if (doc["display"]) {
        auto d = doc["display"];
        config->font_file          = d["font"].as<std::string>(config->font_file);
        config->x_origin           = d["x_origin"].as<int>(config->x_origin);
        config->y_origin           = d["y_origin"].as<int>(config->y_origin);
        config->brightness         = static_cast<uint8_t>(d["brightness"].as<int>(config->brightness));
        config->letter_spacing     = d["letter_spacing"].as<int>(config->letter_spacing);
        config->space_spacing      = d["space_spacing"].as<int>(config->space_spacing);
        std::string layout_str     = d["layout"].as<std::string>(std::string(1, config->layout));
        config->layout             = layout_str.empty() ? 'd' : layout_str[0];
        config->date_format        = d["date_format"].as<std::string>(config->date_format);
        config->show_city_name     = d["show_city_name"].as<bool>(config->show_city_name);
        config->dim_at_night       = d["dim_at_night"].as<bool>(config->dim_at_night);
        config->dim_start_hour     = d["dim_start_hour"].as<int>(config->dim_start_hour);
        config->dim_end_hour       = d["dim_end_hour"].as<int>(config->dim_end_hour);
        config->day_start_hour     = d["day_start_hour"].as<int>(config->day_start_hour);
        config->evening_start_hour = d["evening_start_hour"].as<int>(config->evening_start_hour);
        config->highlight_own_tz   = d["highlight_own_tz"].as<bool>(config->highlight_own_tz);
        if (d["row_offsets"] && d["row_offsets"].IsSequence()) {
            config->row_offsets.clear();
            for (const auto &entry : d["row_offsets"])
                config->row_offsets.push_back(entry.as<int>(0));
        }
    }

    if (doc["colors"]) {
        auto c = doc["colors"];
        config->color_overnight = parse_color(c["overnight"], config->color_overnight);
        config->color_day       = parse_color(c["day"],       config->color_day);
        config->color_evening   = parse_color(c["evening"],   config->color_evening);
        config->color_dimmed    = parse_color(c["dimmed"],    config->color_dimmed);
        config->color_own_tz    = parse_color(c["own_tz"],    config->color_own_tz);
        config->color_temp      = parse_color(c["temp"],      config->color_temp);
    }

    if (doc["cities"] && doc["cities"].IsSequence()) {
        for (const auto &entry : doc["cities"]) {
            CityEntry city;
            city.timezone = entry["timezone"].as<std::string>("");
            city.display  = entry["display"].as<std::string>("");
            if (!city.timezone.empty() && !city.display.empty())
                config->cities.push_back(city);
        }
    }

    if (doc["temperature"]) {
        auto t = doc["temperature"];
        config->show_temp     = t["enabled"].as<bool>(config->show_temp);
        config->temp_file     = t["file"].as<std::string>(config->temp_file);
        config->temp_interval = t["interval"].as<int>(config->temp_interval);
        config->temp_prefix   = t["prefix"].as<std::string>(config->temp_prefix);
    }

    return true;
}
