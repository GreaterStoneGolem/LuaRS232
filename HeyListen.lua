
-- Program to listen to UART
-- Example usage:
-- lua-5.3.1\bin\lua53.exe HeyListen.lua COM12
-- lua-5.3.1\bin\lua53.exe HeyListen.lua COM12 115200 8N1

local LuaRS232=require("LuaRS232")
local UART=LuaRS232.SerialPortOpen(...)
print("Listening to serial port "..UART.name..", press any key to exit")
while true do
	local k=LuaRS232.GetKeyPress()
	if k~=0 then
		return
	end
	LuaRS232.Sleep(100)
	local r=UART:Receive()
	for c in (function() local n=0 return (function() n=n+1 if n>#r then return nil else return r:sub(n,n) end end) end)() do
		print(LuaRS232.Bin2Hex(c).." "..c:gsub("[^%g]",function(c)
			return ({["\r"]="\\r",["\n"]="\\n",["\t"]="tab",["\x20"]="space"})[c] or ("\\x%02X"):format(c:byte())
		end))
	end
end
