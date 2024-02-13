Using avrdude with the Raspberry Pi
===================================

Since the Raspberry Pi lacks a DTR pin that makes it oh-so-easy to upload your hex files into
the AVR, we need this hack to make it just as easy.  When you wire up your ATmega chip, be sure
to connect one of the digital GPIO pins to the reset pin, and then you'll be able to use avrdude
as if your serial cable actually had a DTR pin.

Instructions:
-------------

Install using script:

    $ git clone https://github.com/openenergymonitor/avrdude-rpi.git ~/avrdude-rpi && ~/avrdude-rpi/install


Manual Install: 

Copy both files into your /usr/bin directory, then rename the original avrdude to avrdude-original
and symlink avrdude-autoreset to become avrdude.

    $ cp autoreset /usr/bin
    $ cp avrdude-autoreset /usr/bin
    $ mv /usr/bin/avrdude /usr/bin/avrdude-original
    $ ln -s /usr/bin/avrdude-autoreset /usr/bin/avrdude

Modify the autoreset script to use the pin that you wired up to the reset pin.  See the line in
autoreset where we do "PIN = 4" and change the 4 to your GPIO pin number.

Now when you run avrdude from anywhere (including via arduino's normal UI) it will flag DTR when
it is about to upload hex data.

To upload to the RFM12/69Pi (ATmega328 @ 8Mhz):

    $ avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 38400 -U flash:w:sketch_name.hex

To upload to the emonPi (ATmega328 @ 16Mhz): 

    $ avrdude -v -c arduino -p ATMEGA328P -P /dev/ttyAMA0 -b 115200 -U flash:w:sketch_name.hex

Make sure Python is installed:

    $ sudo apt-get update
    $ sudo apt-get install python3-dev
    $ sudo apt-get install python3-rpi.gpio
