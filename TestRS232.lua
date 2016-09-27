
local UART=require("LuaRS232")
UART.SayHello()
UART.Open("COM5")
UART.Send("What's up doc")
