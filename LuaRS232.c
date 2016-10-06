

#include "conversion.h"
#include "keypress.h"
#include "sleep.h"
#include "LuaRS232.h"
#include "rs232_comm.h"
#include <stdio.h>

static int PrintHelloWorld(lua_State* L)
{
	printf("Hello from \"%s:%d\"\n",__FILE__,__LINE__);
	return 0;
}


static const struct luaL_Reg LuaRS232API[] = {
	// Loading the library returns a table with:
	// - General helper functions
	// - A function to open a serial port
	// The rest of the API is provided as member functions of the object returned by a succesfull Open

	// Functions to help translate strings:
	{"Hex2Bin",Hex2Bin},
	{"Bin2Hex",Bin2Hex},
	{"InvertEndianness",InvertEndianness},

	// Functions to improve interactivity:
	{"GetKeyPress",GetKeyPress},

	// Function to wait for n milliseconds:
	{"Sleep",LuaSleep},

	// Function to list all serial port
	{"SerialPortList",LuaRS232_List},

	// Function to open a comm port, return a table with the rest of the interface
	{"SerialPortOpen",LuaRS232_Open},

	// Early test and example of C Lua bindings
	{"SayHello",PrintHelloWorld},

	{NULL,NULL}
	// This table may be recreated from the .h file with this Np++ regexp
	//    extern int LuaRS232\_(\w+)\(lua\_State\s*\*\s*L\)\;
	//    \t{"\1",LuaRS232_\1},
};


// Entry point of the API of LuaRS232.dll
// Doing a require("LuaRS232") returns one dictionary-like table with:
// - All the functions listed in the table above
int __declspec(dllexport) luaopen_LuaRS232(lua_State* L)
{
	// Create new table, which will be the library
	lua_newtable(L);

	// Add all functions to the library table
	luaL_setfuncs(L, LuaRS232API, 0);

	printf("LuaRS232 loaded!\n");
	return 1;// Return number of elements added to the stack
}
