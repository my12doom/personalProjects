#include "private_filter.h"
#include "ImageSource.h"
#include <atlbase.h>
#include "MediaInfo.h"
#include "global_funcs.h"

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
	luaState L;
	lua_getglobal(L, "dshow");
	lua_getfield(L, -1, "decide_splitter");
	lua_pushstring(L, W2UTF8(filename));

	lua_mypcall(L, 1, 1, 0);

	if (lua_type(L, -1) == LUA_TSTRING)
		return CLSIDFromString(UTF82W(lua_tostring(L, -1)), out);

	return E_FAIL;
}

#include "IntelWiDiExtensions_i.h"

HRESULT myCreateInstance(CLSID clsid, IID iid, void**out)
{
	DWORD os = LOBYTE(LOWORD(GetVersion()));
	if (iid == __uuidof(IWiDiExtensions) && os < 6)
		return E_FAIL;

	if (clsid == CLSID_my12doomImageSource )
	{
		HRESULT hr;
		my12doomImageSource *s = (my12doomImageSource *)my12doomImageSource::CreateInstance(NULL, &hr);

		if (FAILED(hr))
			return hr;

		return s->QueryInterface(iid, out);
	}

	if (clsid == CLSID_MXFReader)
		config_MXFReader();

	if (clsid == CLSID_AVSource)
		make_av_splitter_support_my_formats();


	LPOLESTR strCLSID = NULL;
	wchar_t filename[MAX_PATH] = {0};
	wchar_t tmp[MAX_PATH];

	StringFromCLSID(clsid, &strCLSID);
	GetAppPath(tmp);
	wcscat(tmp, L"dwindow.ini");
	luaState L;
	lua_getglobal(L, "dshow");
	lua_getfield(L, -1, "clsid2module");
	lua_pushstring(L, W2UTF8(strCLSID));
	lua_mypcall(L, 1, 1, 0);
	if (lua_type(L, -1) == LUA_TSTRING)
		wcscpy(filename, UTF82W(lua_tostring(L, -1)));
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