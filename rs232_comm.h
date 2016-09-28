#ifndef RS232_COMM_H
#define RS232_COMM_H

#include "lua.hpp"

// Wrappers to make Lua style function out of http://www.teuniz.net/RS-232/

extern int LuaRS232_Open(lua_State* L);
extern int LuaRS232_Close(lua_State* L);
extern int LuaRS232_Receive(lua_State* L);
extern int LuaRS232_Send(lua_State* L);

extern int LuaRS232_EnableDTR(lua_State* L);
extern int LuaRS232_DisableDTR(lua_State* L);
extern int LuaRS232_EnableRTS(lua_State* L);
extern int LuaRS232_DisableRTS(lua_State* L);

extern int LuaRS232_GetPinStatus(lua_State* L);

extern int LuaRS232_FlushRX(lua_State* L);
extern int LuaRS232_FlushTX(lua_State* L);
extern int LuaRS232_FlushRXTX(lua_State* L);

extern int LuaRS232_Sleep(lua_State* L);


#endif
