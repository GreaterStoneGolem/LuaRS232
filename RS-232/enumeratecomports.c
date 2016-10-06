

/*
	Enumerating com ports in Windows

	Original by PJ Naughter, 1998 - 2013
	http://www.naughter.com/enumser.html  (Web: www.naughter.com, Email: pjna@naughter.com)

	Reorganized by Gaiger Chen , Jul, 2015
	http://gaiger-programming.blogspot.fr/2015/07/methods-collection-of-enumerating-com.html

	Slightly modified by SG, Oct, 2016

	No copyright, welcome to use for everyone.
*/

#include <windows.h>

#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>
#include <locale.h>

#include "enumeratecomports.h"

/* Todo: Handle that properly */
//#include "winnt.h"
#include <initguid.h>
#include <tchar.h>
#include <ctype.h>
#include <ddk/ntddser.h>

#define _stprintf_s snprintf
#define _tcsnlen strnlen
#define VER_PLATFORMID 0x0000008
#define VER_EQUAL 1
//ULONGLONG WINAPI VerSetConditionMask(ULONGLONG,DWORD,BYTE);
#define VER_SET_CONDITION(ConditionMask, TypeBitMask, ComparisonType)  \
	((ConditionMask) = VerSetConditionMask((ConditionMask), \
	(TypeBitMask), (ComparisonType)))
#define _countof(array) (sizeof(array)/sizeof(TCHAR))
#define _tcscat_s(a,b,c) strncat(a,c,b)
#define GUID_CLASS_COMPORT GUID_DEVINTERFACE_COMPORT


// On some older MinGW version strnlen was missing so I had to redefine it

/*size_t strnlen(const char * s, size_t maxlen )
{
	int k;
	for(k=0;k<maxlen;++k)
	{
		if(s[k]==0)
			return k;
	}
	return maxlen;
}*/

