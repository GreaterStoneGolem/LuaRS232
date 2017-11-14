
#include "rs232_comm.h"// Header of this .c file, doing the bridge between Lua and the RS232 C library
#include "sleep.h"// Header of my function that add sleep in Lua
#include <stdio.h>// For reasons. Maybe.
#include <string.h>// For strcat and sprintf and the likes.
#include <ctype.h>// For tolower
#include <stdlib.h>// For malloc and free
#ifdef _WIN32
#include <windows.h>// Because I use Windows API
#endif
#include "enumeratecomports.h"// Header of functions to list comm ports

// Size and buffer where data is received
#define MAX_RECEIVE 1024
unsigned char receptionbuffer[MAX_RECEIVE];

// List functions to add a to serial port
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


// All my other functions must be called by passing as first argument:
// - A Serial Comm Object, that is, a Lua table, with internal.handle as a field
// So this function look at the first object of the stack
// Check it is a table, with a element with name internal that is itself a table with an element handle which is a number
// And it returns this number casted into a handle
// Otherwise, throws a Lua error
#ifdef _WIN32
void* GetPortHandle(lua_State *L)
#else
int  GetPortHandle(lua_State *L)
#endif
{
	if(!lua_istable(L,1))
	{
		lua_pushstring(L,"Error: First stack element is not a RS-232 object");
	}
	else if(lua_getfield(L,1,"internal")!=LUA_TTABLE)
	{
		lua_pushstring(L,"Error: First stack element is a table without an internal table");
	}
	else if(lua_getfield(L,-1,"handle")!=LUA_TNUMBER)
	{
		lua_pushstring(L,"Error: First stack element is a table with an internal table without a handle number");
	}
	else
	{
		#ifdef _WIN32
			return (void*) lua_tointeger(L,-1);
		#else
			return lua_tointeger(L,-1);
		#endif
	}
	lua_error(L);
	#ifdef _WIN32
	return NULL;
	#else
	return 0;
	#endif
}


// Takes a lua_State*, check there's a table at the start of this Lua stack, and return the value of ["name"] in that table
// Not a Lua bind, but takes a lua_State* with the table where to find the name
const char* GetPortNameFromTable(lua_State* L)
{
	if(!lua_istable(L,1))
	{
		lua_pushstring(L,"Error: First stack element is not a RS-232 object");
	}
	else if(lua_getfield(L,1,"name")!=LUA_TSTRING)
	{
		lua_pushstring(L,"Error: First stack element is a table without a name string");
	}
	else
	{
		return lua_tostring(L,-1);
	}
	lua_error(L);
	return "";// Never happens, but to respect C signature
}

// Function to remove the leading \\.\ or /dev/ from a portname
const char* ShortenPortName(const char* name)
{
	if(strncmp(name,"\\\\.\\",4)==0)
	{
		return name+4;
	}
	if(strncmp(name,"/dev/",5)==0)
	{
		return name+5;
	}
	return name;
}

int LuaRS232_Sleep(lua_State* L)
{
	lua_remove(L,1);
	LuaSleep(L);
	return 1;
}


/******************/
/*    Windows     */
/******************/

#ifdef _WIN32

// Takes a number, returns a port name built from it
// Not a Lua bind, but takes a lua_State* so it can throw a Lua error
char* GetPortNameByNbr(lua_State* L,const int portnumber)
{
	static char portname[16];
	if(portnumber<1 || portnumber>99)
	{
		lua_pushfstring(L,"%d is not a valid port number",portnumber);
		lua_error(L);
	}
	sprintf(portname,"\\\\.\\COM%d",portnumber);
	return portname;
}

// Takes a port name, return the number at the end of it, or -1 if invalid name
// Not a Lua bind, but takes a lua_State* so it can throw a Lua error
int GetPortNbrByName(lua_State* L,const char* portname)
{
	const char* digits;// digits point to a constant, but digits itself isn't constant
	if(strncmp(portname,"COM",3)==0 && portname[3]!=0)
	{
		digits=&portname[3];
	}
	else if(strncmp(portname,"\\\\.\\COM",7)==0 && portname[7]!=0)
	{
		digits=&portname[7];
	}
	else
	{
		lua_pushfstring(L,"Invalid port name: \"%s\" doesn't start by \"COM\" nor by \"\\\\.\\COM\" so isn't a port name",portname);
		lua_error(L);
	}
	int n=0;
	while(*digits!=0)
	{
		n*=10;
		if(*digits>='0' && *digits<='9')
		{
			n+=*digits-'0';
		}
		else
		{
			lua_pushfstring(L,"Invalid port name: \"%s\" has '%c' which is not a digit",portname,*digits);
			lua_error(L);
		}
		++digits;
	}
	if(n<1 || n>99)
	{
		lua_pushfstring(L,"Invalid port name: \"%s\" would be port number %d which is out of range",portname,n);
		lua_error(L);
	}
	return n;
}


