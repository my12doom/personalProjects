#include "private_filter.h"
#include <atlbase.h>

typedef HRESULT (__stdcall *pDllGetClassObject) (REFCLSID rclsid, REFIID riid, LPVOID*ppv);

void GetAppPath(wchar_t *out, int size = MAX_PATH)
{
	GetModuleFileNameW(NULL, out, size);
	for(int i = wcslen(out); i>0; i--)
	{
		if(out[i] == L'\\')
		{
			out[i+1] = NULL;
			break;
		}
	}
}

HRESULT GetFileSource(const wchar_t *filename, CLSID *out)
{
	wchar_t ini[MAX_PATH];
	GetAppPath(ini);
	wcscat(ini, L"dwindow.ini");

	wchar_t ext[MAX_PATH];
	for(int i=wcslen(filename); i>0; i--)
		if(filename[i] == L'.')
		{
			wcscpy(ext, filename+i+1);
			break;
		}

	wchar_t tmp[MAX_PATH];
	GetPrivateProfileStringW(L"Extensions", ext, L"", tmp, MAX_PATH, ini);

	if (tmp[0] == NULL)
		return E_FAIL;
	else
		return CLSIDFromString(tmp, out);
}

HRESULT myCreateInstance(CLSID clsid, IID iid, void**out)
{
	LPOLESTR strCLSID = NULL;
	wchar_t filename[MAX_PATH];
	wchar_t tmp[MAX_PATH];

	StringFromCLSID(clsid, &strCLSID);
	GetAppPath(tmp);
	wcscat(tmp, L"dwindow.ini");
	GetPrivateProfileStringW(L"modules", strCLSID, L"", filename, MAX_PATH, tmp);
	if (strCLSID) CoTaskMemFree(strCLSID);

	// not found in private files
	if (filename[0] == NULL)
		return CoCreateInstance(clsid, NULL, CLSCTX_INPROC, iid, out);

	// load from private filters
	GetAppPath(tmp);
	wcscat(tmp, filename);
	
	HINSTANCE hDll = CoLoadLibrary(tmp, TRUE);
	if (!hDll)
		return E_FAIL;
	pDllGetClassObject pGetClass = (pDllGetClassObject)GetProcAddress(hDll, "DllGetClassObject");
	if (!pGetClass)
		return E_FAIL;

	CComPtr<IClassFactory> factory;
	pGetClass(clsid, IID_IClassFactory, (void**)&factory);
	if (factory)
	{
		return factory->CreateInstance(NULL, iid, (void**) out);
	}
	else
	{
		return E_FAIL;
	}
}