/* Assure portName be double ARRAY , not double point */
BOOL EnumerateComPortByCreateFile(UINT* pPortCount, TCHAR* pPortNames)
{
	UINT i, jj;
	INT ret;
	TCHAR* pTempPortNames;

	*pPortCount = 0;
	jj = 0;
	pTempPortNames = (TCHAR*)HeapAlloc(
			GetProcessHeap(),
			HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
			(MAX_STR_LEN)*sizeof(pTempPortNames));

	ret = FALSE;

	for(i = 1; i<= 255; i++)
	{
		HANDLE hSerial;

		_stprintf_s(pTempPortNames, (MAX_STR_LEN), TEXT("\\\\.\\COM%u"), i);

		hSerial = CreateFile(pTempPortNames, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

		if(INVALID_HANDLE_VALUE == hSerial)
		{
			CloseHandle(hSerial);
			continue;
		}

		_tcsncpy(pPortNames + jj*(MAX_STR_LEN), pTempPortNames,
			_tcsnlen(pTempPortNames, (MAX_STR_LEN)));

		// Gaiger Chen had forgotten this, pretty important when I want to use the port later in the same program
		CloseHandle(hSerial);

		jj++;
	}/* end for [i MAX_PORT_NUM] */

	HeapFree(GetProcessHeap(), 0, pTempPortNames);
	pTempPortNames = NULL;
	*pPortCount = jj;

	if(0 <jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortByCreateFile */




BOOL EnumerateComPortQueryDosDevice(UINT* pPortCount, TCHAR* pPortNames)
{
	UINT i, jj;
	INT ret;

	OSVERSIONINFOEX osvi;
	ULONGLONG dwlConditionMask;
	DWORD dwChars;

	TCHAR* pDevices;
	UINT nChars;

	ret = FALSE;

	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;
	dwlConditionMask = 0;

	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

	if(FALSE == VerifyVersionInfo(&osvi, VER_PLATFORMID, dwlConditionMask))
	{
		DWORD dwError = GetLastError();
		_tprintf(TEXT("VerifyVersionInfo error, %d\n"), dwError);
		return -1;
	}/* end if */


	pDevices = NULL;

	nChars = 4096;
	pDevices = (TCHAR*)HeapAlloc(GetProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS, nChars*sizeof(TCHAR));

	while(0 < nChars)
	{
		dwChars = QueryDosDevice(NULL, pDevices, nChars);

		if(0 == dwChars)
		{
			DWORD dwError = GetLastError();
			if(ERROR_INSUFFICIENT_BUFFER == dwError)
			{
				nChars *= 2;
				HeapFree(GetProcessHeap(), 0, pDevices);
				pDevices = (TCHAR*)HeapAlloc(GetProcessHeap(),
					HEAP_GENERATE_EXCEPTIONS, nChars*sizeof(TCHAR));
				continue;
			}/* end if ERROR_INSUFFICIENT_BUFFER == dwError*/
			_tprintf(TEXT("QueryDosDevice error, %d\n"), dwError);
			return -1;
		}/* end if */

		//printf("dwChars = %d\n", dwChars);
		i = 0;
		jj = 0;
		while(TEXT('\0') != pDevices[i])
		{
			TCHAR* pszCurrentDevice;
			size_t nLen;
			pszCurrentDevice = &(pDevices[i]);
			nLen = _tcslen(pszCurrentDevice);
			//_tprintf(TEXT("%s\n"), &pTargetPathStr[i]);
			if(3 < nLen)
			{
				if((0 == _tcsnicmp(pszCurrentDevice, TEXT("COM"), 3))
						&& FALSE != isdigit(pszCurrentDevice[3]) )
				{
					//Work out the port number
					_tcsncpy(pPortNames + jj*(MAX_STR_LEN), pszCurrentDevice, (MAX_STR_LEN));
					jj++;
				}
			}
			i += (nLen + 1);
		}
		break;
	}/* end while */

	if(NULL != pDevices)
		HeapFree(GetProcessHeap(), 0, pDevices);

	*pPortCount = jj;

	if(0 < jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortByQueryDosDevice */




BOOL EnumerateComPortByGetDefaultCommConfig(UINT* pPortCount, TCHAR* pPortNames)
{
	UINT i, jj;
	INT ret;

	TCHAR* pTempPortNames;

	pTempPortNames = (TCHAR*)HeapAlloc(
		GetProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS| HEAP_ZERO_MEMORY, (MAX_STR_LEN));

	*pPortCount = 0;
	jj = 0;
	ret = FALSE;

	for(i = 1; i<=255; i++)
	{
		// Form the raw device name
		COMMCONFIG cc;
		DWORD dwSize ;
		dwSize = sizeof(COMMCONFIG);
		_stprintf_s(pTempPortNames, (MAX_STR_LEN)/2, TEXT("COM%u"), i);
		if(FALSE == GetDefaultCommConfig(pTempPortNames, &cc, &dwSize))
			continue;
		_tcsncpy(pPortNames + jj*(MAX_STR_LEN), pTempPortNames, _tcsnlen(pTempPortNames, (MAX_STR_LEN)));
		jj++;
	}/* end for [1 255] */

	HeapFree(GetProcessHeap(), 0, pTempPortNames);
		pTempPortNames = NULL;

	*pPortCount = jj;

	if(0 <jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortByGetDefaultCommConfig*/




BOOL EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT(UINT* pPortCount, TCHAR* pPortNames, TCHAR* pFriendlyNames)
{
	UINT i, jj;
	INT ret;

	TCHAR* pTempPortNames;
	HMODULE hLibrary;
	TCHAR szFullPath[_MAX_PATH];

	GUID guid;
	HDEVINFO hDevInfoSet;

	typedef HKEY (__stdcall SetupDiOpenDevRegKeyFunType)
		(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);

	//typedef BOOL (__stdcall SetupDiClassGuidsFromNameFunType)
	// (LPCTSTR, LPGUID, DWORD, PDWORD);

	typedef BOOL (__stdcall SetupDiDestroyDeviceInfoListFunType)
		(HDEVINFO);
	typedef BOOL (__stdcall SetupDiEnumDeviceInfoFunType)
		(HDEVINFO, DWORD, PSP_DEVINFO_DATA);

	typedef HDEVINFO (__stdcall SetupDiGetClassDevsFunType)
		(LPGUID, LPCTSTR, HWND, DWORD);

	typedef BOOL (__stdcall SetupDiGetDeviceRegistryPropertyFunType)
		(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	SetupDiOpenDevRegKeyFunType* SetupDiOpenDevRegKeyFunPtr;

	SetupDiGetClassDevsFunType *SetupDiGetClassDevsFunPtr;
	SetupDiGetDeviceRegistryPropertyFunType *SetupDiGetDeviceRegistryPropertyFunPtr;

	SetupDiDestroyDeviceInfoListFunType *SetupDiDestroyDeviceInfoListFunPtr;
	SetupDiEnumDeviceInfoFunType *SetupDiEnumDeviceInfoFunPtr;

	BOOL bMoreItems;
	SP_DEVINFO_DATA devInfo;

	ret = FALSE;
	jj = 0;
	szFullPath[0] = _T('\0');

	// Get the Windows System32 directory
	if(0 == GetSystemDirectory(szFullPath, _countof(szFullPath)))
	{
		_tprintf(TEXT("CEnumerateSerial::UsingSetupAPI1 failed, Error:%u\n"),GetLastError());
		return FALSE;
	}/* end if */

	// Setup the full path and delegate to LoadLibrary
#pragma warning(suppress: 6102) // There is a bug with the SAL annotation of GetSystemDirectory in the Windows 8.1 SDK
	_tcscat_s(szFullPath, _countof(szFullPath), _T("\\"));
	_tcscat_s(szFullPath, _countof(szFullPath), TEXT("SETUPAPI.DLL"));
	hLibrary = LoadLibrary(szFullPath);

	SetupDiOpenDevRegKeyFunPtr =
		(SetupDiOpenDevRegKeyFunType*)GetProcAddress(hLibrary, "SetupDiOpenDevRegKey");

#if defined _UNICODE
	SetupDiGetClassDevsFunPtr = (SetupDiGetClassDevsFunType*)
		GetProcAddress(hLibrary, "SetupDiGetClassDevsW");
	SetupDiGetDeviceRegistryPropertyFunPtr = (SetupDiGetDeviceRegistryPropertyFunType*) 
		GetProcAddress(hLibrary, "SetupDiGetDeviceRegistryPropertyW");
#else
	SetupDiGetClassDevsFunPtr =
		(SetupDiGetClassDevsFunType*)GetProcAddress(hLibrary, "SetupDiGetClassDevsA");
	SetupDiGetDeviceRegistryPropertyFunPtr = (SetupDiGetDeviceRegistryPropertyFunType*)
		GetProcAddress(hLibrary, "SetupDiGetDeviceRegistryPropertyA");
#endif
	SetupDiDestroyDeviceInfoListFunPtr = (SetupDiDestroyDeviceInfoListFunType*)
		GetProcAddress(hLibrary, "SetupDiDestroyDeviceInfoList");
	SetupDiEnumDeviceInfoFunPtr = (SetupDiEnumDeviceInfoFunType*)
		GetProcAddress(hLibrary, "SetupDiEnumDeviceInfo");
	guid = GUID_DEVINTERFACE_COMPORT;
	hDevInfoSet = SetupDiGetClassDevsFunPtr(&guid, NULL, NULL,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if(INVALID_HANDLE_VALUE == hDevInfoSet)
	{
		// Set the error to report
		_tprintf(TEXT("error lpfnSETUPDIGETCLASSDEVS, %d"), GetLastError());
		return FALSE;
	}/* end if */

	//bMoreItems = TRUE;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	i = 0;
	jj = 0;

	do
	{
		HKEY hDeviceKey;
		BOOL isFound;

		isFound = FALSE;
		bMoreItems = SetupDiEnumDeviceInfoFunPtr(hDevInfoSet, i, &devInfo);

		if(FALSE == bMoreItems)
			break;

		i++;

		hDeviceKey = SetupDiOpenDevRegKeyFunPtr(hDevInfoSet, &devInfo,
			DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);

		if(INVALID_HANDLE_VALUE != hDeviceKey)
		{
			int nPort;
			size_t nLen;
			LPTSTR pszPortName;

			nPort = 0;
			pszPortName = NULL;

			{
				// First query for the size of the registry value
				DWORD dwType;
				DWORD dwDataSize;
				LONG err;
				DWORD dwAllocatedSize;
				DWORD dwReturnedSize;
				dwType = 0;
				dwDataSize = 0;

				err = RegQueryValueEx(hDeviceKey, TEXT("PortName"), NULL,
					&dwType, NULL, &dwDataSize);

				if(ERROR_SUCCESS != err)
					continue;

				// Ensure the value is a string
				if(dwType != REG_SZ)
					continue;

				// Allocate enough bytes for the return value
				dwAllocatedSize = dwDataSize + sizeof(TCHAR);

				/* +sizeof(TCHAR) is to allow us to NULL terminate
					the data if it is not null terminated in the registry
				*/

				pszPortName = (LPTSTR)LocalAlloc(LMEM_FIXED, dwAllocatedSize);

				if(pszPortName == NULL)
					continue;

				// Recall RegQueryValueEx to return the data
				pszPortName[0] = _T('\0');
				dwReturnedSize = dwAllocatedSize;

				err = RegQueryValueEx(hDeviceKey, TEXT("PortName"), NULL,
					&dwType, (LPBYTE)pszPortName, &dwReturnedSize);

				if(ERROR_SUCCESS != err)
				{
					LocalFree(pszPortName);
					pszPortName = NULL;
					continue;
				}

				// Handle the case where the data just returned is the same size as the allocated size. This could occur where the data
				// has been updated in the registry with a non null terminator between the two calls to ReqQueryValueEx above. Rather than
				// return a potentially non-null terminated block of data, just fail the method call
				if(dwReturnedSize >= dwAllocatedSize)
					continue;

				// NULL terminate the data if it was not returned NULL terminated because it is not stored null terminated in the registry
				if(pszPortName[dwReturnedSize/sizeof(TCHAR) - 1] != _T('\0'))
					pszPortName[dwReturnedSize/sizeof(TCHAR)] = _T('\0');
			}/* end local variables */

			// If it looks like "COMX" then
			// add it to the array which will be returned
			nLen = _tcslen(pszPortName);

			if(3 < nLen)
			{
				if(0 == _tcsnicmp(pszPortName, TEXT("COM"), 3))
				{
					if(FALSE == isdigit(pszPortName[3]) )
						continue;

					// Work out the port number
					_tcsncpy(pPortNames + jj*(MAX_STR_LEN), pszPortName,
					_tcsnlen(pszPortName, (MAX_STR_LEN)));

					//_stprintf_s(&portName[jj][0], (MAX_STR_LEN), TEXT("%s"), pszPortName);
				}
				else
				{
					continue;
				}/* end if 0 == _tcsnicmp(pszPortName, TEXT("COM"), 3)*/
			}/* end if 3 < nLen*/

			LocalFree(pszPortName);
			isFound = TRUE;

			//Close the key now that we are finished with it
			RegCloseKey(hDeviceKey);
		}/*INVALID_HANDLE_VALUE != hDeviceKey*/

		if(FALSE == isFound)
			continue;

		//If the port was a serial port, then also try to get its friendly name
		{
			TCHAR szFriendlyName[1024];
			DWORD dwSize;
			DWORD dwType;
			szFriendlyName[0] = _T('\0');
			dwSize = sizeof(szFriendlyName);
			dwType = 0;

			if((TRUE == SetupDiGetDeviceRegistryPropertyFunPtr(hDevInfoSet, &devInfo,
				SPDRP_DEVICEDESC, &dwType, (PBYTE)(szFriendlyName),
				dwSize, &dwSize)) && (REG_SZ == dwType))
			{
				_tcsncpy(pFriendlyNames + jj*(MAX_STR_LEN), &szFriendlyName[0],
					_tcsnlen(&szFriendlyName[0], (MAX_STR_LEN)));
			}
			else
			{
				_stprintf_s(pFriendlyNames + jj*(MAX_STR_LEN), (MAX_STR_LEN), TEXT(""));
			}/* end if SetupDiGetDeviceRegistryPropertyFunPtr */
		}/* end local variable */

		jj++;
	} while(1);

	*pPortCount = jj;
	if(0 <jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT */




BOOL EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort(UINT* pPortCount, TCHAR* pPortNames, TCHAR* pFriendlyNames)
{
	UINT i, jj;
	INT ret;

	TCHAR* pTempPortNames;
	HMODULE hLibrary;
	TCHAR szFullPath[_MAX_PATH];

	GUID *pGuid;
	DWORD dwGuids;
	HDEVINFO hDevInfoSet;

	typedef HKEY (__stdcall SetupDiOpenDevRegKeyFunType)
		(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);

	typedef BOOL (__stdcall SetupDiClassGuidsFromNameFunType)
		(LPCTSTR, LPGUID, DWORD, PDWORD);

	typedef BOOL (__stdcall SetupDiDestroyDeviceInfoListFunType)
		(HDEVINFO);
	typedef BOOL (__stdcall SetupDiEnumDeviceInfoFunType)
		(HDEVINFO, DWORD, PSP_DEVINFO_DATA);

	typedef HDEVINFO (__stdcall SetupDiGetClassDevsFunType)
		(LPGUID, LPCTSTR, HWND, DWORD);

	typedef BOOL (__stdcall SetupDiGetDeviceRegistryPropertyFunType)
		(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	SetupDiOpenDevRegKeyFunType* SetupDiOpenDevRegKeyFunPtr;

	SetupDiClassGuidsFromNameFunType *SetupDiClassGuidsFromNameFunPtr;
	SetupDiGetClassDevsFunType *SetupDiGetClassDevsFunPtr;
	SetupDiGetDeviceRegistryPropertyFunType *SetupDiGetDeviceRegistryPropertyFunPtr;

	SetupDiDestroyDeviceInfoListFunType *SetupDiDestroyDeviceInfoListFunPtr;
	SetupDiEnumDeviceInfoFunType *SetupDiEnumDeviceInfoFunPtr;

	BOOL bMoreItems;
	SP_DEVINFO_DATA devInfo;

	ret = FALSE;
	jj = 0;
	szFullPath[0] = _T('\0');

	// Get the Windows System32 directory
	if(0 == GetSystemDirectory(szFullPath, _countof(szFullPath)))
	{
		_tprintf(TEXT("CEnumerateSerial::UsingSetupAPI1 failed, Error:%u\n"),GetLastError());
		return FALSE;
	}/* end if */

	// Setup the full path and delegate to LoadLibrary
#pragma warning(suppress: 6102) //There is a bug with the SAL annotation of GetSystemDirectory in the Windows 8.1 SDK
	_tcscat_s(szFullPath, _countof(szFullPath), _T("\\"));
	_tcscat_s(szFullPath, _countof(szFullPath), TEXT("SETUPAPI.DLL"));
	hLibrary = LoadLibrary(szFullPath);

	SetupDiOpenDevRegKeyFunPtr =
		(SetupDiOpenDevRegKeyFunType*)GetProcAddress(hLibrary, "SetupDiOpenDevRegKey");

#if defined _UNICODE
	SetupDiClassGuidsFromNameFunPtr = (SetupDiClassGuidsFromNameFunType*)
		GetProcAddress(hLibrary, "SetupDiGetDeviceRegistryPropertyW");
	SetupDiGetClassDevsFunPtr = (SetupDiGetClassDevsFunType*)
		GetProcAddress(hLibrary, "SetupDiGetClassDevsW");
	SetupDiGetDeviceRegistryPropertyFunPtr = (SetupDiGetDeviceRegistryPropertyFunType*)
		GetProcAddress(hLibrary, "SetupDiGetDeviceRegistryPropertyW");
#else
	SetupDiClassGuidsFromNameFunPtr = (SetupDiClassGuidsFromNameFunType*)
		GetProcAddress(hLibrary, "SetupDiClassGuidsFromNameA");
	SetupDiGetClassDevsFunPtr = (SetupDiGetClassDevsFunType*)
		GetProcAddress(hLibrary, "SetupDiGetClassDevsA");
	SetupDiGetDeviceRegistryPropertyFunPtr = (SetupDiGetDeviceRegistryPropertyFunType*)
		GetProcAddress(hLibrary, "SetupDiGetDeviceRegistryPropertyA");
#endif
	SetupDiDestroyDeviceInfoListFunPtr = (SetupDiDestroyDeviceInfoListFunType*)
		GetProcAddress(hLibrary, "SetupDiDestroyDeviceInfoList");
	SetupDiEnumDeviceInfoFunPtr = (SetupDiEnumDeviceInfoFunType*)
		GetProcAddress(hLibrary, "SetupDiEnumDeviceInfo");

	// First need to convert the name "Ports" to a GUID using SetupDiClassGuidsFromName
	dwGuids = 0;
	SetupDiClassGuidsFromNameFunPtr(TEXT("Ports"), NULL, 0, &dwGuids);

	if(0 == dwGuids)
		return FALSE;

	// Allocate the needed memory
	pGuid = (GUID*)HeapAlloc(GetProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS, dwGuids * sizeof(GUID));

	if(NULL == pGuid)
		return FALSE;

	// Call the function again
	if(FALSE == SetupDiClassGuidsFromNameFunPtr(TEXT("Ports"),
			pGuid, dwGuids, &dwGuids))
	{
		return FALSE;
	}/* end if */

	hDevInfoSet = SetupDiGetClassDevsFunPtr(pGuid, NULL, NULL, DIGCF_PRESENT /*| DIGCF_DEVICEINTERFACE*/);

	if(INVALID_HANDLE_VALUE == hDevInfoSet)
	{
		// Set the error to report
		_tprintf(TEXT("error SetupDiGetClassDevsFunPtr, %d"), GetLastError());
		return FALSE;
	}/* if */

	//bMoreItems = TRUE;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
	i = 0;
	jj = 0;

	do
	{
		HKEY hDeviceKey;
		BOOL isFound;

		isFound = FALSE;
		bMoreItems = SetupDiEnumDeviceInfoFunPtr(hDevInfoSet, i, &devInfo);
		if(FALSE == bMoreItems)
			break;

		i++;

		hDeviceKey = SetupDiOpenDevRegKeyFunPtr(hDevInfoSet, &devInfo,
			DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);

		if(INVALID_HANDLE_VALUE != hDeviceKey)
		{
			int nPort;
			size_t nLen;
			LPTSTR pszPortName;

			nPort = 0;
			pszPortName = NULL;

			{
				// First query for the size of the registry value
				DWORD dwType;
				DWORD dwDataSize;
				LONG err;
				DWORD dwAllocatedSize;
				DWORD dwReturnedSize;
				dwType = 0;
				dwDataSize = 0;

				err = RegQueryValueEx(hDeviceKey, TEXT("PortName"), NULL,
					&dwType, NULL, &dwDataSize);

				if(ERROR_SUCCESS != err)
					continue;

				// Ensure the value is a string
				if(dwType != REG_SZ)
					continue;

				// Allocate enough bytes for the return value
				dwAllocatedSize = dwDataSize + sizeof(TCHAR);

				/* +sizeof(TCHAR) is to allow us to NULL terminate
					the data if it is not null terminated in the registry
				*/

				pszPortName = (LPTSTR)LocalAlloc(LMEM_FIXED, dwAllocatedSize);

				if(pszPortName == NULL)
					continue;

				//Recall RegQueryValueEx to return the data
				pszPortName[0] = TEXT('\0');
				dwReturnedSize = dwAllocatedSize;

				err = RegQueryValueEx(hDeviceKey, TEXT("PortName"), NULL,
					&dwType, (LPBYTE)pszPortName, &dwReturnedSize);

				if(ERROR_SUCCESS != err)
				{
					LocalFree(pszPortName);
					pszPortName = NULL;
					continue;
				}

				// Handle the case where the data just returned is the same size as the allocated size. This could occur where the data
				// has been updated in the registry with a non null terminator between the two calls to ReqQueryValueEx above. Rather than
				// return a potentially non-null terminated block of data, just fail the method call
				if(dwReturnedSize >= dwAllocatedSize)
					continue;

				// NULL terminate the data if it was not returned NULL terminated because it is not stored null terminated in the registry
				if(pszPortName[dwReturnedSize/sizeof(TCHAR) - 1] != _T('\0'))
					pszPortName[dwReturnedSize/sizeof(TCHAR)] = _T('\0');
			}/* end local variable */

			// If it looks like "COMX" then
			// add it to the array which will be returned
			nLen = _tcslen(pszPortName);

			if(3 < nLen)
			{
				if(0 == _tcsnicmp(pszPortName, TEXT("COM"), 3))
				{
					if(FALSE == isdigit(pszPortName[3]))
						continue;

					//Work out the port number
					_tcsncpy(pPortNames + jj*(MAX_STR_LEN), pszPortName,
					_tcsnlen(pszPortName, (MAX_STR_LEN)));

					//_stprintf_s(&portName[jj][0], (MAX_STR_LEN), TEXT("%s"), pszPortName);
				}
				else
				{
					continue;
				}/* end if 0 == _tcsnicmp(pszPortName, TEXT("COM"), 3) */
			}/* end if 3 < nLen */

			LocalFree(pszPortName);
			isFound = TRUE;

			// Close the key now that we are finished with it
			RegCloseKey(hDeviceKey);
		}/* end INVALID_HANDLE_VALUE != hDeviceKey */

		if(FALSE == isFound)
			continue;

		// If the port was a serial port, then also try to get its friendly name
		{
			TCHAR szFriendlyName[1024];
			DWORD dwSize;
			DWORD dwType;
			szFriendlyName[0] = _T('\0');
			dwSize = sizeof(szFriendlyName);
			dwType = 0;

			if((TRUE == SetupDiGetDeviceRegistryPropertyFunPtr(hDevInfoSet, &devInfo,
				SPDRP_DEVICEDESC, &dwType, (PBYTE)(szFriendlyName),
				dwSize, &dwSize)) && (REG_SZ == dwType))
			{
				_tcsncpy(pFriendlyNames + jj*(MAX_STR_LEN), &szFriendlyName[0],
					_tcsnlen(&szFriendlyName[0], (MAX_STR_LEN)));
			}
			else
			{
				_stprintf_s(pFriendlyNames + jj*(MAX_STR_LEN), (MAX_STR_LEN), TEXT(""));
			}/* end if SetupDiGetDeviceRegistryPropertyFunPtr */
		}/* end local variable */

		jj++;
	} while(1);

	HeapFree(GetProcessHeap(), 0, pGuid);

	*pPortCount = jj;

	if(0 <jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort */




BOOL EnumerateComPortRegistry(UINT* pPortCount, TCHAR* pPortNames)
{
	// What will be the return value from this function (assume the worst)
	UINT jj;
	BOOL ret;
	HKEY hSERIALCOMM;

	jj = 0;
	ret = FALSE;

	if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_QUERY_VALUE, &hSERIALCOMM))
	{
		//Get the max value name and max value lengths
		DWORD dwMaxValueNameLen;
		DWORD dwMaxValueLen;
		DWORD dwQueryInfo;

		dwQueryInfo = RegQueryInfoKey(hSERIALCOMM, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL,
		&dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);

		if(ERROR_SUCCESS == dwQueryInfo)
		{
			DWORD dwMaxValueNameSizeInChars, dwMaxValueNameSizeInBytes,
				dwMaxValueDataSizeInChars, dwMaxValueDataSizeInBytes;

			GUID* pValueName;
			GUID* pValueData;

			dwMaxValueNameSizeInChars = dwMaxValueNameLen + 1;// Include space for the NULL terminator
			dwMaxValueNameSizeInBytes = dwMaxValueNameSizeInChars * sizeof(TCHAR);
			dwMaxValueDataSizeInChars = dwMaxValueLen/sizeof(TCHAR) + 1;//Include space for the NULL terminator
			dwMaxValueDataSizeInBytes = dwMaxValueDataSizeInChars * sizeof(TCHAR);

			// Allocate some space for the value name and value data
			pValueName = (GUID*)HeapAlloc(GetProcessHeap(),
				HEAP_GENERATE_EXCEPTIONS| HEAP_ZERO_MEMORY, dwMaxValueNameSizeInBytes);
			pValueData = (GUID*)HeapAlloc(GetProcessHeap(),
				HEAP_GENERATE_EXCEPTIONS| HEAP_ZERO_MEMORY, dwMaxValueDataSizeInBytes);

			if(NULL != pValueName && NULL != pValueData)
			{
				// Enumerate all the values underneath HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
				DWORD i;
				DWORD dwType;
				DWORD dwValueNameSize;
				DWORD dwDataSize;
				LONG nEnum;

				dwValueNameSize = dwMaxValueNameSizeInChars;
				dwDataSize = dwMaxValueDataSizeInBytes;

				i = 0;

				nEnum = RegEnumValue(hSERIALCOMM, i,
					(LPTSTR)pValueName, &dwValueNameSize, NULL, &dwType,
					(LPBYTE)pValueData, &dwDataSize);

				jj = 0;
				while(ERROR_SUCCESS == nEnum)
				{
					// If the value is of the correct type, then add it to the array
					if(REG_SZ == dwType)
					{
						_stprintf_s(pPortNames + jj*(MAX_STR_LEN),
							(MAX_STR_LEN), TEXT("%s"), pValueData);
						jj++;
					}/* end if */

					// Prepare for the next time around
					dwValueNameSize = dwMaxValueNameSizeInChars;
					dwDataSize = dwMaxValueDataSizeInBytes;
					ZeroMemory(pValueName, dwMaxValueNameSizeInBytes);
					ZeroMemory(pValueData, dwMaxValueDataSizeInBytes);
					i++;
					nEnum = RegEnumValue(hSERIALCOMM, i,
						(LPTSTR)pValueName, &dwValueNameSize, NULL, &dwType,
						(LPBYTE)pValueData, &dwDataSize);
				}/* end while*/
			}
			else
			{
				return FALSE;
			}/* end if NULL != pValueName && NULL != pValueData */

			HeapFree(GetProcessHeap(), 0, pValueName);
			HeapFree(GetProcessHeap(), 0, pValueData);
		}/* end ERROR_SUCCESS == dwQueryInfo */

		// Close the registry key now that we are finished with it
		RegCloseKey(hSERIALCOMM);

		if(dwQueryInfo != ERROR_SUCCESS)
			return FALSE;
	}/* end ERROR_SUCCESS == RegOpenKeyEx */

	*pPortCount = jj;

	if(0 <jj)
		ret = TRUE;

	return ret;
}/* end EnumerateComPortRegistry*/



/* Using timeGetTime() from winmm.lib */

#define TIMER_BEGIN(TIMER_LABEL) \
			{ unsigned int tBegin##TIMER_LABEL, tEnd##TIMER_LABEL; \
			tBegin##TIMER_LABEL = (unsigned int) timeGetTime();

#define TIMER_END(TIMER_LABEL)  \
			tEnd##TIMER_LABEL = (unsigned int) timeGetTime();\
			fprintf(stderr, "%s cost time = %d ms\n", \
			#TIMER_LABEL, tEnd##TIMER_LABEL - tBegin##TIMER_LABEL); \
			}

void BenchmarkEnumComPorts(void)
{
	UINT i;
	UINT n;
	char* nativeLocale;
	nativeLocale = _strdup( setlocale(LC_CTYPE,NULL) );
	TCHAR portNames[MAX_PORT_NUM][MAX_STR_LEN];
	TCHAR friendlyNames[MAX_PORT_NUM][MAX_STR_LEN];


	_tprintf(TEXT("\nCreateFile method : \n"));
	for(i = 0; i< MAX_PORT_NUM; i++)
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
	TIMER_BEGIN(EnumerateComPortByCreateFile);
	EnumerateComPortByCreateFile(&n, &portNames[0][0]);
	TIMER_END(EnumerateComPortByCreateFile);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	for(i = 0; i< n; i++)
		_tprintf(TEXT("\t%s\n"), &portNames[i][0]);


	_tprintf(TEXT("\nQueryDosDevice method : "));
	for(i = 0; i< MAX_PORT_NUM; i++)
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
	TIMER_BEGIN(EnumerateComPortQueryDosDevice);
	EnumerateComPortQueryDosDevice(&n, &portNames[0][0]);
	TIMER_END(EnumerateComPortQueryDosDevice);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	for(i = 0; i< n; i++)
		_tprintf("\t%s\n", &portNames[i][0]);


	_tprintf(TEXT("\nGetDefaultCommConfig method : \n"));
	for(i = 0; i< MAX_PORT_NUM; i++)
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
	TIMER_BEGIN(EnumerateComPortByGetDefaultCommConfig);
	EnumerateComPortByGetDefaultCommConfig(&n, &portNames[0][0]);
	TIMER_END(EnumerateComPortByGetDefaultCommConfig);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	for(i = 0; i< n; i++)
		_tprintf(TEXT("\t%s\n"), &portNames[i][0]);


	_tprintf(TEXT("\nSetupAPI GUID_DEVINTERFACE_COMPORT method : \n"));
	for(i = 0; i< MAX_PORT_NUM; i++)
	{
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
		ZeroMemory(&friendlyNames[i][0], MAX_STR_LEN);
	}
	TIMER_BEGIN(EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT);
	EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT(&n, &portNames[0][0], &friendlyNames[0][0]);
	TIMER_END(EnumerateComPortSetupAPI_GUID_DEVINTERFACE_COMPORT);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	setlocale(LC_CTYPE, "");
	for(i = 0; i< n; i++)
		_tprintf(TEXT("\t%s - %s \n"), &portNames[i][0], &friendlyNames[i][0]);
	setlocale(LC_CTYPE, nativeLocale);


	_tprintf(TEXT("\nSetupAPI SetupDiClassGuidsFromNamePort method : \n"));
	for(i = 0; i< MAX_PORT_NUM; i++)
	{
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
		ZeroMemory(&friendlyNames[i][0], MAX_STR_LEN);
	}
	TIMER_BEGIN(EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort);
	EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort(&n, &portNames[0][0], &friendlyNames[0][0]);
	TIMER_END(EnumerateComPortSetupAPISetupDiClassGuidsFromNamePort);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	setlocale(LC_CTYPE, "");
	for(i = 0; i< n; i++)
		_tprintf(TEXT("\t%s - %s \n"), &portNames[i][0], &friendlyNames[i][0]);
	setlocale(LC_CTYPE, nativeLocale);


	_tprintf(TEXT("\nRegistry method : \n"));
	for(i = 0; i< MAX_PORT_NUM; i++)
		ZeroMemory(&portNames[i][0], MAX_STR_LEN);
	TIMER_BEGIN(EnumerateComPortRegistry);
	EnumerateComPortRegistry(&n, &portNames[0][0]);
	TIMER_END(EnumerateComPortRegistry);
	_tprintf(TEXT("Found %d Comm Ports:\n"), n);
	for(i = 0; i< n; i++)
		_tprintf(TEXT("\t%s\n"), &portNames[i][0]);
 
}/* main */


