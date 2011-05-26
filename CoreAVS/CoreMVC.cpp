#include <atlbase.h>
#include <DShow.h>
#include "CoreMVC.h"

HRESULT write_property(IPropertyBag *bag, const wchar_t *property_to_write)
{
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = NULL;
	if (property_to_write)
		var.bstrVal =  SysAllocString(property_to_write);

	HRESULT hr = bag->Write(L"Settings", &var);
	if (property_to_write)
		SysFreeString(var.bstrVal);

	return hr;
}

HRESULT ActiveCoreMVC(IBaseFilter *decoder)
{
	CComQIPtr<IPropertyBag, &IID_IPropertyBag> pbag(decoder);
	if (pbag)
	{
		write_property(pbag, L"use_tray=0");
		return write_property(pbag, L"app_mode=1");
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT CreateCoreMVC(IBaseFilter **out)
{
	HINSTANCE hDll = CoLoadLibrary(L"CoreAVCDecoder.dll", TRUE);
	if (!hDll)
		return E_FAIL;
	pDllGetClassObject pGetClass = (pDllGetClassObject)GetProcAddress(hDll, "DllGetClassObject");
	if (!pGetClass)
		return E_FAIL;

	CComPtr<IClassFactory> factory;
	pGetClass(CLSID_CoreAVC, IID_IClassFactory, (void**)&factory);
	if (factory)
	{
		HRESULT hr = factory->CreateInstance(NULL, IID_IBaseFilter, (void**) out);
		if (FAILED(hr))
			return hr;
		else
			return ActiveCoreMVC(*out);
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT Create_my12doomSource(IBaseFilter **out)
{
	HINSTANCE hDll = CoLoadLibrary(L"my12doomSource.ax", TRUE);
	if (!hDll)
		return E_FAIL;
	pDllGetClassObject pGetClass = (pDllGetClassObject)GetProcAddress(hDll, "DllGetClassObject");
	if (!pGetClass)
		return E_FAIL;

	CComPtr<IClassFactory> factory;
	pGetClass(CLSID_3dtvMPEGSource, IID_IClassFactory, (void**)&factory);
	if (factory)
	{
		return factory->CreateInstance(NULL, IID_IBaseFilter, (void**) out);
	}
	else
	{
		return E_FAIL;
	}
}