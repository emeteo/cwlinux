#!/bin/bash



SPEED=19200
DEVICE=/dev/ttyUSB0


# List of knowns Text commands:
AUTO_LINE_WRAP_ON="\xFE\x43\xFD"
AUTO_LINE_WRAP_OFF="\xFE\x44\xFD"
AUTO_SCROLL_ON="\xFE\X51\xFD"
AUTO_SCROLL_OFF="\xFE\X52\xFD"
SET_TEXT_HOME="\xFE\x48\xFD"

#Misc commands:
CLEAR_DISPLAY="\xFE\x58\xFD"
BACKLIGHT_ON="\xFE\x42\xFD"
BACKLIGHT_Off="\xFE\x46\xFD"


function setup ()
{
	stty ispeed $SPEED ospeed $SPEED -F $DEVICE > /dev/null
}

function print()
{
	echo -en "$1" > $DEVICE
}





function main()
{
	setup
	print "$AUTO_LINE_WRAP_ON"
	print "$AUTO_SCROLL_ON"
#	print "********* GORRION ***********"
	print "$CLEAR_DISPLAY"
	while true; do
		print "$SET_TEXT_HOME"
		print "`uptime`"
		sleep 1;
	done;
}




main

