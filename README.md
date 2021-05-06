# StatusStick
A cobbled together script for the M5StickC ESP32 gadget to make homeoffice life a bit nicer. It can help you to know when someone in another room is free to talk and do some basic communication on a dedicated device without having to message them. It's all very basic and hacked together but I'm happy with this first project as it has worked fine in production so far ;)

## Features:
- User can indicate their status via button press, others can check it via a website running on the stick
- Others can switch on a notification LED on the stick from the website
- Notification LED can be turned off via button press in case it annoys the user
- Others can send a short message to the stick display from the website (i.e. "Lunch?")
- When a TVOC Sensor is attatched and the eCO2 value goes over 900ppm, the stick displays a message asking to open a window and flashes the LED until the value drops under 900
- OTA update

## Instructions
- Put your wifi credentials in (remember this ESP32 only does 2,4GHz wifi)
- after switching on the stick, find out how to reach the website (probably check out what IP your AP/router assigned to it)
- button A toggles free/busy
- button B toggles notification LED


## Opportunities for improvement:
- make message display pretty (or display usage in general)
- make the website better
- multi-device support via a server or meshing?
- smooth out the eCO2 reading triggering the air quality notification
- battery life can probably be extended
- make the code acceptable to someone who is not just learning to mash together some scripts

I welcome feedback and contributions and will try to list all contributing sources for this.

## I took code and/or inspiration from:
- the Arduino OTA example
- several posts on TVOC sensor stuff
- I should really start saving links to all the sources while I use them
