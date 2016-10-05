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


#include "rs232.h"


/* Buffer and macro to format error messages */
#define MAX_ERR_MSG_LEN 128
char rs232errmsg1[MAX_ERR_MSG_LEN];
char rs232errmsg2[MAX_ERR_MSG_LEN];
#define return_error(...) \
	{ \
		snprintf(rs232errmsg1,MAX_ERR_MSG_LEN,__VA_ARGS__); \
		snprintf(rs232errmsg2,MAX_ERR_MSG_LEN,"Error in %s: %s", __FUNCTION__ , rs232errmsg1); \
		return rs232errmsg2; \
	}

#define return_ok \
	return (char*)0;



#if defined(__linux__) || defined(__FreeBSD__)
/*=====================*/
/*   Linux & FreeBSD   */
/*=====================*/

#define RS232_PORTNR  38

int Cport[RS232_PORTNR];
int error;

struct termios new_port_settings;
struct termios old_port_settings[RS232_PORTNR];

const char* PortsNameByNbr[RS232_PORTNR]={
	"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5",
	"/dev/ttyS6","/dev/ttyS7","/dev/ttyS8","/dev/ttyS9","/dev/ttyS10","/dev/ttyS11",
	"/dev/ttyS12","/dev/ttyS13","/dev/ttyS14","/dev/ttyS15","/dev/ttyUSB0",
	"/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4","/dev/ttyUSB5",
	"/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyACM0","/dev/ttyACM1",
	"/dev/rfcomm0","/dev/rfcomm1","/dev/ircomm0","/dev/ircomm1",
	"/dev/cuau0","/dev/cuau1","/dev/cuau2","/dev/cuau3",
	"/dev/cuaU0","/dev/cuaU1","/dev/cuaU2","/dev/cuaU3"};

