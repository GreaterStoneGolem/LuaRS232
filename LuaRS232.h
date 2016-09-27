#ifndef LuaI2C_H
#define LuaI2C_H

#include "lua.hpp"

#ifdef __cplusplus
extern "C" {
#endif

int __declspec(dllexport) luaopen_LuaRS232(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif
