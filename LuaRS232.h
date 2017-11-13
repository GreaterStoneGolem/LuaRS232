#ifndef LuaI2C_H
#define LuaI2C_H

#include "lua.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
	int __declspec(dllexport) luaopen_LuaRS232(lua_State* L); // Windows
#else
	int luaopen_LuaRS232(lua_State* L); // Linux
#endif


#ifdef __cplusplus
}
#endif

#endif