// Function to get a string describing the last error that happened within Windows API (Not a Lua bind)
char* GetLastErrorStr(void)
{
	DWORD errorCode = GetLastError();
	LPVOID lpMsgBuf;
	DWORD bufLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL );
	if(bufLen)
	{
		LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
		return (char*)lpMsgStr;
	}
	else
	{
		static char errmsg[32];
		sprintf(errmsg,"Unknown Error %d",errorCode);
		return errmsg;
	}
}

int LuaRS232_Open(lua_State* L)
{
	// Check that function was called with parameters of proper types
	if(lua_gettop(L)<1 ||       !(lua_isstring(L,1) || lua_isnil(L,1))
		|| (lua_gettop(L)>=2 && !(lua_isnumber(L,2) || lua_isnil(L,2)))
		|| (lua_gettop(L)>=3 && !(lua_isstring(L,3) || lua_isnil(L,3))))
	{
		lua_pushstring(L,"RS232 Open arguments must be (string) port name, plus optional (number) baudrate, (string) mode");
		return lua_error(L);
	}


	// Convert parameters from Lua stack to C variables, or use default values
	const char* portname = lua_gettop(L)>=1 && !lua_isnil(L,1) ? lua_tostring(L,1) : "COM1";
	const int   baudrate = lua_gettop(L)>=2 && !lua_isnil(L,2) ? lua_tonumber(L,2) : 115200;
	const char* mode     = lua_gettop(L)>=3 && !lua_isnil(L,3) ? lua_tostring(L,3) : "8N1";

	// Check that portname is a valid port name
	// Besides checking name validity it also returns the number of the port
	int portnumber=GetPortNbrByName(L,portname);

	// Get the port name back from port number, this way we ensure there's the leading \\.\ that CreateFile needs
	const char* portslashedname=GetPortNameByNbr(L,portnumber);

	// Check that baudrate is an allowed value
	{
		int AllowedList[]={110,300,600,1200,2400,4800,9600,19200,38400,57600,115200,128000,256000,500000,1000000};
		char IsAllowed=0;
		unsigned int bindex;
		for(bindex=0;bindex<sizeof(AllowedList)/sizeof(int);++bindex)
		{
			if(baudrate==AllowedList[bindex])
			{
				IsAllowed=1;
			}
		}
		if(!IsAllowed)
		{
			lua_pushfstring(L,"RS232 Open received an invalid baud rate: %d",baudrate);
			return lua_error(L);
		}
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
		lua_pushfstring(L,"RS232 Open received an incorrect mode: \"%s\" does not match [5-8][NOE][12]",mode);
		return lua_error(L);
	}


	// Open file handle
	void* handle = CreateFileA(
		portslashedname,
		GENERIC_READ|GENERIC_WRITE,
		0,          /* no share  */
		NULL,       /* no security */
		OPEN_EXISTING,
		0,          /* no threads */
		NULL);      /* no templates */

	if(handle==INVALID_HANDLE_VALUE)
	{
		lua_pushfstring(L,"RS232 Open (\"%s\",%d,%s) failed: Unable to open com port %s",portname,baudrate,mode,portslashedname);
		return lua_error(L);
	}


	// Create the string describing the configuration, to pass to BuildCommDCBA
	// It's in the format: COMx[:][baud=b][parity=p][data=d][stop=s][to={on|off}][xon={on|off}][odsr={on|off}][octs={on|off}][dtr={on|off|hs}][rts={on|off|hs|tg}][idsr={on|off}]
	char settings_as_string[128];
	sprintf(settings_as_string,"baud=%d parity=%c data=%d stop=%d dtr=%s rts=%s",baudrate,tolower(parity),databits,stopbits,"on","on");

	// Create a little "DCB structure", empty beside its length field
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363214
	DCB settings_as_structure;
	memset(&settings_as_structure, 0, sizeof(settings_as_structure)); /* clear the new struct */
	settings_as_structure.DCBlength = sizeof(settings_as_structure);

	// Function that parse the string to fill the DCB structure
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363143
	if(!BuildCommDCBA(settings_as_string, &settings_as_structure))
	{
		CloseHandle(handle);
		lua_pushfstring(L,"RS232 Open (\"%s\",%d,%s) failed: Unable to fill setting structure via BuildCommDCBA",portname,baudrate,mode);
		return lua_error(L);
	}

	// Configure the communication according to the DCB structure
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363436
	if(!SetCommState(handle, &settings_as_structure))
	{
		CloseHandle(handle);
		lua_pushfstring(L,"RS232 Open (\"%s\",%d,%s) failed: Unable to SetCommState",portname,baudrate,mode);
		return lua_error(L);
	}


	// Create the structure containing the time-out parameters for the communications device
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363190
	COMMTIMEOUTS CommTimeouts;
	CommTimeouts.ReadIntervalTimeout         = MAXDWORD;
	CommTimeouts.ReadTotalTimeoutMultiplier  = 0;
	CommTimeouts.ReadTotalTimeoutConstant    = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant   = 0;

	// Apply the time-out parameters
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363437
	if(!SetCommTimeouts(handle, &CommTimeouts))
	{
		CloseHandle(handle);
		lua_pushfstring(L,"RS232 Open (\"%s\",%d,%s) failed: Unable to set comport time-out settings",portname,baudrate,mode);
		return lua_error(L);
	}

	printf("Opened RS-232 communication on \"%s\".\n",ShortenPortName(portslashedname));

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

	// Add internal sub-table, to store handle (casted from void* to unsigned integer), and portnumber (from 1 to 99)
	lua_newtable(L);
		lua_pushinteger(L,(unsigned int)handle);
		lua_setfield(L,-2,"handle");
		lua_pushinteger(L,portnumber);
		lua_setfield(L,-2,"portnumber");
	lua_setfield(L,-2,"internal");

	// Add name
	lua_pushstring (L,portslashedname);
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
	if(!CloseHandle(GetPortHandle(L)))
	{
		lua_pushfstring(L,"RS232 Close failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	// Get the port name from the Lua table in the stack,
	// using a generic name instead of erroring
	// if no "name" field in the table at start of Lua stack
	const char* portname="a serial port";
	if(lua_istable(L,1) && lua_getfield(L,1,"name")==LUA_TSTRING)
	{
		portname=lua_tostring(L,-1);
	}
	printf("Closed RS-232 communication on \"%s\".\n",ShortenPortName(portname));
	return 0;
}


int LuaRS232_Send(lua_State* L)
{
	void* handle=GetPortHandle(L);

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

	// Write buffer to file handle
	int nbrbytessent;
	if(!WriteFile(handle, message, length, (LPDWORD)((void*)&nbrbytessent), NULL))
	{
		lua_pushfstring(L,"RS232 Send failed: %s",GetLastErrorStr());
		return lua_error(L);
	}

	// If this is negative, this could indicate an error
	if(nbrbytessent<0)
	{
		lua_pushfstring(L,"RS232 Send failed: Bad Write count: %d",nbrbytessent);
		return lua_error(L);
	}

	// Check all the bytes were sent
	if(nbrbytessent!=length)
	{
		lua_pushfstring(L,"RS232 Send failed: Only %d out of %d bytes sent",nbrbytessent,length);
		return lua_error(L);
	}

	// All ok
	return 0;
}



int LuaRS232_Receive(lua_State* L)
{
	void* handle=GetPortHandle(L);
	int wanted_byte_count=lua_isnumber(L,2) ? lua_tointeger(L,2) : (MAX_RECEIVE);
	int actual_byte_count=0;
	unsigned char* rbuffer=receptionbuffer;

	// If using a custom size below the static buffer size, use the static buffer
	// If using a custom size above the static buffer size, allocate a buffer dynamically
	if(wanted_byte_count>MAX_RECEIVE)
	{
		unsigned char* rbuffer=(unsigned char*)malloc(wanted_byte_count);
		if(!rbuffer)
		{
			lua_pushfstring(L,"RS232 Receive failed to allocate %d bytes",wanted_byte_count);
			return lua_error(L);
		}
	}

	if(wanted_byte_count<0)
	{
		lua_pushfstring(L,"RS232 Receive asked a negative amount of byte: %d",wanted_byte_count);
		return lua_error(L);
	}

	// Just as a precaution, erase any previous data lingering there
	memset(rbuffer,0,wanted_byte_count);

	/* Using a void pointer cast, otherwise gcc will complain about */
	/* "Warning: dereferencing type-punned pointer will break strict aliasing rules" */
	if(!ReadFile(handle, rbuffer, wanted_byte_count, (LPDWORD)((void*)&actual_byte_count), NULL))
	{
		if(rbuffer!=receptionbuffer)
		{
			free(rbuffer);
		}
		lua_pushfstring(L,"RS232 Receive failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	if(actual_byte_count<0)
	{
		if(rbuffer!=receptionbuffer)
		{
			free(rbuffer);
		}
		lua_pushfstring(L,"RS232 Receive failed: Bad Read count: %d",actual_byte_count);
		return lua_error(L);
	}
	// Print a message if no size selected at call, and default-size buffer filled up
	if((!lua_isnumber(L,2)) && (actual_byte_count==wanted_byte_count))
	{
		printf("RS232 Receive filled its %d bytes buffer!\n",actual_byte_count);
	}
	// Make the reception buffer into a Lua string
	lua_pushlstring(L,(char*)rbuffer,actual_byte_count);
	if(rbuffer!=receptionbuffer)
	{
		free(rbuffer);
	}
	return 1;
}


int LuaRS232_EnableDTR(lua_State* L)
{
	if(!EscapeCommFunction(GetPortHandle(L), SETDTR))
	{
		lua_pushfstring(L,"RS232 EnableDTR failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_DisableDTR(lua_State* L)
{
	if(!EscapeCommFunction(GetPortHandle(L), CLRDTR))
	{
		lua_pushfstring(L,"RS232 DisableDTR failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_EnableRTS(lua_State* L)
{
	if(!EscapeCommFunction(GetPortHandle(L), SETRTS))
	{
		lua_pushfstring(L,"RS232 EnableRTS failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_DisableRTS(lua_State* L)
{
	if(!EscapeCommFunction(GetPortHandle(L), CLRRTS))
	{
		lua_pushfstring(L,"RS232 DisableRTS failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_GetPinStatus(lua_State* L)
{
	int status;
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa363258
	if(!GetCommModemStatus(GetPortHandle(L), (LPDWORD)((void*)&status)))
	{
		lua_pushfstring(L,"RS232 GetPinStatus failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	lua_newtable(L);
	lua_pushboolean(L,status&MS_CTS_ON);
	lua_setfield(L,-2,"CTS");
	lua_pushboolean(L,status&MS_DSR_ON);
	lua_setfield(L,-2,"DSR");
	lua_pushboolean(L,status&MS_RING_ON);
	lua_setfield(L,-2,"RING");
	lua_pushboolean(L,status&MS_RLSD_ON);
	lua_setfield(L,-2,"RLSD");
	lua_pushboolean(L,status&MS_RLSD_ON);
	lua_setfield(L,-2,"DSD");
	return 1;
}


int LuaRS232_FlushRX(lua_State* L)
{
	if(!PurgeComm(GetPortHandle(L), PURGE_RXCLEAR | PURGE_RXABORT))
	{
		lua_pushfstring(L,"RS232 FlushRX failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_FlushTX(lua_State* L)
{
	if(!PurgeComm(GetPortHandle(L), PURGE_TXCLEAR | PURGE_TXABORT))
	{
		lua_pushfstring(L,"RS232 FlushTX failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_FlushRXTX(lua_State* L)
{
	void* handle=GetPortHandle(L);
	if(!PurgeComm(handle, PURGE_RXCLEAR | PURGE_RXABORT))
	{
		lua_pushfstring(L,"RS232 FlushRXTX failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	if(!PurgeComm(handle, PURGE_TXCLEAR | PURGE_TXABORT))
	{
		lua_pushfstring(L,"RS232 FlushRXTX failed: %s",GetLastErrorStr());
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_List(lua_State* L)
{
	unsigned int portCount;
	unsigned char hasFriendlyNames;
	int i;
	// Declare tables of names
	TCHAR portNames[MAX_PORT_NUM][MAX_STR_LEN];
	TCHAR friendlyNames[MAX_PORT_NUM][MAX_STR_LEN];
	// Initialize them at 0
	memset((void*)portNames,0,(MAX_PORT_NUM)*(MAX_STR_LEN));
	memset((void*)friendlyNames,0,(MAX_PORT_NUM)*(MAX_STR_LEN));
	hasFriendlyNames=0;

	static const char *const enumMethodNames[]={
		"CreateFile",
		"QueryDosDevice",
		"DefaultCommConfig",
		"GUID_DEVINTERFACE_COMPORT",
		"DiClassGuidsFromNamePort",
		"Registry",
		NULL};
	int enumMethodInt=luaL_checkoption(L,1,"DiClassGuidsFromNamePort",enumMethodNames);
	switch(enumMethodInt)
	{
		case 0:
			EnumerateComPortByCreateFile(&portCount, &portNames[0][0]);
			break;
		case 1:
			EnumerateComPortQueryDosDevice(&portCount, &portNames[0][0]);
			break;
		case 2:
			EnumerateComPortByGetDefaultCommConfig(&portCount, &portNames[0][0]);
			break;
		case 3:
			EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT(&portCount, &portNames[0][0], &friendlyNames[0][0]);
			hasFriendlyNames=1;
			break;
		case 4:
			EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort(&portCount, &portNames[0][0], &friendlyNames[0][0]);
			hasFriendlyNames=1;
			break;
		case 5:
			EnumerateComPortRegistry(&portCount, &portNames[0][0]);
			break;
		default:
			lua_pushfstring(L,"Invalid method name %s, please use %s %s %s %s %s or %s.",
				lua_tostring(L,1),
				enumMethodNames[0],
				enumMethodNames[1],
				enumMethodNames[2],
				enumMethodNames[3],
				enumMethodNames[4],
				enumMethodNames[5]);
			return lua_error(L);// Trigger error
	}

	// Create a new table, that will be the list of comm port
	lua_newtable(L);

	// For each port comm
	for(i=0;i<portCount;++i)
	{
		// Create a sub-table for each port comm
		lua_newtable(L);
		// First field is name
		lua_pushstring(L,portNames[i]);
		lua_setfield(L,-2,"name");
		// Second field, fullname, only with some methods
		if(hasFriendlyNames)
		{
			lua_pushstring(L,friendlyNames[i]);
			lua_setfield(L,-2,"fullname");
		}
		// Put it in the list of port comm
		lua_seti(L,-2,1+i);
	}

	return 1;
}


#elif __linux__

/******************/
/*     Linux      */
/******************/

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>

struct termios old_port_settings;
struct termios new_port_settings;

// Helper function for the Enable/Disable DTR/RTS
void ChangeStatus(lua_State* L, unsigned int bitmask, int setorclear)
{
	// Get handle from bottom of Lua stack
	int handle=GetPortHandle(L);
	// Allocate status
	int status;
	// Read status
	if(ioctl(handle,TIOCMGET,&status)<0)
	{
		lua_pushfstring(L,"RS232 Error: Unable to read port settings: %s",strerror(errno));
		lua_error(L);
	}
	// Edit status
	switch(setorclear)
	{
		case 1:// 1 for SET
			status |= bitmask;
			break;
		case -1:// -1 for CLEAR
			status &= ~bitmask;
			break;
		default:
			lua_pushfstring(L,"RS232 Error: Neither set nor clear but %d",setorclear);
			lua_error(L);
			break;
	}
	// Write back status
	if(ioctl(handle,TIOCMSET,&status)<0)
	{
		lua_pushfstring(L,"RS232 Error: Unable to set port settings: %s",strerror(errno));
		lua_error(L);
	}
}


int LuaRS232_Open(lua_State* L)
{
	// Check that function was called with parameters of proper types
	if(lua_gettop(L)<1 ||       !(lua_isstring(L,1) || lua_isnil(L,1))
		|| (lua_gettop(L)>=2 && !(lua_isnumber(L,2) || lua_isnil(L,2)))
		|| (lua_gettop(L)>=3 && !(lua_isstring(L,3) || lua_isnil(L,3))))
	{
		lua_pushstring(L,"RS232 Open arguments must be (string) port name, plus optional (number) baudrate, (string) mode");
		return lua_error(L);
	}

	// Convert parameters from Lua stack to C variables, or use default values
	const char* portname = lua_gettop(L)>=1 && !lua_isnil(L,1) ? lua_tostring(L,1) : "ttyS0";
	const int   baudrate = lua_gettop(L)>=2 && !lua_isnil(L,2) ? lua_tonumber(L,2) : 115200;
	const char* mode     = lua_gettop(L)>=3 && !lua_isnil(L,3) ? lua_tostring(L,3) : "8N1";

	// From port name, get port number, short port name, slashed port name
	const char* portshortname=ShortenPortName(portname);
	char portslashedname[256];
	strcpy(portslashedname,"/dev/");
	strncat(portslashedname,portname,200);
	// Now:
	// portname is the name we called the function with
	// portshortname is the name adjusted to never have the leading /dev/
	// portslashedname is the name adjusted to always have the leading /dev/


	int baudr;
	int status;
	int cbits = CS8;
	int cpar = 0;
	int ipar = IGNPAR;
	int bstop = 0;


	switch(baudrate)
	{
		case      50 : baudr = B50;  break;
		case      75 : baudr = B75;  break;
		case     110 : baudr = B110; break;
		case     134 : baudr = B134; break;
		case     150 : baudr = B150; break;
		case     200 : baudr = B200; break;
		case     300 : baudr = B300; break;
		case     600 : baudr = B600; break;
		case    1200 : baudr = B1200; break;
		case    1800 : baudr = B1800; break;
		case    2400 : baudr = B2400; break;
		case    4800 : baudr = B4800; break;
		case    9600 : baudr = B9600; break;
		case   19200 : baudr = B19200; break;
		case   38400 : baudr = B38400; break;
		case   57600 : baudr = B57600; break;
		case  115200 : baudr = B115200; break;
		case  230400 : baudr = B230400; break;
		case  460800 : baudr = B460800; break;
		case  500000 : baudr = B500000; break;
		case  576000 : baudr = B576000; break;
		case  921600 : baudr = B921600; break;
		case 1000000 : baudr = B1000000; break;
		case 1152000 : baudr = B1152000; break;
		case 1500000 : baudr = B1500000; break;
		case 2000000 : baudr = B2000000; break;
		case 2500000 : baudr = B2500000; break;
		case 3000000 : baudr = B3000000; break;
		case 3500000 : baudr = B3500000; break;
		case 4000000 : baudr = B4000000; break;
		default:
			lua_pushfstring(L,"RS232 Open received an invalid baud rate: %d",baudrate);
			return lua_error(L);
			break;
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
		lua_pushfstring(L,"RS232 Open received an incorrect mode: \"%s\" does not match [5-8][NOE][12]",mode);
		return lua_error(L);
	}


	switch(databits)
	{
		case 8:
			cbits = CS8;
			break;
		case 7:
			cbits = CS7;
			break;
		case 6:
			cbits = CS6;
			break;
		case 5:
			cbits = CS5;
			break;
		default:
			lua_pushfstring(L,"RS232 Open received incorrect mode \"%s\" : invalid number of data bits: %d",mode,databits);
			return lua_error(L);
	}

	switch(parity)
	{
		case 'N':
		case 'n':
			cpar = 0;
			ipar = IGNPAR;
			break;
		case 'E':
		case 'e':
			cpar = PARENB;
			ipar = INPCK;
			break;
		case 'O':
		case 'o':
			cpar = (PARENB | PARODD);
			ipar = INPCK;
			break;
		default:
			lua_pushfstring(L,"RS232 Open received an incorrect mode \"%s\" : invalid parity %c",mode,parity);
			return lua_error(L);
	}

	switch(stopbits)
	{
		case 1:
			bstop = 0;
			break;
		case 2:
			bstop = CSTOPB;
			break;
		default:
			lua_pushfstring(L,"RS232 Open received an incorrect mode \"%s\" : invalid number of stop bits %d",mode,stopbits);
			return lua_error(L);
	}

	int handle = open(portslashedname, O_RDWR | O_NOCTTY | O_NDELAY);
	if(handle==-1)
	{
		lua_pushfstring(L,"RS232 Open (\"%s\",%d,%s) failed: Unable to open com port %s : %s",portname,baudrate,mode,portslashedname,strerror(errno));
		return lua_error(L);
	}

	// Lock access so that another process can't also use the port
	if(flock(handle, LOCK_EX | LOCK_NB) != 0)
	{
		close(handle);
		lua_pushfstring(L,"RS232 Error: Port %s was already locked",portslashedname);
		return lua_error(L);
	}

	if(-1 == tcgetattr(handle, &old_port_settings))
	{
		close(handle);
		flock(handle, LOCK_UN);// Free the port so others can use it
		lua_pushfstring(L,"RS232 Error: Unable to read port %s settings",portslashedname);
		return lua_error(L);
	}

	memset(&new_port_settings, 0, sizeof(new_port_settings));// Clear the new struct
	new_port_settings.c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	new_port_settings.c_iflag = ipar;
	new_port_settings.c_oflag = 0;
	new_port_settings.c_lflag = 0;
	new_port_settings.c_cc[VMIN] = 0;// Block until n bytes are received
	new_port_settings.c_cc[VTIME] = 0;// Block until a timer expires (n * 100 mSec.)
	cfsetispeed(&new_port_settings, baudr);
	cfsetospeed(&new_port_settings, baudr);

	if(-1 == tcsetattr(handle, TCSANOW, &new_port_settings))
	{
		tcsetattr(handle, TCSANOW, &old_port_settings);
		close(handle);
		flock(handle, LOCK_UN);// Free the port so others can use it
		lua_pushfstring(L,"RS232 Error: Unable to adjust port settings");
		return lua_error(L);
	}

	if(ioctl(handle,TIOCMGET,&status)<0)
	{
		int e=errno;
		tcsetattr(handle, TCSANOW, &old_port_settings);
		flock(handle, LOCK_UN);// Free the port so others can use it
		lua_pushfstring(L,"RS232 Error: Unable to get port status: %s",strerror(e));
		return lua_error(L);
	}

	status |= TIOCM_DTR;// Turn on DTR
	status |= TIOCM_RTS;// Turn on RTS

	if(ioctl(handle,TIOCMSET,&status)<0)
	{
		int e=errno;
		tcsetattr(handle, TCSANOW, &old_port_settings);
		flock(handle, LOCK_UN);// Free the port so others can use it
		lua_pushfstring(L,"RS232 Error: Unable to set port status: %s",strerror(e));
		return lua_error(L);
	}

	printf("Opened RS-232 communication on \"%s\".\n",ShortenPortName(portslashedname));

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

	// Add internal sub-table, to store handle (note: no such thing as portnumber on linux)
	lua_newtable(L);
		lua_pushinteger(L,(unsigned int)handle);
		lua_setfield(L,-2,"handle");
		//lua_pushinteger(L,portnumber);
		//lua_setfield(L,-2,"portnumber");
	lua_setfield(L,-2,"internal");

	// Add name
	lua_pushstring(L,portslashedname);
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
	int handle=GetPortHandle(L);
	int status;
	if(ioctl(handle, TIOCMGET, &status)<0)
	{
		lua_pushfstring(L,"RS232 Error: Unable to get port status: %s",strerror(errno));
		return lua_error(L);
	}

	status &= ~TIOCM_DTR;// Turn off DTR
	status &= ~TIOCM_RTS;// Turn off RTS

	if(ioctl(handle, TIOCMSET, &status)<0)
	{
		lua_pushfstring(L,"RS232 Error: Unable to set port status: %s",strerror(errno));
		return lua_error(L);
	}

	tcsetattr(handle, TCSANOW, &old_port_settings);
	close(handle);
	flock(handle, LOCK_UN);// Free the port so others can use it

	// Get the port name from the Lua table in the stack,
	// using a generic name instead of erroring
	// if no "name" field in the table at start of Lua stack
	const char* portname="a serial port";
	if(lua_istable(L,1) && lua_getfield(L,1,"name")==LUA_TSTRING)
	{
		portname=lua_tostring(L,-1);
	}
	printf("Closed RS-232 communication on \"%s\".\n",ShortenPortName(portname));
	return 0;
}


int LuaRS232_Send(lua_State* L)
{
	int handle=GetPortHandle(L);

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

	// Write buffer to file handle
	ssize_t nbrbytessent=write(handle, message, length);

	// If this is negative, this indicate an error
	if(nbrbytessent<0)
	{
		lua_pushfstring(L,"RS232 Send failed: %s",strerror(errno));
		return lua_error(L);
	}

	// Check all the bytes were sent
	if(nbrbytessent!=length)
	{
		lua_pushfstring(L,"RS232 Send failed: Only %d out of %d bytes sent",nbrbytessent,length);
		return lua_error(L);
	}

	// All ok
	return 0;
}



int LuaRS232_Receive(lua_State* L)
{
	int handle=GetPortHandle(L);
	int wanted_byte_count=lua_isnumber(L,2) ? lua_tointeger(L,2) : (MAX_RECEIVE);
	ssize_t actual_byte_count=0;
	unsigned char* rbuffer=receptionbuffer;

	// If using a custom size below the static buffer size, use the static buffer
	// If using a custom size above the static buffer size, allocate a buffer dynamically
	if(wanted_byte_count>MAX_RECEIVE)
	{
		unsigned char* rbuffer=(unsigned char*)malloc(wanted_byte_count);
		if(!rbuffer)
		{
			lua_pushfstring(L,"RS232 Receive failed to allocate %d bytes",wanted_byte_count);
			return lua_error(L);
		}
	}

	if(wanted_byte_count<0)
	{
		lua_pushfstring(L,"RS232 Receive asked a negative amount of byte: %d",wanted_byte_count);
		return lua_error(L);
	}

	// Just as a precaution, erase any previous data lingering there
	memset(rbuffer,0,wanted_byte_count);

	// Read buffer from file handle
	actual_byte_count=read(handle, rbuffer, wanted_byte_count);
	if(actual_byte_count<0)
	{
		int e=errno;
		if(rbuffer!=receptionbuffer)
		{
			free(rbuffer);
		}
		lua_pushfstring(L,"RS232 Receive failed: %s",strerror(e));
		return lua_error(L);
	}
	// Print a message if no size selected at call, and default-size buffer filled up
	if((!lua_isnumber(L,2)) && (actual_byte_count==wanted_byte_count))
	{
		printf("RS232 Receive filled its %ld bytes buffer!\n",actual_byte_count);
	}
	// Make the reception buffer into a Lua string
	lua_pushlstring(L,(char*)rbuffer,actual_byte_count);
	if(rbuffer!=receptionbuffer)
	{
		free(rbuffer);
	}
	return 1;
}


int LuaRS232_EnableDTR(lua_State* L)
{
	ChangeStatus(L,TIOCM_DTR,1);
	return 0;
}


int LuaRS232_DisableDTR(lua_State* L)
{
	ChangeStatus(L,TIOCM_DTR,-1);
	return 0;
}


int LuaRS232_EnableRTS(lua_State* L)
{
	ChangeStatus(L,TIOCM_RTS,1);
	return 0;
}


int LuaRS232_DisableRTS(lua_State* L)
{
	ChangeStatus(L,TIOCM_RTS,-1);
	return 0;
}


int LuaRS232_GetPinStatus(lua_State* L)
{
	int handle=GetPortHandle(L);
	int status;
	int ret=ioctl(handle, TIOCMGET, &status);
	if(ret<0)
	{
		lua_pushfstring(L,"RS232 GetPinStatus failed: %s",strerror(ret));
		return lua_error(L);
	}
	lua_newtable(L);
	lua_pushboolean(L,status&TIOCM_LE);
	lua_setfield(L,-2,"LE");// Line Enable
	lua_pushboolean(L,status&TIOCM_DTR);
	lua_setfield(L,-2,"DTR");// Data Terminal Ready
	lua_pushboolean(L,status&TIOCM_RTS);
	lua_setfield(L,-2,"RTS");// Request to send
	lua_pushboolean(L,status&TIOCM_ST);
	lua_setfield(L,-2,"ST");// Secondary Transmit
	lua_pushboolean(L,status&TIOCM_SR);
	lua_setfield(L,-2,"SR");// Secondary Receive
	lua_pushboolean(L,status&TIOCM_CTS);
	lua_setfield(L,-2,"CTS");// Clear To Send
	lua_pushboolean(L,status&TIOCM_CAR);
	lua_setfield(L,-2,"CAR");// CArrier Detect
	lua_pushboolean(L,status&TIOCM_CD);
	lua_setfield(L,-2,"CD");// Carrier Detect
	lua_pushboolean(L,status&TIOCM_RNG);
	lua_setfield(L,-2,"RNG");// Ring
	lua_pushboolean(L,status&TIOCM_RNG);
	lua_setfield(L,-2,"RING");// Ring
	lua_pushboolean(L,status&TIOCM_RI);
	lua_setfield(L,-2,"RI");
	lua_pushboolean(L,status&TIOCM_DSR);
	lua_setfield(L,-2,"DSR");
	return 1;
}


int LuaRS232_FlushRX(lua_State* L)
{
	int handle=GetPortHandle(L);
	if(!tcflush(handle,TCIFLUSH))
	{
		lua_pushfstring(L,"RS232 FlushRX failed: %s",strerror(errno));
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_FlushTX(lua_State* L)
{
	int handle=GetPortHandle(L);
	if(!tcflush(handle,TCOFLUSH))
	{
		lua_pushfstring(L,"RS232 FlushTX failed: %s",strerror(errno));
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_FlushRXTX(lua_State* L)
{
	int handle=GetPortHandle(L);
	if(!tcflush(handle,TCIOFLUSH))
	{
		lua_pushfstring(L,"RS232 FlushRXTX failed: %s",strerror(errno));
		return lua_error(L);
	}
	return 0;
}


int LuaRS232_List(lua_State* L)
{
	unsigned int portCount;
	unsigned char hasFriendlyNames;
	unsigned char hasLongNames;
	int i;
	// Declare tables of names
	char PortNames[MAX_PORT_NUM][MAX_STR_LEN];
	char FriendlyNames[MAX_PORT_NUM][MAX_STR_LEN];
	char LongNames[MAX_PORT_NUM][MAX_STR_LEN];
	// Initialize them at 0
	memset((void*)PortNames,0,(MAX_PORT_NUM)*(MAX_STR_LEN));
	memset((void*)FriendlyNames,0,(MAX_PORT_NUM)*(MAX_STR_LEN));
	memset((void*)LongNames,0,(MAX_PORT_NUM)*(MAX_STR_LEN));
	hasFriendlyNames=0;
	hasLongNames=0;

	static const char *const enumMethodNames[]={
		"/sys/class/tty/",
		"/dev/serial/by-id/",
		"/dev/serial/by-path/",
		NULL};
	int enumMethodInt=luaL_checkoption(L,1,"/dev/serial/by-id/",enumMethodNames);
	switch(enumMethodInt)
	{
		case 0:
			EnumerateComPort_sys_class_tty(&portCount, PortNames, LongNames);
			hasLongNames=1;
			break;		
		case 1:
			EnumerateComPort_dev_serial_by_id(&portCount, PortNames, FriendlyNames);
			hasFriendlyNames=1;
			break;
		case 2:
			EnumerateComPort_dev_serial_by_path(&portCount, PortNames, FriendlyNames);
			hasFriendlyNames=1;
			break;
		default:
			lua_pushfstring(L,"Invalid method name %s, please use %s %s or %s.",
				lua_tostring(L,1),
				enumMethodNames[0],
				enumMethodNames[1],
				enumMethodNames[2]);
			return lua_error(L);// Trigger error*/
	}

	// Create a new table, that will be the list of comm port
	lua_newtable(L);

	// For each port comm
	for(i=0;i<portCount;++i)
	{
		// Create a sub-table for each port comm
		lua_newtable(L);
		// First field is name
		lua_pushstring(L,PortNames[i]);
		lua_setfield(L,-2,"name");
		// Extra field, fullname, only with some methods
		if(hasFriendlyNames)
		{
			lua_pushstring(L,FriendlyNames[i]);
			lua_setfield(L,-2,"fullname");
		}
		// Extra field, longname, only with some methods
		if(hasLongNames)
		{
			lua_pushstring(L,LongNames[i]);
			lua_setfield(L,-2,"longpathname");
		}
		// Put it in the list of port comm
		lua_seti(L,-2,1+i);
	}

	return 1;
}


#else
/******************/
/*    Other OS    */
/******************/
/* Not supported! */
/******************/
#endif









