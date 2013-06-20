#include "my12doomRenderer.h"

HRESULT ui_drawer_base::init(int width, int height,IDirect3DDevice9 *device)
{
	HRESULT hr = S_OK;

	if (!(m_init_state & CPU_INITED))
		hr = init_cpu(device);
	if (FAILED(hr))
		return hr;
	m_init_state |= CPU_INITED;

// 	if (!(m_init_state & GPU_INITED))
		hr = init_gpu(width, height, device);
	if (FAILED(hr))
		return hr;
	m_init_state |= GPU_INITED;

	return hr;
}
HRESULT ui_drawer_base::uninit()
{
	
	HRESULT hr = S_OK;
	if (m_init_state & GPU_INITED)
		hr = invalidate_gpu();
	if (FAILED(hr))
		return hr;
	m_init_state &= ~GPU_INITED;

	if (m_init_state & CPU_INITED)
		hr = invalidate_cpu();
	if (FAILED(hr))
		return hr;
	m_init_state &= ~CPU_INITED;

	return hr;
}
