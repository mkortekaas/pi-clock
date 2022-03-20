# pi-clock

# Start

Starting instructions are here: https://learn.adafruit.com/adafruit-rgb-matrix-plus-real-time-clock-hat-for-raspberry-pi/downloads?view=all. In particular there is a main script in there that configures the required library ( https://github.com/hzeller/rpi-rgb-led-matrix ) and installs what is required. In the case of this setup that is presumed to be in `/home/pi/matrix' (and then the rest of our code is in `/home/pi/git/pi-clock`). 

# Parts
Parts I purchased:
- 1x raspberry pi 4/4GB
  - https://www.adafruit.com/product/4296
  - though I tested a pi2 and it worked fine
  - you will also need a memory card
- 1x ada-fruit hat + rtc
  - https://www.adafruit.com/product/2345
  - This requires soldering - I'd have gladly paid $$ for it to be done
- 2x ada-fruit 64x32 RGB LED Matrix - 4mm pitch
  - https://www.adafruit.com/product/2278
- 30cm 16-pin IDC connector
  - https://www.amazon.com/gp/product/B07FZWH9S6/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1
- 5V Power Supply (DC5V10A)
  - https://www.amazon.com/gp/product/B01D8FM71S/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
  - You can get by with lower power
- 8x M3-50 screws
  - Along with a 8x M3 bolts and 8 washers
  - (there are plenty of options on AMZ but I grabbed from hardware store)
- 10" x 10" thin board
  -  4"x4" square cutout in the center for the PI to get through
  -  I used this to mount the matrixies to - just be careful to not screw in too far
  -  Holes drilled slightly larger to give me play

- If you were to decide to do a 64x64 Matrix in theory it should work fine
  - You will need to solder extra jumper on the HAT board
  - You will need to (slightly) rework the code logic for displaying the times
  - You will not need the extra 30cm cable above
  - You will need 4x screws instead

## Added by [@mahtin](https://github.com/mahtin)

```
  -d                : Layout is down vs left/right
  -c                : Show city name vs airport code
  -G <spacing>      : Gap between columns in pixels (Default: 4)
  -F <date-format>  : Date format (Default is HH:MM:SS via %H:%M:%S)
```

If you are using a wide display (or two panels left-right) then it paints left right.
If you have a square or tall setup, then use the `-d` flag to paint downwards.

The `-c` flag will show city names vs three letter airport codes - it's not perfect.

The `-F` flag allows you to play with the date format output. For example `-F '%H:%M:%S %Z'` will add a timezone string.
You could use `%a` or `%A` for the day of the week, etc.
See [strfime(3)](https://man7.org/linux/man-pages/man3/strftime.3.html) man page.

An example command line for a 64x64 display could be:
```
$ sudo ./pi-clock --led-rows=64 --led-cols=64 --led-slowdown-gpio=4 --led-gpio-mapping=adafruit-hat -f $HZELLER/fonts/4x6.bdf -F '%H:%M:%S %Z' -x 1 -y 14 -G 2 -d
$
```

Where `$HZELLER` is wherever you have installed and compiled the initial code above

## Run as a service

I created a service so this starts up automatically on startup

1. Create a file /lib/systemd/system/pi-clock.service with the following in it:
```
   [Unit]
   Description=PI Clock Service
   After=multi-user.target

   [Service]
   Type=idle
   ExecStart=/home/pi/git/pi-clock/pi-clock --led-rows=32 --led-cols=128 -f /home/pi/matrix/rpi-rgb-led-matrix/fonts/6x10.bdf -x 2 -y 2 -S -1 --led-slowdown-gpio=4 -b 50

   [Install]
   WantedBy=multi-user.target
```
2. `systemctl enable pi-clock`
3. `systemctl start pi-clock` (or reboot...)


