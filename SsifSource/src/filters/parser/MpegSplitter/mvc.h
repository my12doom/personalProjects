#pragma once
#include <dshow.h>

// {D647E0AB-E9BB-401a-8B37-6E64C8B798CC}
DEFINE_GUID(IID_IMVC, 
0xd647e0ab, 0xe9bb, 0x401a, 0x8b, 0x37, 0x6e, 0x64, 0xc8, 0xb7, 0x98, 0xcc);

class DECLSPEC_UUID("D647E0AB-E9BB-401a-8B37-6E64C8B798CC") IMVC : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE IsMVC() = 0;
};