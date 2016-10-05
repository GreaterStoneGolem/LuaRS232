/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Teunis van Beelen
*
* Email: teuniz@gmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/

/* For more info and how to use this library, visit: http://www.teuniz.net/RS-232/ */
/* Modified by SG from original, mostly to return error strings instead of printing them */

/* Last revision by Teuniz: July 10, 2016 */
/* Last revision by SG: October 05, 2016 */


#ifndef rs232_INCLUDED
#define rs232_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>



#if defined(__linux__) || defined(__FreeBSD__)

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>

#else

#include <windows.h>

#endif

/* First parameter is Port Number, (except for first function) */
/* Return value error message in case of error, or null pointer if ok */
char* RS232_GetPortNbr(const char* inPortName, int* outPortNbr);
char* RS232_OpenPort(const int inPortNbr, const int inBaudRate, const char* inMode);
char* RS232_ClosePort(const int inPortNbr);
char* RS232_SendByte(const int inPortNbr, const unsigned char inByte);
char* RS232_SendBuffer(const int inPortNbr, const unsigned char* inBuffer, const int inSize, int* outSent);
char* RS232_SendText(const int inPortNbr, const char* inText);
char* RS232_ReadBuffer(const int inPortNbr, unsigned char* outBuffer, const int inMaxWantedSize, int* outActualSize);

// Return the value, no error possible
int RS232_IsDCDEnabled(int);
int RS232_IsCTSEnabled(int);
int RS232_IsDSREnabled(int);

// Do the action, no error possible
void RS232_EnableDTR(int);
void RS232_DisableDTR(int);
void RS232_EnableRTS(int);
void RS232_DisableRTS(int);
void RS232_FlushRX(int);
void RS232_FlushTX(int);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif


