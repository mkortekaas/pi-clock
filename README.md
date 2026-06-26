# pi-clock

## Building

1. Starting instructions are here: <https://learn.adafruit.com/adafruit-rgb-matrix-plus-real-time-clock-hat-for-raspberry-pi/downloads?view=all>.
   - While [adafruit](https://learn.adafruit.com/adafruit-rgb-matrix-plus-real-time-clock-hat-for-raspberry-pi/downloads?view=all#install-using-script) has a `rgb-matrix.sh` file I did not have luck with it in the most recent version
1. Clone this repo into `~/git/pi-clock`
1. Clone a copy of <https://github.com/hzeller/rpi-rgb-led-matrix>.
   - Best to install into `~/pi/rpi-rgb-led-matrix` as the later `Makefile` assumes that location
      - I just did a straight `make` of this library
      - The most recent commit-hash of the version I am running is `a6d11e56110da3442a7781db91d0889345ee8137`
   - Build this prior to building the pi-clock as we need the library
   - If you do not use the adafruit script you will require a `--led-gpio-mapping=adafruit-hat` argument added to your script as that tells the library which hardware to use
1. Install yaml-cpp: `sudo apt-get install libyaml-cpp-dev`
1. `cd ~/git/pi-clock ; make` to build the pi-clock executable
1. Edit `config.yaml` to match your hardware and preferences
1. You can use the `clock-cli.sh` script to run it, or pass `-c <path>` to specify a config file

## Parts

Parts I purchased:

- 1x raspberry pi 4/4GB
  - <https://www.adafruit.com/product/4296>
  - though I tested a pi2 and it worked fine
  - you will also need a memory card
- 1x ada-fruit hat + rtc
  - <https://www.adafruit.com/product/2345>
  - This requires soldering - I'd have gladly paid $$ for it to be done
- 2x ada-fruit 64x32 RGB LED Matrix - 4mm pitch
  - <https://www.adafruit.com/product/2278>
- 30cm 16-pin IDC connector
  - <https://www.amazon.com/gp/product/B07FZWH9S6/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1>
- 5V Power Supply (DC5V10A)
  - <https://www.amazon.com/gp/product/B01D8FM71S/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1>
  - You can get by with lower power
- 8x M3-50 screws
  - Along with a 8x M3 bolts and 8 washers
  - (there are plenty of options on AMZ but I grabbed from hardware store)
- 10" x 10" thin board
  - 4"x4" square cutout in the center for the PI to get through
  - I used this to mount the matrixies to - just be careful to not screw in too far
  - Holes drilled slightly larger to give me play

- If you were to decide to do a 64x64 Matrix in theory it should work fine
  - You will need to solder extra jumper on the HAT board
  - You will need to (slightly) rework the code logic for displaying the times
  - You will not need the extra 30cm cable above
  - You will need 4x screws instead

## Configuration (`config.yaml`)

All settings live in `config.yaml`. Pass it to the binary with `-c`:

```bash
sudo ./pi-clock -c config.yaml
```

### `matrix` — hardware settings

| Key | Description | Default |
|---|---|---|
| `hardware_mapping` | LED driver mapping (`adafruit-hat`, `regular`, …) | `regular` |
| `rows` | Rows per panel | `32` |
| `cols` | Columns per panel | `64` |
| `chain_length` | Number of panels daisy-chained | `1` |
| `parallel` | Number of parallel chains | `1` |
| `pixel_mapper` | Optional mapper, e.g. `V-mapper` for stacked panels | (empty) |
| `slowdown_gpio` | GPIO slowdown — increase if display glitches | `1` |

### `display` — layout and timing

| Key | Description | Default |
|---|---|---|
| `font` | Path to a `.bdf` font file | (required) |
| `x_origin` | Horizontal pixel offset of text | `0` |
| `y_origin` | Vertical pixel offset of text | `0` |
| `brightness` | Panel brightness 0–100 | `100` |
| `letter_spacing` | Pixel gap between letters (negative tightens) | `0` |
| `space_spacing` | Pixel width of a space character | `2` |
| `layout` | `d`=rows stacked Downward, `l`=Left/Right side-by-side | `d` |
| `date_format` | [strftime(3)](https://man7.org/linux/man-pages/man3/strftime.3.html) format, e.g. `"%H:%M:%S %Z"` | `%H:%M:%S` |
| `show_city_name` | Show full city name instead of the `display` label | `false` |
| `dim_at_night` | Reduce brightness outside daytime hours | `false` |
| `dim_start_hour` | Hour (24h) to start dimming | `21` |
| `dim_end_hour` | Hour (24h) to stop dimming | `6` |
| `day_start_hour` | Hour (24h) for overnight→day color transition | `8` |
| `evening_start_hour` | Hour (24h) for day→evening color transition | `18` |
| `highlight_own_tz` | Show the local timezone row in `colors.own_tz` color | `false` |

### `colors` — RGB values per role

Each entry takes `{r, g, b}` values in the range 0–255.

| Key | When applied | Default |
|---|---|---|
| `overnight` | Before `day_start_hour` | blue `{0, 0, 255}` |
| `day` | Between `day_start_hour` and `evening_start_hour` | white `{255, 255, 255}` |
| `evening` | After `evening_start_hour` | red `{255, 0, 0}` |
| `dimmed` | All rows during dim hours (when `dim_at_night: true`) | red `{255, 0, 0}` |
| `own_tz` | Local timezone row (when `highlight_own_tz: true`) | green `{0, 255, 0}` |
| `temp` | Temperature line | green `{0, 255, 0}` |

### `cities` — timezones to display

A list of entries. Order determines display order on the panel.

```yaml
cities:
  - timezone: America/New_York
    display: NYC
  - timezone: Europe/Paris
    display: EUR
```

| Key | Description |
|---|---|
| `timezone` | IANA timezone name (see `/usr/share/zoneinfo/`) |
| `display` | Short label shown on the panel (airport code, city abbreviation, etc.) |

### `temperature` — optional temperature line

| Key | Description | Default |
|---|---|---|
| `enabled` | Show a temperature line on the display | `false` |
| `file` | Path to a text file whose first line is the temperature value | (empty) |
| `interval` | How often (in seconds) to re-read the file | `5` |
| `prefix` | String prepended to the value, e.g. `"TEMP "` | `TEMP ` |

The temperature file should contain a plain value such as `72.3F` or `22.4C`. A separate script (e.g. a cron job or `ExecStartPre` in the service unit) is responsible for keeping that file up to date.

## Panel layout

With a single panel, set `matrix.chain_length: 1` in `config.yaml`.

With two panels side by side, set `matrix.chain_length: 2` (and leave `pixel_mapper` empty).
With two panels stacked vertically, set `matrix.chain_length: 2` and `matrix.pixel_mapper: "V-mapper"`.

Replace `2` with however many panels you have.

## Run as a service

I created a service so this starts up automatically on startup (2 64x32 panels in one chain but mounted vertically)

1. Create a file `/lib/systemd/system/pi-clock.service` with the following in it:

  ```bash
  [Unit]
  Description=PI Clock Service
  After=multi-user.target

  [Service]
  Type=idle
  ### THIS IS IF YOU HAVE A temperature script
  ExecStartPre=/home/pi/git/mk-scripts/tempest/getTempest.sh
  ExecStart=/home/pi/git/pi-clock/pi-clock -c /home/pi/git/pi-clock/config.yaml

  [Install]
  WantedBy=multi-user.target
  ```

1. `sudo systemctl enable pi-clock`
1. `sudo systemctl start pi-clock` (or reboot...)
