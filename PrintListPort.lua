

-- Program to list all Serial Port dirver of the PC
-- Example usage:
-- lua-5.3.1\bin\lua53.exe PrintListPort.lua

local LuaRS232=require("LuaRS232")
local list=LuaRS232.SerialPortList()
for _,port in ipairs(list) do
	print(port.name.."  "..(port.fullname or ""))
end
