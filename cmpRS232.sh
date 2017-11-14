#!/bin/bash
rm *.o 2> /dev/null
rm *.so 2> /dev/null

# Check user is in group dialout and add it if not
ingroup(){ [[ " "`groups $1`" " == *" $2 "* ]]; }
if ! ingroup $USER dialout; then
  sudo adduser $USER dialout
fi

# Compile all the source into a linux shared library
g++ *.c -shared -fpic -o LuaRS232.so || exit 1

# List interfaces and serial ports
lua PrintInterfaceInfo.lua

# Try it by running an interactive serial console on first USB-to-Serial
lua SerialConsole.lua ttyUSB0
