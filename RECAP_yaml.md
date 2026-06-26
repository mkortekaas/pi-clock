# Branch: 202606_yaml — Work Recap

## Goal

Move all runtime configuration out of command-line arguments into a single YAML file (`config.yaml`), so the systemd service line becomes simply:

```
ExecStart=/home/pi/git/pi-clock/pi-clock -c /home/pi/git/pi-clock/config.yaml
```

## Status: Complete (needs Pi build/test)

All code changes are done. The branch has not yet been built or tested on the Pi.

---

## What Was Changed

### New files
| File | Purpose |
|---|---|
| `yaml_config.h` | `PiClockConfig` struct (all settings + defaults) and `RGBColor` struct |
| `yaml_config.cc` | YAML parsing via yaml-cpp; `load_yaml_config()` |
| `config.yaml` | Full configuration matching current running setup |

### Modified files
| File | What changed |
|---|---|
| `pi-clock.cc` | Removed `ParseOptionsFromFlags` + all `getopt` options; now only `-c <file>`; builds `RGBMatrix::Options` directly from config; colors and thresholds from config |
| `Makefile` | Added `yaml_config.o` to OBJECTS; added `-lyaml-cpp` to LDFLAGS |
| `clock-cli.sh` | Simplified to `./pi-clock -c config.yaml` |
| `README.md` | Updated build steps, service unit, panel layout docs |

### Deleted / removed
- `configuration.cc` `read_city_list()` is no longer called (function still exists but is dead code — can be cleaned up later)
- `tzColorSet()` function removed from `pi-clock.cc` (logic inlined with configurable hours)
- `cities.txt` is no longer used at runtime (still in repo as reference)

---

## config.yaml Structure

```
matrix:        hardware_mapping, rows, cols, chain_length, parallel, pixel_mapper, slowdown_gpio
display:       font, x_origin, y_origin, brightness, letter_spacing, space_spacing,
               layout (d/l), date_format, show_city_name, dim_at_night,
               dim_start_hour, dim_end_hour, day_start_hour, evening_start_hour,
               highlight_own_tz
colors:        overnight, day, evening, dimmed, own_tz, temp  (each: {r, g, b})
cities:        list of {timezone, display} entries
temperature:   enabled, file, interval, prefix
```

All fields have defaults in `PiClockConfig` that match what was previously hardcoded, so the clock behaves identically without editing `config.yaml`.

---

## Before Building on the Pi

```bash
sudo apt-get install libyaml-cpp-dev
cd ~/git/pi-clock
git pull
make clean && make
```

Then test with:
```bash
sudo ./pi-clock -c config.yaml
```

And if happy, restart the service:
```bash
sudo systemctl restart pi-clock
```

---

## Known Loose Ends

- `read_city_list()` in `configuration.cc` and the `cities.txt` file are both dead code — safe to delete if desired
- `adjust_color()` in `pi-clock.cc` references `k_to_rgb()` from `color_temp.cc` but is never called — also dead code
- The `README_v1_config.md` documents the old CLI config and is now out of date; could be archived or deleted
