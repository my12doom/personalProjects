#include <windows.h>

HRESULT myCreateInstance(CLSID clsid, IID iid, void**out);
HRESULT GetFileSource(const wchar_t *filename, CLSID *out);