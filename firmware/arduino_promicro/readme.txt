Program Arduino Pro Micro (ATmega32U4):
---------------------------------------
avrdude -c avr109 -p atmega32u4 -P COMx -b 57600 -D -U flash:w:arduino_promicro.hex:i

Build:
------
cd firmware/arduino_promicro
make all
