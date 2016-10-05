
-- Program to Test LuaRS232
-- Example usage:
-- lua-5.3.1\bin\lua53.exe TestRS232.lua COM12

local PortName=...
local LuaRS232=require("LuaRS232")
local UART=LuaRS232.SerialPortOpen(PortName)

print("Waiting 5 seconds (send it messages meanwhile)")
LuaRS232.Sleep(5000)
print("Done waiting")

local got=UART:Receive()
print("Received: \""..got.."\"")

print("Send a message")
UART:Send("What's up doc\r\n")

print("Sleep some more")
UART:Sleep(5000)

local got=UART:Receive()
print("Received: \""..got.."\"")
