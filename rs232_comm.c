
#include "rs232_comm.h"// Header of this .c file, doing the bridge between Lua and the RS232 C library
#include "rs232.h"// Header of the RS232 library I got from internet
#include "sleep.h"// Header of my function that add sleep in Lua

// Size and buffer where data is received
#define MAX_RECEIVE 1024
unsigned char receptionbuffer[MAX_RECEIVE];

// List function to add a to serial port
struct luaL_Reg SerialPortAPI[] = {
	// RS-232 interface functions
	{"Open",LuaRS232_Open},// Open is to be called not from there but by what return this table
	{"Close",LuaRS232_Close},// Called automatically by garbage collector
	{"Receive",LuaRS232_Receive},
	{"Send",LuaRS232_Send},
	{"EnableDTR",LuaRS232_EnableDTR},
	{"DisableDTR",LuaRS232_DisableDTR},
	{"EnableRTS",LuaRS232_EnableRTS},
	{"DisableRTS",LuaRS232_DisableRTS},
	{"GetPinStatus",LuaRS232_GetPinStatus},
	{"FlushRX",LuaRS232_FlushRX},
	{"FlushTX",LuaRS232_FlushTX},
	{"Sleep",LuaRS232_Sleep},
	{NULL,NULL}
};


// All my other function must be called by passing as first argument:
// - A Serial Comm Object, that is, a Lua table, with internal.portnbr as a field
// So this function look at the first object of the stack
// Check it is a table, with a element with name internal that is itself a table with an element portnbr which is a number
// And it returns this number
// Otherwise, throws a Lua error
int GetPortNbr(lua_State *L)
{
	if(!lua_istable(L,1))
	{
		lua_pushstring(L,"Error: First stack element is not a RS-232 object");
	}
	else if(lua_getfield(L,1,"internal")!=LUA_TTABLE)
	{
		lua_pushstring(L,"Error: First stack element is a table without an internal table");
	}
	else if(lua_getfield(L,-1,"portnbr")!=LUA_TNUMBER)
	{
		lua_pushstring(L,"Error: First stack element is a table with an internal table without a portnbr number");
	}
	else
	{
		return lua_tointeger(L,-1);
	}
	return lua_error(L);
}


const char* GetPortName(lua_State *L)
{
	if(!lua_istable(L,1))
	{
		lua_pushstring(L,"Error: First stack element is not a RS-232 object");
	}
	else if(lua_getfield(L,1,"name")!=LUA_TSTRING)
	{
		lua_pushstring(L,"Error: First stack element is a table without an name string");
	}
	else
	{
		return lua_tostring(L,-1);
	}
	lua_error(L);
	return "";// Never happens, but to respect C signature
}




int LuaRS232_Open(lua_State* L)
{
	// Check that function was called with parameters of proper types
	if(lua_gettop(L)<1 || !lua_isstring(L,1) || (lua_gettop(L)>=2 && !lua_isnumber(L,2)) || (lua_gettop(L)>=3 && !lua_isstring(L,3)))
	{
		lua_pushstring(L,"SerialPortOpen arguments must be (string) port name, plus optional (number) baudrate, (string) mode");
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
		lua_pushfstring(L,"SerialPortOpen received an invalid port name: \"%s\"",portname);
		return lua_error(L);
	}


	// Check the validity of mode
	int databits = mode[0]-'0';
	char parity  = mode[1];
	int stopbits = mode[2]-'0';

	if( (strlen(mode)!=3)
		|| (databits!=8 && databits!=7 && databits!=6 && databits!=5)
		|| (parity!='N' && parity!='O' && parity!='E')
		|| (stopbits!=1 && stopbits!=2) )
	{
		lua_pushfstring(L,"SerialPortOpen received an incorrect mode: \"%s\" does not match [5-8][NOE][12]",mode);
		return lua_error(L);
	}


	int err=RS232_OpenComport(portnumber,baudrate,mode);
	if(err!=0)
	{
		lua_pushfstring(L,"SerialPortOpen(\"%s\",...) failed with code %d",portname,portnumber,err);
		return lua_error(L);
	}

	// Create a new table, that will be the comm object
	lua_newtable(L);

	// Add comm methods
	luaL_setfuncs(L, SerialPortAPI, 0);

	// Add settings sub-table
	lua_newtable(L);
		lua_pushinteger(L,databits);
		lua_setfield(L,-2,"databits");
		lua_pushfstring(L,"%c",parity);
		lua_setfield(L,-2,"parity");
		lua_pushinteger(L,stopbits);
		lua_setfield(L,-2,"stopbits");
		lua_pushinteger(L,baudrate);
		lua_setfield(L,-2,"baudrate");
	lua_setfield(L,-2,"settings");

	// Add internal sub-table
	lua_newtable(L);
		lua_pushinteger(L,portnumber);
		lua_setfield(L,-2,"portnbr");
	lua_setfield(L,-2,"internal");

	// Add name
	lua_pushstring (L,portname);
	lua_setfield(L,-2,"name");

	// Create a table that will be the metatable of the returned comm object
	lua_newtable(L);
	// Put into this table a new field, "_gc", with contains the function LuaRS232_Close
	lua_pushcfunction(L,LuaRS232_Close);
	lua_setfield(L,-2,"__gc");
	// Set this table as the meta table of that table
	lua_setmetatable(L,-2);

	// Return only one thing, the serial comm object
	return 1;
}


