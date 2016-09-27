
#include "conversion.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Lua callable function
// Takes a string containing an hexadecimal representation of data
// Strip it of spaces, tabs, return carriage, new lines, slashes and anti-slashes
// Strip it of 0x and 0X
// Verify only hexadecimal digits are left
// Convert it to binary data
// Return that binary data as a lua string
int Hex2Bin(lua_State *L)
{
	// Check the parameter passed is a string
	if(!lua_isstring(L,1))
	{
		lua_pushstring(L,"Error in Hex2Bin: First argument isn't a string");
		return lua_error(L);
	}

	// Get the C buffer and size from the the lua string
	size_t hexlen;
	const char* hex=lua_tolstring(L,1,&hexlen);

	// If zero-sized string, don't even attempt to convert it
	if(hexlen==0)
	{
		lua_pushstring(L,"");
		return 1;
	}

	// Create a work buffer, that will first contains the hex stripped of spaces and 0x, then later the binary
	char* work=(char*)malloc(hexlen);
	if(!work)
	{
		lua_pushfstring(L,"Hex2Bin failed to allocate %d bytes",hexlen);
		return lua_error(L);
	}
	memset(work,0,hexlen);

	// Verify the string is correctly formatted, count its size
	int d=0;// Size of the hex string without spaces, tabs and 0x, so double the size of the bin string
	for(int c=0;c<hexlen;++c)
	{
		// If space or tab, skip it
		if(hex[c]==' ' || hex[c]=='\t' || hex[c]=='\r' || hex[c]=='\n' || hex[c]=='/' || hex[c]=='\\')
		{
		}
		// If an hexadecimal number, write it back in the work buffer
		else if((hex[c]>='a' && hex[c]<='f') || (hex[c]>='A' && hex[c]<='F') || (hex[c]>='0' && hex[c]<='9'))
		{
			work[d++]=hex[c];
			//printf("Hex2Bin: adding \'%c\', so far: \"%s\"\n",hex[c],work);// Debug
		}
		// If a X or X and is preceded by a 0, then ignore that 0x
		else if((hex[c]=='x' || hex[c]=='X') && d>=1 && work[d-1]=='0')
		{
			d--;
		}
		// Otherwise, it's not an allowed hexadecimal digit
		else
		{
			char nullTerminatedStringOfOneChar[2];
			nullTerminatedStringOfOneChar[0]=hex[c];
			nullTerminatedStringOfOneChar[1]=0;
			//work[d+1]=0;
			lua_pushfstring(L,"Forbidden character at %d/%d in hexadecimal string: %d = \'%s\'",1+c,hexlen,(unsigned char)hex[c],nullTerminatedStringOfOneChar);
			free(work);
			return lua_error(L);
		}
	}

	// So work buffer is now the string stripped of spaces and x, and d is its size

	if(d&1)
	{
		lua_pushfstring(L,"Odd number of hexadecimal digits %d",d);
		free(work);
		return lua_error(L);
	}

	// Replace pairs of hex digit by binary bites, in place
	int b;
	char hexnumber[3];
	hexnumber[2]=0;
	for(int c=0;c<d;c+=2)
	{
		hexnumber[0]=work[c];
		hexnumber[1]=work[c+1];
		b=strtol(hexnumber,NULL,16);
		work[c/2]=b;
	}

	// So work is now the binary string, and d/2 is its size

	// Push the binary data on the stack as a Lua string
	// Yes, Lua string can be binary data, including zero
	lua_pushlstring(L,work,d/2);

	// Lua made a copy when it was pushed on the stack, so we can free our buffer now
	free(work);

	return 1;

}



// Lua callable function
// Takes a Lua string containing binary data
// Convert it to a string of space separated pairs of hexadecimal digit
// Return that as a Lua string
int Bin2Hex(lua_State *L)
{
	// Check the parameter passed is a string
	if(!lua_isstring(L,1))
	{
		lua_pushstring(L,"Error in Bin2Hex: First argument isn't a string");
		return lua_error(L);
	}

	// Get the C buffer and size from the the lua string
	size_t binlen;
	const char* bin=lua_tolstring(L,1,&binlen);

	// If zero-sized string, don't even attempt to convert it
	if(binlen==0)
	{
		lua_pushstring(L,"");
		return 1;
	}

	// Create a hex output buffer
	char* hex=(char*)malloc(binlen*3);
	if(!hex)
	{
		lua_pushfstring(L,"Bin2Hex failed to allocate %d bytes",3*binlen);
		return lua_error(L);
	}

	int h=0;
	char tmp[3];
	for(int b=0;b<binlen;++b)
	{
		sprintf(tmp,"%02X",(unsigned char)bin[b]);
		hex[h++]=tmp[0];
		hex[h++]=tmp[1];
		hex[h++]=32;// Space
	}
	hex[--h]=0;// Remove last space, add terminator
	//printf("Bin2Hex -> \"%s\"\n",hex);// Debug

	// Push the hex translation on the Lua stack
	lua_pushlstring(L,hex,h);

	// Lua made a copy when it was pushed on the stack, so we can free our buffer now
	free(hex);

	return 1;

}


int InvertEndianness(lua_State *L)
{
	// Check the parameter passed is a string
	if(!lua_isstring(L,1))
	{
		lua_pushstring(L,"Error in InvertEndianness: First argument isn't a string");
		return lua_error(L);
	}

	// Get the C buffer and size from the the lua string
	size_t len;
	const char* in=lua_tolstring(L,1,&len);

	// Create output buffer
	char* out=(char*)malloc(len);
	if(!out)
	{
		lua_pushfstring(L,"InvertEndianness failed to allocate %d bytes",len);
		return lua_error(L);
	}

	// Recopy while inverting
	for(int b=0;b<len;++b)
	{
		out[b]=in[len-1-b];
	}

	// Push result on Lua stack
	lua_pushlstring(L,out,len);

	return 1;
}


