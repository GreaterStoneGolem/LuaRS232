
#include "keypress.h"
#include <conio.h>

// Non blocking check for key press
// Returns 0 if no key pressed, keycode otherwise
int GetKeyPress(lua_State *L)
{
	int key=0;
	if(kbhit())// Was a key hit
	{
		key=getch();// Retrieve keycode
	}
	lua_pushinteger(L,key);
	return 1;
}

// Note:
// According to the internet,
// for arrows keys or function keys,
// getch would need to be called twice
