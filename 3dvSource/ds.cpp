#include <tchar.h>
#include <Windows.h>
#include <dshow.h>
#include "my12doomSource.h"

#pragma warning(disable:4710)  // 'function': function not inlined (optimzation)

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
	L"Output",              // Pin string name
	FALSE,                  // Is it rendered
	TRUE,                   // Is it an output
	FALSE,                  // Can we have none
	FALSE,                  // Can we have many
	&CLSID_NULL,            // Connects to filter
	NULL,                   // Connects to pin
	1,                      // Number of types
	&sudOpPinTypes			// Pin details
};       

const AMOVIESETUP_FILTER sudBallax =
{
	&CLSID_my12doomSource,    // Filter CLSID
	L"my12doom's 3dv source",       // String name
	MERIT_DO_NOT_USE,       // Filter merit
	1,                      // Number pins
	&sudOpPin,               // Pin details
	CLSID_LegacyAmFilterCategory
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
	{ L"my12doom's 3dv source"
	, &CLSID_my12doomSource
	, my12doomSource::CreateInstance
	, NULL
	, &sudBallax }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
	bool bOK = false;

	TCHAR szKey[512];
	_tcscpy(szKey, pszKey);
	if(pszSubkey != 0)
	{
		_tcscat(szKey, L"\\");
		_tcscat(szKey, pszSubkey);
	}

	HKEY hKey;
	LONG ec = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if(ec == ERROR_SUCCESS)
	{
		if(pszValue != 0)
		{
			ec = ::RegSetValueEx(hKey, pszValueName, 0, REG_SZ,
				reinterpret_cast<BYTE*>(const_cast<LPTSTR>(pszValue)),
				(_tcslen(pszValue) + 1) * sizeof(TCHAR));
		}

		bOK = (ec == ERROR_SUCCESS);

		::RegCloseKey(hKey);
	}

	return bOK;
}

bool SetRegKeyValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValue)
{
	return SetRegKeyValue(pszKey, pszSubkey, 0, pszValue);
}
STDAPI DllRegisterServer()
{
	//SetRegKeyValue(_T("Media Type\\Extensions\\"), _T(".3dv"), _T("Source Filter"), _T("{FBCFD6AF-B25F-4a6d-AE56-5B5303F1A40E}"));

	return AMovieDllRegisterServer2(TRUE);

} // DllRegisterServer

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);

}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}