char* RS232_OpenPort(const int portnbr, const int baudrate, const char* mode)
{
	int baudr, status;

	if((portnbr>=(RS232_PORTNR))||(portnbr<0))
	{
		return_error("Illegal comport number: %d",portnbr);
	}

	switch(baudrate)
	{
		case      50 : baudr = B50; break;
		case      75 : baudr = B75; break;
		case     110 : baudr = B110; break;
		case     134 : baudr = B134; break;
		case     150 : baudr = B150; break;
		case     200 : baudr = B200; break;
		case     300 : baudr = B300; break;
		case     600 : baudr = B600; break;
		case    1200 : baudr = B1200; break;
		case    1800 : baudr = B1800; break;
		case    2400 : baudr = B2400; break;
		case    4800 : baudr = B4800; break;
		case    9600 : baudr = B9600; break;
		case   19200 : baudr = B19200; break;
		case   38400 : baudr = B38400; break;
		case   57600 : baudr = B57600; break;
		case  115200 : baudr = B115200; break;
		case  230400 : baudr = B230400; break;
		case  460800 : baudr = B460800; break;
		case  500000 : baudr = B500000; break;
		case  576000 : baudr = B576000; break;
		case  921600 : baudr = B921600; break;
		case 1000000 : baudr = B1000000; break;
		case 1152000 : baudr = B1152000; break;
		case 1500000 : baudr = B1500000; break;
		case 2000000 : baudr = B2000000; break;
		case 2500000 : baudr = B2500000; break;
		case 3000000 : baudr = B3000000; break;
		case 3500000 : baudr = B3500000; break;
		case 4000000 : baudr = B4000000; break;
		default      : return_error("Invalid baudrate: %d",baudrate);
	}

	int cbits=CS8;
	int cpar=0;
	int ipar=IGNPAR;
	int bstop=0;

	if(strlen(mode) != 3)
	{
		return_error("Invalid mode: \"%s\"",mode);
	}

	switch(mode[0])
	{
		case '8': cbits = CS8; break;
		case '7': cbits = CS7; break;
		case '6': cbits = CS6; break;
		case '5': cbits = CS5; break;
		default : return_error("Invalid number of data-bits: '%c' (allowed: 5,6,7,8)",mode[0]);
	}

	switch(mode[1])
	{
		case 'N':
		case 'n':
			cpar = 0;
			ipar = IGNPAR;
			break;
		case 'E':
		case 'e':
			cpar = PARENB;
			ipar = INPCK;
			break;
		case 'O':
		case 'o':
			cpar = (PARENB | PARODD);
			ipar = INPCK;
			break;
		default :
			return_error("Invalid parity: '%c' (allowed: N,E,O)",mode[1]);
	}

	switch(mode[2])
	{
		case '1':
			bstop = 0;
			break;
		case '2':
			bstop = CSTOPB;
			break;
		default :
			return_error("Invalid number of stop bits: '%c' (allowed: 1,2)",mode[2]);
	}

	/*
	http://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html

	http://man7.org/linux/man-pages/man3/termios.3.html
	*/

	Cport[portnbr] = open(PortsNameByNbr[portnbr], O_RDWR | O_NOCTTY | O_NDELAY);
	if(Cport[portnbr]==-1)
	{
		return_error("Unable to open com port %s",PortsNameByNbr[portnbr]);
	}

	/* Lock access so that another process can't also use the port */
	if(flock(Cport[portnbr], LOCK_EX | LOCK_NB) != 0)
	{
		close(Cport[portnbr]);
		return_error("Another process has locked the comport");
	}

	error = tcgetattr(Cport[portnbr], old_port_settings + portnbr);
	if(error==-1)
	{
		close(Cport[portnbr]);
		flock(Cport[portnbr], LOCK_UN); /* free the port so that others can use it. */
		return_error("Unable to read portsettings");
	}
	memset(&new_port_settings, 0, sizeof(new_port_settings)); /* clear the new struct */

	new_port_settings.c_cflag = cbits | cpar | bstop | CLOCAL | CREAD;
	new_port_settings.c_iflag = ipar;
	new_port_settings.c_oflag = 0;
	new_port_settings.c_lflag = 0;
	new_port_settings.c_cc[VMIN] = 0;  /* block until n bytes are received */
	new_port_settings.c_cc[VTIME] = 0; /* block until a timer expires (n * 100 mSec.) */

	cfsetispeed(&new_port_settings, baudr);
	cfsetospeed(&new_port_settings, baudr);

	error = tcsetattr(Cport[portnbr], TCSANOW, &new_port_settings);
	if(error==-1)
	{
		tcsetattr(Cport[portnbr], TCSANOW, old_port_settings + portnbr);
		close(Cport[portnbr]);
		flock(Cport[portnbr], LOCK_UN); /* free the port so that others can use it. */
		return_error("Unable to adjust portsettings");
	}

	/* http://man7.org/linux/man-pages/man4/tty_ioctl.4.html */

	if(ioctl(Cport[portnbr], TIOCMGET, &status) == -1)
	{
		tcsetattr(Cport[portnbr], TCSANOW, old_port_settings + portnbr);
		flock(Cport[portnbr], LOCK_UN); /* free the port so that others can use it. */
		return_error("Unable to get portstatus");
	}

	status |= TIOCM_DTR; /* turn on DTR */
	status |= TIOCM_RTS; /* turn on RTS */

	if(ioctl(Cport[portnbr], TIOCMSET, &status) == -1)
	{
		tcsetattr(Cport[portnbr], TCSANOW, old_port_settings + portnbr);
		flock(Cport[portnbr], LOCK_UN); /* free the port so that others can use it. */
		return_error("Unable to set portstatus");
	}

	return_ok;
}



char* RS232_SendByte(const int portnbr, const unsigned char byte)
{
	int n = write(Cport[portnbr], &byte, 1);
	if((n < 0) && (errno != EAGAIN))
	{
		return_error("Error writing to port");
	}
	return_ok;
}


char* RS232_SendBuffer(const int portnbr, const unsigned char* buffer, const int size, int* sent)
{
	int n = write(Cport[portnbr], buffer, size);
	if((n < 0) && (errno != EAGAIN))
	{
		*sent=-1;
		return_error("Error writing to port");
	}
	*sent=n;
	return_ok;
}


char* RS232_ReadBuffer(const int portnbr, unsigned char* buffer, const int wanted_size,int* real_size)
{
	int n;
	n = read(Cport[portnbr], buffer, wanted_size);
	if(n < 0)
	{
		if(errno == EAGAIN)
		{
			/* EAGAIN means no data available, so it's not a real error */
			/* Change it into returning an empty string */
			n = 0;
		}
		else
		{
			return_error("Error %d during polling: %d",errorno,n);
		}
	}
	if(real_size!=(int*)0)
		*real_size=n;
	return_ok;
}




