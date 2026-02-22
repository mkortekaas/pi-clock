#!/bin/bash

RGB_LIB_DISTRIBUTION=/home/pi/git/rpi-rgb-led-matrix

## font is required
##  -S spacing option of -1 allows everything to fit with this font
## 
## while 2 panels the system sees it as one with these matrics
##    as such we use 128 instead of using the --led-chain option
##
## - x / y are based on playing around
##
## with a pi4 you need the --led-slowdown-gpio=4 option, the pi2 did not
##
## -b(rightness) option based on personal preference/room layout

#./pi-clock --led-rows=32 --led-cols=128 -f ${RGB_LIB_DISTRIBUTION}/fonts/6x10.bdf -S -1 -x 2 -y 2 --led-slowdown-gpio=4 -b 50 -d l
#./pi-clock --led-rows=32 --led-cols=64 -f ${RGB_LIB_DISTRIBUTION}/fonts/6x10.bdf -S -1 -x 2 -y 4 --led-slowdown-gpio=4 -b 50 --led-pixel-mapper=V-mapper --led-chain=2 
#./pi-clock --led-rows=32 --led-cols=64 -f ${RGB_LIB_DISTRIBUTION}/fonts/6x10.bdf -S -1 -x 2 -y 1 --led-slowdown-gpio=4 -b 50 --led-pixel-mapper=V-mapper --led-chain=2  -C cities.txt
./pi-clock --led-rows=32 --led-cols=64 -f ${RGB_LIB_DISTRIBUTION}/fonts/6x10.bdf -x 2 -y 4 -S -1 --led-slowdown-gpio=4 -b 50 --led-pixel-mapper=V-mapper --led-chain=2 -C /home/pi/git/pi-clock/cities.txt -T -t /tmp/tempest_airtemp.txt --led-gpio-mapping=adafruit-hat
#./pi-clock --led-rows=32 --led-cols=64 -f ${RGB_LIB_DISTRIBUTION}/fonts/6x10.bdf -x 2 -y 4 -S -1 --led-slowdown-gpio=4 -b 50 --led-pixel-mapper=V-mapper --led-chain=2 -C /home/pi/git/pi-clock/cities.txt -T -t /tmp/tempest_airtemp.txt


