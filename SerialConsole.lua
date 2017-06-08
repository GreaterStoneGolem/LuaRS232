
-- Program that implement a serial console in Lua

-- Basically I wanted something like HyperTerminal, but scriptable instead of mouse/keyboard GUI.

-- Example usage:
--   lua53 SerialConsole.lua
--   lua53 SerialConsole.lua COM5 115200 8N1
--   lua53 SerialConsole.lua "Serial Port, USB Multi-function Cable" 115200 8N1

-- First argument can be either a comm port number, or a driver name.
-- Arguments are optional: you can have zero, one, two, all three arguments.

-- When printing received characters, it escape special character, and attempts to clean the extra prompts and newlines.
-- Press escape key at any time to exit (though I reckon it's hard not to reflexively exit by CTRL+C)
-- It relies on the other end for echo. If not, please uncomment antepenultimate line.


-- Use the serial port with that driver name if none passed as argument
local DriverName="Serial Port, USB Multi-function Cable"

-- Load library
local LuaRS232=require("LuaRS232")

-- Function to convert a driver name, for exemple "USB to Serial Bridge", to a Comm Port, for exemple "COM5"
local function FindPortByDriverName(name)
	local PortList=LuaRS232.SerialPortList()
	if #PortList==0 then
		error("Error, no serial port!")
	end
	local matches={}
	for _,port in ipairs(PortList) do
		if port.fullname==name then
			table.insert(matches,port)
		end
		if port.name==name then
			table.insert(matches,port)
		end
	end
	assert(#matches~=0,"Error, no serial port with a name of "..name)
	assert(#matches<=1,"Error, there are "..#matches.." serial port with a name of "..name..", don't know which to pick")
	return matches[1].name
end

-- Get commandline arguments
local Port,Baud,Mode=...

local UART=LuaRS232.SerialPortOpen(FindPortByDriverName(Port or DriverName),tonumber(Baud),Mode)

-- Note: Exemple usage:
--local UART=LuaRS232.SerialPortOpen("COM5",115200,"8N1")


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



repeat
	LuaRS232.Sleep(100)
	local rx=UART:Receive()
	rx=rx:gsub("%c",
		function(c)
			local d=ControlChars[c:byte()]
			if d and c~="\r" and c~="\n" then
				return ("<%s>"):format(d)
			else
				return c
			end
		end)
	rx=rx:gsub("[\r\n]+","\r\n")
	rx=rx:gsub("([^\r\n])(ipcl@)","%1\r\n%2")
	io.write(rx)
	local key=LuaRS232.GetKeyPress()
	if key==27 then
		print("Exit by keypress")
		os.exit(true)
	elseif key~=0 then
		UART:Send(string.char(key))
		-- Note: currently we rely on the echo from the other end for feeback
		-- Otherwise you might want to uncomment:
		--io.write(string.char(key))
	end
until false