/*
Constant  Description
TIOCM_LE        DSR (data set ready/line enable)
TIOCM_DTR       DTR (data terminal ready)
TIOCM_RTS       RTS (request to send)
TIOCM_ST        Secondary TXD (transmit)
TIOCM_SR        Secondary RXD (receive)
TIOCM_CTS       CTS (clear to send)
TIOCM_CAR       DCD (data carrier detect)
TIOCM_CD        see TIOCM_CAR
TIOCM_RNG       RNG (ring)
TIOCM_RI        see TIOCM_RNG
TIOCM_DSR       DSR (data set ready)

http://man7.org/linux/man-pages/man4/tty_ioctl.4.html
*/

int RS232_IsDCDEnabled(const int portnbr)
{
	int status;
	ioctl(Cport[portnbr], TIOCMGET, &status);
	if(status&TIOCM_CAR)
		return(1);
	else
		return(0);
}


int RS232_IsCTSEnabled(const int portnbr)
{
	int status;
	ioctl(Cport[portnbr], TIOCMGET, &status);
	if(status&TIOCM_CTS)
		return(1);
	else
		return(0);
}


int RS232_IsDSREnabled(const int portnbr)
{
	int status;
	ioctl(Cport[portnbr], TIOCMGET, &status);
	if(status&TIOCM_DSR)
		return(1);
	else
		return(0);
}


void RS232_EnableDTR(const int portnbr)
{
	int status;
	if(ioctl(Cport[portnbr], TIOCMGET, &status) == -1)
	{
		perror("Unable to get portstatus");
	}
	status |= TIOCM_DTR; /* turn on DTR */
	if(ioctl(Cport[portnbr], TIOCMSET, &status) == -1)
	{
		perror("Unable to set portstatus");
	}
}


void RS232_DisableDTR(const int portnbr)
{
	int status;
	if(ioctl(Cport[portnbr], TIOCMGET, &status) == -1)
	{
		perror("Unable to get portstatus");
	}
	status &= ~TIOCM_DTR; /* turn off DTR */
	if(ioctl(Cport[portnbr], TIOCMSET, &status) == -1)
	{
		perror("Unable to set portstatus");
	}
}


void RS232_EnableRTS(const int portnbr)
{
	int status;
	if(ioctl(Cport[portnbr], TIOCMGET, &status) == -1)
	{
		perror("Unable to get portstatus");
	}
	status |= TIOCM_RTS; /* turn on RTS */
	if(ioctl(Cport[portnbr], TIOCMSET, &status) == -1)
	{
		perror("Unable to set portstatus");
	}
}


void RS232_DisableRTS(const int portnbr)
{
	int status;
	if(ioctl(Cport[portnbr], TIOCMGET, &status) == -1)
	{
		perror("Unable to get portstatus");
	}
	status &= ~TIOCM_RTS; /* turn off RTS */
	if(ioctl(Cport[portnbr], TIOCMSET, &status) == -1)
	{
		perror("Unable to set portstatus");
	}
}


void RS232_FlushRX(const int portnbr)
{
	tcflush(Cport[portnbr], TCIFLUSH);
}


void RS232_FlushTX(const int portnbr)
{
	tcflush(Cport[portnbr], TCOFLUSH);
}


void RS232_FlushRXTX(const int portnbr)
{
	tcflush(Cport[portnbr], TCIOFLUSH);
}






#else
/*=============*/
/*   Windows   */
/*=============*/

/* Function to get a string about last error */
char* GetLastErrorStr(void)
{
	DWORD errorCode = GetLastError();
	LPVOID lpMsgBuf;
	DWORD bufLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL );
	if(bufLen)
	{
		LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
		return (char*)lpMsgStr;
	}
	else
	{
		return_error("Unknown Error %d",errorCode);
	}
}


#define RS232_PORTNR  16

HANDLE Cport[RS232_PORTNR];

const char* PortsNameByNbr[RS232_PORTNR]={
	"\\\\.\\COM1",  "\\\\.\\COM2",  "\\\\.\\COM3",  "\\\\.\\COM4",
	"\\\\.\\COM5",  "\\\\.\\COM6",  "\\\\.\\COM7",  "\\\\.\\COM8",
	"\\\\.\\COM9",  "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
	"\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16"};

char mode_str[128];

