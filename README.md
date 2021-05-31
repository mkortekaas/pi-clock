# pi-clock

Starting instructions are here: https://learn.adafruit.com/adafruit-rgb-matrix-plus-real-time-clock-hat-for-raspberry-pi/downloads?view=all

Main script that gets the library ( https://github.com/hzeller/rpi-rgb-led-matrix ) and installs what is required.

Parts I purchased:
1x raspberry pi 4/4GB
   https://www.adafruit.com/product/4296
   though I tested a pi2 and it worked fine
   you will also need a memory card
1x ada-fruit hat + rtc
   https://www.adafruit.com/product/2345
   This requires soldering - I'd have gladly paid $$ for it to be done
2x ada-fruit 64x32 RGB LED Matrix - 4mm pitch
   https://www.adafruit.com/product/2278
30cm 16-pin IDC connector
   https://www.amazon.com/gp/product/B07FZWH9S6/ref=ppx_yo_dt_b_asin_title_o01_s00?ie=UTF8&psc=1
5V Power Supply (DC5V10A)
   https://www.amazon.com/gp/product/B01D8FM71S/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
   You can get by with lower power
8x M3-50 screws
   Along with a 8x M3 bolts and 8 washers
   (there are plenty of options on AMZ but I grabbed from hardware store)
10" x 10" thin board
    4"x4" square cutout in the center for the PI to get through
    I used this to mount the matrixies to - just be careful to not screw in too far
    Holes drilled slightly larger to give me play

If you were to decide to do a 64x64 Matrix in theory it should work fine
   You will need to solder extra jumper on the HAT board
   You will need to (slightly) rework the code logic for displaying the times
   You will not need the extra 30cm cable above
   You will need 4x screws instead


=============
I created a service so this starts up automatically on startup

1. Create a file /lib/systemd/system/pi-clock.service with the following in it: 
   [Unit]
   Description=PI Clock Service
   After=multi-user.target

   [Service]
   Type=idle
   ExecStart=/home/pi/git/pi-clock/pi-clock --led-rows=32 --led-cols=128 -f /home/pi/matrix/rpi-rgb-led-matrix/fonts/6x10.bdf -x 2 -y 2 -S -1 --led-slowdown-gpio=4 -b 50

   [Install]
   WantedBy=multi-user.target

2. systemctl enable pi-clock
3. systemctl start pi-clock (or reboot...)


