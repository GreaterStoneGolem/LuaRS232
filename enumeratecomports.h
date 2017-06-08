


/*
	Enumerating com ports in Windows

	Original by PJ Naughter, 1998 - 2013
	http://www.naughter.com/enumser.html  (Web: www.naughter.com, Email: pjna@naughter.com)

	Reorganized by Gaiger Chen , Jul, 2015
	http://gaiger-programming.blogspot.fr/2015/07/methods-collection-of-enumerating-com.html

	Slightly modified by SG, Oct, 2016

	No copyright, welcome to use for everyone.
*/


#define MAX_PORT_NUM      (256)
#define MAX_STR_LEN       (256*sizeof(TCHAR))

BOOL EnumerateComPortByCreateFile(UINT* pPortCount, TCHAR* pPortNames);
BOOL EnumerateComPortQueryDosDevice(UINT* pPortCount, TCHAR* pPortNames);
BOOL EnumerateComPortByGetDefaultCommConfig(UINT* pPortCount, TCHAR* pPortNames);
BOOL EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT(UINT* pPortCount, TCHAR* pPortNames, TCHAR* pFriendlyNames);
BOOL EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort(UINT* pPortCount, TCHAR* pPortNames, TCHAR* pFriendlyNames);
BOOL EnumerateComPortRegistry(UINT* pPortCount, TCHAR* pPortNames);
void BenchmarkEnumComPorts(void);

/*
	The output is :


	CreateFile method :
	EnumerateComPortByCreateFile cost time = 35 ms
	sought 5:
			\\.\COM1
			\\.\COM4
			\\.\COM5
			\\.\COM98
			\\.\COM99

	QueryDosDevice method : EnumerateComPortQueryDosDevice cost time = 7 ms
	sought 5:
			COM5
			COM1
			COM98
			COM99
			COM4

	GetDefaultCommConfig method :
	EnumerateComPortByGetDefaultCommConfig cost time = 772 ms
	sought 5:
			COM1
			COM4
			COM5
			COM98
			COM99

	SetupAPI GUID_DEVINTERFACE_COMPORT method :
	EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT cost time = 12 ms
	sought 2:
			COM1 <通訊連接埠>
			COM4 <USB Serial Port>

	SetupAPI SetupDiClassGuidsFromNamePort method :
	EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort cost time = 10 ms
	sought 5:
			COM99 <Bluetooth Serial Port>
			COM4 <USB Serial Port>
			COM1 <通訊連接埠>
			COM98 <Bluetooth Serial Port>
			COM5 <USB-SERIAL CH340>

	Registry method :
	EnumerateComPortRegistry cost time = 0 ms
	sought 5:
			COM98
			COM99
			COM5
			COM1
			COM4
*/