int LuaRS232_Close(lua_State* L)
{
	printf("Closing RS-232 communication on \"%s\"\n",GetPortName(L));
	RS232_CloseComport(GetPortNbr(L));
	return 0;
}

int LuaRS232_Send(lua_State* L)
{
	int port=GetPortNbr(L);

	// Check argument
	if(!lua_isstring(L,2))
	{
		lua_pushstring(L,"RS232 Send error: Require a string as first parameter!");
		return lua_error(L);
	}

	// Retrieve data to send and its length from the Lua string passed as argument
	size_t length;
	unsigned char* message=(unsigned char*)lua_tolstring(L,2,&length);
	if(length==0)
	{
		lua_pushstring(L,"RS232 Send error: I refuse to send a zero size string!");
		return lua_error(L);
	}

	// Send and verify all the bytes were sent
	int nbrbytessent=RS232_SendBuf(port,message,length);
	if(nbrbytessent!=length)
	{
		lua_pushfstring(L,"RS232 Send failed, only %d bytes out of %d sent",nbrbytessent,length);
		return lua_error(L);
	}

	return 0;
}



int LuaRS232_Receive(lua_State* L)
{
	int port=GetPortNbr(L);
	int length=RS232_PollComport(port,receptionbuffer,MAX_RECEIVE);
	if(length==MAX_RECEIVE)
	{
		printf("RS232 Receive filled its %d bytes buffer!\n",length);
	}
	lua_pushlstring(L,(char*)receptionbuffer,length);
	return 1;
}


int LuaRS232_EnableDTR(lua_State* L)
{
	RS232_enableDTR(GetPortNbr(L));
	return 0;
}


int LuaRS232_DisableDTR(lua_State* L)
{
	RS232_disableDTR(GetPortNbr(L));
	return 0;
}


int LuaRS232_EnableRTS(lua_State* L)
{
	RS232_enableRTS(GetPortNbr(L));
	return 0;
}


int LuaRS232_DisableRTS(lua_State* L)
{
	RS232_disableRTS(GetPortNbr(L));
	return 0;
}


int LuaRS232_GetPinStatus(lua_State* L)
{
	int port=GetPortNbr(L);
	lua_newtable(L);
	lua_pushboolean(L,RS232_IsDSREnabled(port));
	lua_setfield(L,-2,"DSR");
	lua_pushboolean(L,RS232_IsCTSEnabled(port));
	lua_setfield(L,-2,"CTS");
	lua_pushboolean(L,RS232_IsDCDEnabled(port));
	lua_setfield(L,-2,"DSD");
	lua_pushboolean(L,RS232_IsDSREnabled(port));
	lua_setfield(L,-2,"CTS");
	return 1;
}


int LuaRS232_FlushRX(lua_State* L)
{
	RS232_flushRX(GetPortNbr(L));
	return 0;
}


int LuaRS232_FlushTX(lua_State* L)
{
	RS232_flushTX(GetPortNbr(L));
	return 0;
}


int LuaRS232_FlushRXTX(lua_State* L)
{
	RS232_flushRXTX(GetPortNbr(L));
	return 0;
}

int LuaRS232_Sleep(lua_State* L)
{
	lua_remove(L,1);
	LuaSleep(L);
	return 1;
}


