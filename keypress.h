#ifndef KEYPRESS_H
#define KEYPRESS_H

#include "lua.hpp"

// Lua style function that returns the code of last (current?) key pressed, or 0 if none
extern int GetKeyPress(lua_State *L);

#endif
