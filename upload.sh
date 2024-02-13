#! /bin/bash

echo copy hexfile
#scp Release/Stopwatch_PI.hex filou@timestamp:/home/filou/upload.hex
scp Release/Stopwatch_PI.hex pi@elzwelle:/home/pi/upload.hex
echo upload done

echo flash hexfile
#ssh filou@timestamp "avrdude -P/dev/ttyAMA0 -pm168 -b19200 -carduino -D -Uflash:w:upload.hex:i 2>&1"
ssh pi@elzwelle "avrdude -P/dev/ttyAMA0 -pm168 -b19200 -carduino -D -Uflash:w:upload.hex:i 2>&1"
echo flash done

