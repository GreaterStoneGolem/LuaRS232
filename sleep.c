
#include "sleep.h"

#ifdef _MSC_VER
	#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
	#include <time.h> 
	#include <unistd.h> // for nanosleep
#else
	#include <unistd.h> // for usleep
#endif

int LuaSleep(lua_State *L)
{
	if(!lua_isnumber(L,1))
	{
		lua_pushstring(L,"Sleep requires as parameter a number: how many milliseconds to sleep");
		return lua_error(L);
	}
	unsigned int milliseconds=lua_tointeger(L,1);
#ifdef _MSC_VER
	Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
#else
	usleep(milliseconds * 1000);
#endif
	return 0;
}

