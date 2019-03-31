# SocialDisplayStats
Gets social media follow counts and displays them on a MAX7219 display

The only items required for the build are an ESP8266, a four panel MAX7219 display, and a 5v micro usb power supply.

The FontData.h, SocialDisplay.h, SocialDisplay.cpp, and SocialDisplay.ino are used on an ESP8266.
The ESP8266 connects a mqtt broker to get a data structure that contains the social media icon image and follow count.

The SocialStats.py is a process to be run on a server that will get the social media information every minute for each user.
New users can be added to the database file and dont need to have each of the social media platforms defined.