char* RS232_OpenPort(const int portnbr, const int baudrate, const char* mode)
{
	if((portnbr>=(RS232_PORTNR))||(portnbr<0))
	{
		return_error("Illegal comport number: %d",portnbr);
	}

	switch(baudrate)
	{
		case     110 : strcpy(mode_str,"baud=110"); break;
		case     300 : strcpy(mode_str,"baud=300"); break;
		case     600 : strcpy(mode_str,"baud=600"); break;
		case    1200 : strcpy(mode_str,"baud=1200"); break;
		case    2400 : strcpy(mode_str,"baud=2400"); break;
		case    4800 : strcpy(mode_str,"baud=4800"); break;
		case    9600 : strcpy(mode_str,"baud=9600"); break;
		case   19200 : strcpy(mode_str,"baud=19200"); break;
		case   38400 : strcpy(mode_str,"baud=38400"); break;
		case   57600 : strcpy(mode_str,"baud=57600"); break;
		case  115200 : strcpy(mode_str,"baud=115200"); break;
		case  128000 : strcpy(mode_str,"baud=128000"); break;
		case  256000 : strcpy(mode_str,"baud=256000"); break;
		case  500000 : strcpy(mode_str,"baud=500000"); break;
		case 1000000 : strcpy(mode_str,"baud=1000000"); break;
		default      : return_error("Invalid baudrate: %d",baudrate);
	}

	if(strlen(mode) != 3)
	{
		return_error("Invalid mode: \"%s\"",mode);
	}

	switch(mode[0])
	{
		case '8':
			strcat(mode_str," data=8");
			break;
		case '7':
			strcat(mode_str," data=7");
			break;
		case '6':
			strcat(mode_str," data=6");
			break;
		case '5':
			strcat(mode_str," data=5");
			break;
		default :
			return_error("Invalid number of data-bits: '%c'",mode[0]);
	}

	switch(mode[1])
	{
		case 'N':
		case 'n':
			strcat(mode_str, " parity=n");
			break;
		case 'E':
		case 'e':
			strcat(mode_str, " parity=e");
			break;
		case 'O':
		case 'o':
			strcat(mode_str, " parity=o");
			break;
		default :
			return_error("Invalid parity: '%c'", mode[1]);
	}

	switch(mode[2])
	{
		case '1':
			strcat(mode_str," stop=1");
			break;
		case '2':
			strcat(mode_str," stop=2");
			break;
		default :
			return_error("Invalid number of stop bits '%c'",mode[2]);
	}

	strcat(mode_str," dtr=on rts=on");

	/*
	http://msdn.microsoft.com/en-us/library/windows/desktop/aa363145%28v=vs.85%29.aspx

	http://technet.microsoft.com/en-us/library/cc732236.aspx
	*/

	Cport[portnbr] = CreateFileA(
		PortsNameByNbr[portnbr],
		GENERIC_READ|GENERIC_WRITE,
		0,          /* no share  */
		NULL,       /* no security */
		OPEN_EXISTING,
		0,          /* no threads */
		NULL);      /* no templates */

	if(Cport[portnbr]==INVALID_HANDLE_VALUE)
	{
		return_error("Unable to open com port %s",PortsNameByNbr[portnbr]);
	}

	DCB port_settings;
	memset(&port_settings, 0, sizeof(port_settings)); /* clear the new struct */
	port_settings.DCBlength = sizeof(port_settings);

	if(!BuildCommDCBA(mode_str, &port_settings))
	{
		CloseHandle(Cport[portnbr]);
		return_error("Unable to set comport dcb settings");
	}

	if(!SetCommState(Cport[portnbr], &port_settings))
	{
		CloseHandle(Cport[portnbr]);
		return_error("Unable to set comport cfg settings");
	}

	COMMTIMEOUTS Cptimeouts;

	Cptimeouts.ReadIntervalTimeout         = MAXDWORD;
	Cptimeouts.ReadTotalTimeoutMultiplier  = 0;
	Cptimeouts.ReadTotalTimeoutConstant    = 0;
	Cptimeouts.WriteTotalTimeoutMultiplier = 0;
	Cptimeouts.WriteTotalTimeoutConstant   = 0;

	if(!SetCommTimeouts(Cport[portnbr], &Cptimeouts))
	{
		CloseHandle(Cport[portnbr]);
		return_error("Unable to set comport time-out settings");
	}

	return_ok;
}


char* RS232_ClosePort(const int portnbr)
{
	if(!CloseHandle(Cport[portnbr]))
	{
		return_error(GetLastErrorStr());
	}
	return_ok;
}



