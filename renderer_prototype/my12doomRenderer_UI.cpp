#include "..\dwindow\resource.h"
#include "my12doomRenderer.h"

HRESULT ui_drawer_base::init(int width, int height,IDirect3DDevice9 *device)
{
	HRESULT hr = init_cpu(width, height, device);
	if (FAILED(hr))
		return hr;
	return init_gpu(width, height, device);
}
HRESULT ui_drawer_base::uninit()
{
	HRESULT hr = invalidate_gpu();
	if (FAILED(hr))
		return hr;
	return invalidate_cpu();
}
