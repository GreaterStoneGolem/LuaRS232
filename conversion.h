#ifndef CONVERSION_H
#define CONVERSION_H

#include "lua.hpp"

int Hex2Bin(lua_State *L);
int Bin2Hex(lua_State *L);
int InvertEndianness(lua_State *L);

#endif