char* RS232_SendByte(const int portnbr, const unsigned char byte)
{
	int n;
	if(!WriteFile(Cport[portnbr], &byte, 1, (LPDWORD)((void*)&n), NULL));
	{
		return_error(GetLastErrorStr());
	}
	if(n!=1)
	{
		return_error("Bad Write count: %d",n);
	}
	return_ok;
}


char* RS232_SendBuffer(const int portnbr, const unsigned char* buffer, const int size, int* sent)
{
	if(!WriteFile(Cport[portnbr], buffer, size, (LPDWORD)((void*)sent), NULL))
	{
		return_error(GetLastErrorStr());
	}
	if(sent<0)
	{
		return_error("Bad Write count: %d",sent);
	}
	return_ok;
}


char* RS232_ReadBuffer(const int portnbr, unsigned char* buffer, const int wanted_size, int* real_size)
{
	/* Using a void pointer cast, otherwise gcc will complain about */
	/* "Warning: dereferencing type-punned pointer will break strict aliasing rules" */
	if(!ReadFile(Cport[portnbr], buffer, wanted_size, (LPDWORD)((void*)real_size), NULL))
	{
		return_error(GetLastErrorStr());
	}
	if(real_size<0)
	{
		return_error("Bad Read count: %d",real_size);
	}
	return_ok;
}


/*
http://msdn.microsoft.com/en-us/library/windows/desktop/aa363258%28v=vs.85%29.aspx
*/

int RS232_IsDCDEnabled(const int portnbr)
{
	int status;
	GetCommModemStatus(Cport[portnbr],(LPDWORD)((void*)&status));
	if(status&MS_RLSD_ON)
		return(1);
	else
		return(0);
}


int RS232_IsCTSEnabled(const int portnbr)
{
	int status;
	GetCommModemStatus(Cport[portnbr],(LPDWORD)((void*)&status));
	if(status&MS_CTS_ON)
		return(1);
	else
		return(0);
}


int RS232_IsDSREnabled(const int portnbr)
{
	int status;
	GetCommModemStatus(Cport[portnbr],(LPDWORD)((void*)&status));
	if(status&MS_DSR_ON)
		return(1);
	else
		return(0);
}


void RS232_EnableDTR(const int portnbr)
{
	EscapeCommFunction(Cport[portnbr], SETDTR);
}


void RS232_DisableDTR(const int portnbr)
{
	EscapeCommFunction(Cport[portnbr], CLRDTR);
}


void RS232_EnableRTS(const int portnbr)
{
	EscapeCommFunction(Cport[portnbr], SETRTS);
}


void RS232_DisableRTS(const int portnbr)
{
	EscapeCommFunction(Cport[portnbr], CLRRTS);
}

/*
https://msdn.microsoft.com/en-us/library/windows/desktop/aa363428%28v=vs.85%29.aspx
*/

void RS232_FlushRX(const int portnbr)
{
	PurgeComm(Cport[portnbr], PURGE_RXCLEAR | PURGE_RXABORT);
}


void RS232_FlushTX(const int portnbr)
{
	PurgeComm(Cport[portnbr], PURGE_TXCLEAR | PURGE_TXABORT);
}

#endif


/* Send a null terminated string to serial port */
char* RS232_SendText(const int portnbr, const char* text)
{
	int sent;
	int size=strlen(text);
	char* error=RS232_SendBuffer(portnbr, (const unsigned char*)text, size, &sent);
	if(error)
		return error;
	if(sent!=size)
	{
		return_error("Only sent %d out of %d bytes of text",sent,size);
	}
}


/* Gets the name of a port, returns a port number */
char* RS232_GetPortNbr(const char* PortName,int* PortNbr)
{

	// Add prefix to the portname
	char ppn[32];
	memset(ppn,0,32);
	#if defined(__linux__) || defined(__FreeBSD__) /* Linux & FreeBSD */
	strcpy(ppn,"/dev/");
	#else /* Windows */
	strcpy(ppn,"\\\\.\\");
	#endif
	strncat(ppn,PortName,30);

	// Loop over all names to find one matching
	int n;
	for(n=0;n<(RS232_PORTNR);n++)
	{
		if(!strcmp(PortsNameByNbr[n],ppn))
		{
			*PortNbr = n;
			return_ok;
		}
	}

	// Otherwise, error
	*PortNbr = -1;
	return_error("Device \"%s\" not found",PortName);
}











