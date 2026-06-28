# Current Configuration

## /lib/systemd/system/pi-clock.service

Current content is:

```bash
pi@pi-clock:~ $ more /lib/systemd/system/pi-clock.service 
[Unit]
Description=PI Clock Service
After=multi-user.target

[Service]   
Type=idle
ExecStartPre=/home/pi/git/mk-scripts/tempest/getTempest.py
ExecStart=/home/pi/git/pi-clock/pi-clock --led-rows=32 --led-cols=64 -f /home/pi/git/rpi-rgb-led-matrix/fonts/6x10.bdf -x 2 -y 4 -S -1 --led-slowdown-gpio=4 -b 50 --led-pixel-mapper=V-mapper --led-chain=2 -C /home/pi/git/pi-clock/cities.txt -T -t /tmp/tempest_airtemp.txt --led-gpio-mapping=adafruit-hat -D

[Install]
WantedBy=multi-user.target
```

## /home/pi/git/pi-clock/cities.txt

Current content is:

```text
#
# Whatever you want to show!
#

#	Africa/Johannesburg	JNB
	America/Denver		DEN
#	America/Los_Angeles	SEA
	America/New_York	NYC
#	Pacific/Auckland	AKL
	Asia/Kolkata		BLR
#	Asia/Hong_Kong		HKG
#	Asia/Singapore		SIN	# RPi fun - Singapore does not have a timezone name (but it should be SGT)
#	Asia/Tel_Aviv		TLV
#	Europe/London		LHR
	Europe/Paris		EUR
	UTC			UTC
```
