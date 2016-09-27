

#include "rs232_comm.h"// Header of this .c file, doing the bridge between Lua and the RS232 C library
#include "rs232.h"// Header of the RS232 library I got from internet
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <unistd.h> // for nanosleep

// Size and buffer where data is received
#define MAX_RECEIVE 1024
unsigned char receptionbuffer[MAX_RECEIVE];

// Because the lib I wrap requires it at every step, but I only want to precise it during open
// I create this global holding the port number of the open port, or -1 if none yet opened
int currentportnumber=-1;



int LuaRS232_Open(lua_State* L)
{
	// I don't want to deal with supporting multiple open serial comm in the same time for now
	if(currentportnumber>=0)
	{
		lua_pushstring(L,"RS232 Open error: There's already one open, and multiple simultaneous serial communications not supported yet.");
		return lua_error(L);
	}

	// Check that function was called with parameters of proper types
	if(lua_gettop(L)<1 || !lua_isstring(L,1) || (lua_gettop(L)>=2 && !lua_isnumber(L,2)) || (lua_gettop(L)>=3 && !lua_isstring(L,3)))
	{
		lua_pushstring(L,"RS232 Open arguments must be (string) port name, plus optional (number) baudrate, (string) mode");
		return lua_error(L);
	}

	// Convert parameters from Lua stack to C variables, or use default values
	const char* portname = lua_gettop(L)>=1 ? lua_tostring(L,1) : "COM1";
	const int   baudrate = lua_gettop(L)>=2 ? lua_tonumber(L,2) : 115200;
	const char* mode     = lua_gettop(L)>=3 ? lua_tostring(L,3) : "8N1";

	// RS232_GetPortnr converts a port name into a port number
	/*
       +-------------+-------+---------+
       | Port Number | Linux | Windows |
       +-------------+-------+---------+
       |       0     | ttyS0 | COM1    |
       |       1     | ttyS1 | COM2    |
       |       2     | ttyS2 | COM3    |
       |       3     | ttyS3 | COM4    |
       |       0     | ...   | ...     |
    Beware, Windows numbering start at 1,
    but Linux numbering and
   this lib internal numbering start at 0.
	*/

	int portnumber=RS232_GetPortnr(portname);
	// returns -1 if invalid port name
	if(portnumber<0)
	{
		lua_pushfstring(L,"RS232 Open received an invalid port name: \"%s\"",portname);
		return lua_error(L);
	}


	// Check the validity of mode
	char databits = mode[0];
	char parity   = mode[1];
	char stopbit  = mode[2];

	printf("Trying to open %s (%d) at %d bauds with %c data bits, %c parity, %c stop bits\n",portname,portnumber,baudrate,databits,parity,stopbit);

	if( (strlen(mode)!=3)
		|| (databits!='8' && databits!='7' && databits!='6' && databits!='5')
		|| (parity!='N' && parity!='O' && parity!='E')
		|| (stopbit!='1' && stopbit!='2') )
	{
		lua_pushfstring(L,"RS232 Open received an incorrect mode: \"%s\" does not match [5-8][NOE][12]",mode);
		return lua_error(L);
	}


	int err=RS232_OpenComport(portnumber,baudrate,mode);
	if(err!=0)
	{
		lua_pushfstring(L,"RS232 Open %s (%d) failed with code %d",portname,portnumber,err);
		return lua_error(L);
	}

	currentportnumber=portnumber;


	return 0;

}


int LuaRS232_Close(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 Close error: No open port to close!");
		return lua_error(L);
	}
	RS232_CloseComport(currentportnumber);
	currentportnumber=-1;
	return 0;
}

int LuaRS232_Send(lua_State* L)
{
	// Check there's a port open
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 Send error: No open port!");
		return lua_error(L);
	}

	// Check argument
	if(!lua_isstring(L,1))
	{
		lua_pushstring(L,"RS232 Send error: Require a string as first parameter!");
		return lua_error(L);
	}

	// Retrieve data to send and its length from the Lua string passed as argument
	size_t length;
	unsigned char* message=(unsigned char*)lua_tolstring(L,1,&length);
	if(length==0)
	{
		lua_pushstring(L,"RS232 Send error: I refuse to send a zero size string!");
		return lua_error(L);
	}

	// Send and verify all the bytes were sent
	int nbrbytessent=RS232_SendBuf(currentportnumber,message,length);
	if(nbrbytessent!=length)
	{
		lua_pushfstring(L,"RS232 Send failed, only %d bytes out of %d sent",nbrbytessent,length);
		return lua_error(L);
	}

	return 0;
}



int LuaRS232_Receive(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 Receive error: No open port!");
		return lua_error(L);
	}
	int length=RS232_PollComport(currentportnumber,receptionbuffer,MAX_RECEIVE);
	if(length==MAX_RECEIVE)
	{
		printf("RS232 Receive filled its %d bytes buffer!\n",length);
	}
	lua_pushlstring(L,(char*)receptionbuffer,length);
	return 1;
}


int LuaRS232_EnableDTR(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 EnableDTR error: No open port!");
		return lua_error(L);
	}
	RS232_enableDTR(currentportnumber);
	return 0;
}


int LuaRS232_DisableDTR(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 DisableDTR error: No open port!");
		return lua_error(L);
	}
	RS232_disableDTR(currentportnumber);
	return 0;
}


int LuaRS232_EnableRTS(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 EnableRTS error: No open port!");
		return lua_error(L);
	}
	RS232_enableRTS(currentportnumber);
	return 0;
}


int LuaRS232_DisableRTS(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 DisableRTS error: No open port!");
		return lua_error(L);
	}
	 RS232_disableRTS(currentportnumber);
	return 0;
}


int LuaRS232_GetPinStatus(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 GetPinStatus error: No open port!");
		return lua_error(L);
	}
	lua_newtable(L);
	lua_pushboolean(L,RS232_IsDSREnabled(currentportnumber));
	lua_setfield(L,-2,"DSR");
	lua_pushboolean(L,RS232_IsCTSEnabled(currentportnumber));
	lua_setfield(L,-2,"CTS");
	lua_pushboolean(L,RS232_IsDCDEnabled(currentportnumber));
	lua_setfield(L,-2,"DSD");
	lua_pushboolean(L,RS232_IsDSREnabled(currentportnumber));
	lua_setfield(L,-2,"CTS");
	return 1;
}


int LuaRS232_FlushRX(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 LuaRS232_FlushRX error: No open port!");
		return lua_error(L);
	}
	RS232_flushRX(currentportnumber);
	return 0;
}


int LuaRS232_FlushTX(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 LuaRS232_FlushTX error: No open port!");
		return lua_error(L);
	}
	RS232_flushTX(currentportnumber);
	return 0;
}


int LuaRS232_FlushRXTX(lua_State* L)
{
	if(currentportnumber<0)
	{
		lua_pushstring(L,"RS232 LuaRS232_LuaRS232_FlushRXTX error: No open port!");
		return lua_error(L);
	}
	RS232_flushRXTX(currentportnumber);
	return 0;
}


