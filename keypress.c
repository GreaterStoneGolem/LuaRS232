
// Somewhat crossplatform (not really) way to retrieve key press
//
// On Windows, use conio
// On Linux, use https://stackoverflow.com/a/2985246
// Otherwise, use getchar(), which require the user to press enter

#include "keypress.h"


#ifdef _WIN32

// For windows:
// Non blocking check for key press
// Returns 0 if no key pressed, keycode otherwise

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


#elif __linux__

// For Linux
// Non blocking check for key press
// Returns EOF if no key pressed, character otherwise

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

int GetKeyPress(lua_State *L)
{
	int character;
	struct termios orig_term_attr;
	struct termios new_term_attr;

	/* set the terminal to raw mode */
	tcgetattr(fileno(stdin), &orig_term_attr);
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ECHO|ICANON);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 0;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);

	/* read a character from the stdin stream without blocking */
	/*   returns EOF (-1) if no character is available */
	character = fgetc(stdin);

	/* restore the original terminal attributes */
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);

	// Change EOF to 0, to get a consistent interface with conio
	if(character==EOF)
		character=0;

	lua_pushinteger(L,character);
	return 1;
}


#else

// For other OS
// Print message telling to press enter or Ctrl+C
// Wait till user press enter, and return something
// which seems to be 10='\n' usually, but 27 if esc was pressed before enter

#include <stdio.h>

int GetKeyPress(lua_State *L)
{
	printf("Enter for next, Ctrl+C to escape\n");
	int character = getchar();
	lua_pushinteger(L,character);
	return 1;
}

#endif