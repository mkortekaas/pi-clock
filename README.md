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

## Added by [@mahtin](https://github.com/mahtin)

All settings are now in `config.yaml`. The binary takes a single argument:

  ```bash
  sudo ./pi-clock -c config.yaml
  ```

Key display settings in `config.yaml`:

| Key | Description | Default |
|---|---|---|
| `display.layout` | `d`=Downward, `l`=Left/Right | `d` |
| `display.show_city_name` | City name instead of airport code | `false` |
| `display.space_spacing` | Pixel gap for spaces in time | `2` |
| `display.date_format` | strftime format string | `%H:%M:%S` |
| `display.dim_at_night` | Reduce brightness outside 06:00–21:00 | `false` |
| `display.highlight_own_tz` | Highlight local timezone in green | `false` |

For `date_format`, see [strftime(3)](https://man7.org/linux/man-pages/man3/strftime.3.html). For example `"%H:%M:%S %Z"` adds a timezone abbreviation.

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
