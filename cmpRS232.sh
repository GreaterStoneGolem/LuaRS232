#!/bin/bash
rm *.o 2> /dev/null
rm *.so 2> /dev/null

# Compile all the source into a linux shared library
g++ *.c -shared -fpic -o LuaRS232.so || exit 1

# Try it by running an interactive serial console on first USB-to-Serial
sudo lua SerialConsole.lua ttyUSB0
