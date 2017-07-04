# RFID3

Code electric lock with RFID. You can open your electric lock
with any RFID card (compatible to reader and remembered). Or you
can enter code on keyboard. Simple!

Also you can open and/or watch for door opening via MQTT (using wifi).

This project has special PROG key. If you press it shortly, then you can
add new card (press '\*', lean the card, press digit or alpabeet key to link it).
You can delete any card - press PROG shortly, enter '#', then linked key.
To change the code, press PROG shortly, then 'D' and 4 digits of code
(to change code length, change RFID2.ino, it is simple).
To forget all linked cards, press PROG shortly anf then '0'.

Feel free to write me issues and pullrequests!

## Sources

- DFRobot DFRuino pro mini 3.3V (or other pro mini)
- Wemos D1 (you can replace it with any arduino + wifi module, check the code then)
- Resistors (see scheme)
- RFID reader MFRC522
- keypad 4x4
- relay module
- 2x RS485 modules
- electick lock ;)
- 12V power source, 2Amp (!important! 1A is too low)
- single key (or switch)
- buzzer (optional)
- 3-in-one LED (or 3 leds) (optional)

## Howto

Um... In short: just connect all parts to arduino properly.
Be carefull, RFID reader and esp8266 use 3.3V logic, while arduino uses 5V, so resistors are importatnt.

You can see pin headers on scheme. It is just to shorten the shceme. Actually you should connect
arduino D2..D5 and A1..A4 to 8 pins on keypad. See the ino-file for details. You can fix connection in source code.

Here are two parts:
- Pult (DFRobot pro mini + RFID reader + keyboard + led/buzzer + rs485)
- Control module (WeMos + relay + switdh + rs485)

Control module should be inside room and connected with pult via rs485 wire.



