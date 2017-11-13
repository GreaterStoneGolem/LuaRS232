
-- Script to listen to a serial port
--
-- Example usage:
--   lua53.exe JustListen.lua COM5
-- or:
--   lua-5.3.1\bin\lua53.exe JustListen.lua "Acme USB-to-Serial Bridge" 115200 "8N1"
-- or on Linux:
--   sudo lua JustListen.lua ttyUSB0

-- If an invalid name is passed, it will print the list of available port along with the error message
-- Control characters are replaced by their abbrieviation, between <>, before being printed


-- Load library
local LuaRS232=require("LuaRS232")


-- Function that takes:
--   either a port name, such as: "COM5"
--   or a driver name, such as "Acme USB-to-Serial Bridge"
-- and returns:
--   an opened port handle
local function OpenPortByDriverOrPortName(name,baudrate,mode)
	assert(type(name)=="string","Error, require driver or port name as argument")
	-- Retrive the detailed list of ports
	local PortList=LuaRS232.SerialPortList()
	if #PortList==0 then
		error("Error, no serial port!")
	end
	-- Check which match
	local matches={}
	for _,port in ipairs(PortList) do
		if port.name==name then
			table.insert(matches,port)
		end
		if port.fullname==name then
			table.insert(matches,port)
		end
	end
	if #matches~=1 then
		-- It no or non unique match, helpfully print the list of ports
		print("")
		print("Available ports:")
		for _,port in ipairs(PortList) do
			print("    "..port.name..(port.fullname and ("    "..port.fullname) or ""))
		end
		print("")
	end
	assert(#matches~=0,"Error, no serial port with a name or driver name of "..name)
	assert(#matches<=1,"Error, there are "..#matches.." serial ports with a name or driver name of "..name..", don't know which to pick")
	return LuaRS232.SerialPortOpen(matches[1].name,baudrate,mode)
end



-- Open the port, using command line arguments
local UART=OpenPortByDriverOrPortName(...)



local ControlChars={
	[0x00]="NUL",
	[0x01]="SOH",
	[0x02]="STX",
	[0x03]="ETX",
	[0x04]="EOT",
	[0x05]="ENQ",
	[0x06]="ACK",
	[0x07]="BEL",
	[0x08]="BS",
	[0x09]="TAB",
	[0x0A]="LF",
	[0x0B]="VT",
	[0x0C]="FF",
	[0x0D]="CR",
	[0x0E]="SO",
	[0x0F]="SI",
	[0x10]="DLE",
	[0x11]="DC1",
	[0x12]="DC2",
	[0x13]="DC3",
	[0x14]="DC4",
	[0x15]="NAK",
	[0x16]="SYN",
	[0x17]="ETB",
	[0x18]="CAN",
	[0x19]="EM",
	[0x1A]="SUB",
	[0x1B]="ESC",
	[0x1C]="FS",
	[0x1D]="GS",
	[0x1E]="RS",
	[0x1F]="US",
}



-- Listen to UART, looping until the esc key is pressed
repeat
	LuaRS232.Sleep(100)
	io.write((-- Write to stdout
		UART:Receive()-- Receive bytes from UART
		:gsub("%c",-- Catch control characters
			function(c)
				local d=ControlChars[c:byte()]
				if d and c~="\r" and c~="\n" then
					return ("<%s>"):format(d)
				else
					return c
				end
			end)
		-- Replace any sequence of return carriage and new line by \r\n
		-- This is to avoid overwriting on the same line when a \r is received without \n
		-- Or not moving to next line when \n is received without \r on Windows
		:gsub("[\r\n]+","\r\n")
		))-- Extra () to drop second gsub returned value
until LuaRS232.GetKeyPress()==27-- 27 is the escape key


