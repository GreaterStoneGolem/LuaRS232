#!/usr/local/bin/lua

-- Script to send a file via XModem
--
-- Takes at least two arguments: The name of the file to upload and the name of the serial port (either port or driver name)
-- Optionally also take baud rate and mode if provided, use defaults otherwise
--
-- Example usage:
--   lua53.exe SendFileXModem.lua myfile.bin COM5
-- or:
--   lua-5.3.1\bin\lua53.exe SendFileXModem.lua "C:\Folder\myfile.bin" "Acme USB-to-Serial Bridge" 115200 "8N1"
--
-- This only implements basic XMODEM
-- This does NOT support MODEM7, TeLink, XMODEM-1K, XMODEM-CRC, YMODEM, ZMODEM, WXModem, ...


-- Get arguments from commandline
local FileToSend,PortOrDriverName,BaudRate,Mode=...


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


-- Open the port
local UART=OpenPortByDriverOrPortName(PortOrDriverName,BaudRate,Mode)


-- A function to immediatly exit the script if the esc key is pressed
local function EscIfEsc()
	if LuaRS232.GetKeyPress()==27 then
		print("Exit by keypress")
		os.exit(true)
	end
end


-- Implemented following https://en.wikipedia.org/wiki/XMODEM
local function SendStringXModem(data)
	print("Starting XModem")
	assert(data and #data>100,"Too small payload, that's suspicious")

	-- A few control character constants
	local CTRL_C="\x03"
	local SOH="\x01"
	local EOT="\x04"
	local ACK="\x06"
	local NAK="\x15"
	local SUB="\x1A"

	print("Waiting for initial NAK")
	repeat
		EscIfEsc()
		LuaRS232.Sleep(100)
		local r=UART:Receive()
		-- Print the characters received during the wait for NAK
		io.write((r-- Double the parentheses to drop gsub second value
			:gsub("[^%g\032\t\r\n]",-- For characters that are not printable nor whitespace
				function(c)
					return ("\\x%02X"):format(c:byte())-- Escape sequence for unprintable characters
				end)
			:gsub("[\r\n]+"," ")))-- Replace any new lines and return carriages by a single space
	until r:match(NAK)
	print("")
	print("Got starting NAK!")

	local BlockNumber=0
	for ByteNumber=1,#data,128 do
		-- Increment block number
		BlockNumber=BlockNumber+1
		local errorcount=0
		repeat
			-- Abort if Esc key is pressed
			EscIfEsc()
			-- Small delay in chunk loop
			LuaRS232.Sleep(10)
			-- Extracts 128 byte chunk from the full data
			local chunk=data:sub(ByteNumber,ByteNumber+127)
			-- Pad chunk up to 128 bytes (in case it's the last chunk)
			chunk=chunk..(SUB):rep(128-#chunk)
			-- Calculate checksum over the chunk
			local checksum=0
			for b=1,#chunk do
				checksum=(checksum+string.byte(chunk:sub(b,b)))%256
			end
			-- Create the packet
			local packet=SOH..string.char(BlockNumber%256)..string.char(255-BlockNumber%256)..chunk..string.char(checksum)
			-- Print the packet content
			--print(LuaRS232.Bin2Hex(packet))
			-- Print progression for first five blocks, then every 50 blocks
			if ByteNumber<=5*128 or BlockNumber%50==0 then
				print(("%3d%%    Block: %d/%d    Byte: %d/%d"):format(
					math.floor(100*ByteNumber/#data),
					BlockNumber,math.ceil(#data/128),
					ByteNumber+127,#data))
			end
			-- Send the packet
			UART:Send(packet)
			-- Wait for answer
			local r=""
			repeat
				-- Pause a tiny bit in innermost loop
				LuaRS232.Sleep(10)
				r=UART:Receive(1)
				-- Only the reception of a NAK or ACK can exit this inner loop
				if r:match(NAK) or r:match(ACK) then
					break
				elseif #r>0 then
					print("During XModem, received neither ACK nor NAK but 0x "..LuaRS232.Bin2Hex(r).." = \""..r.."\"")
					LuaRS232.Sleep(1000)
				end
				EscIfEsc()
			until false
			-- If received a NAK, increment error count (for this block) and abort after about ten errors
			if r:match(NAK) then
				print("Block "..BlockNumber.." failed with 0x"..LuaRS232.Bin2Hex(r):gsub("%s+","").." = \""..r.."\"")
				errorcount=errorcount+1
				if errorcount>=10 then
					error(errorcount.." packets failed during XMODEM, aborting")
				end
			end
			if #r>1 then-- For old version or LuaRS232 library that do not support receiving only 1 byte
				-- Put everything after first ack into the to-print buffer, so as to not lose final answer message
				rx=r:match("%"..ACK.."(.+)") or ""
			end
		until r:match(ACK)
	end
	-- Mark file transfer as complete with a lone EOT
	UART:Send(EOT)
	print("XModem Successful")
end


local function SendFileXModem(filename)
	assert(filename,"Please input a filename")
	print("Sending the file "..filename.." via XModem")
	SendStringXModem(assert(io.open(filename,"rb"),"Could not open "..filename):read("*a"))
end


SendFileXModem(FileToSend)
