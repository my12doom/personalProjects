#include "BaseUI.h"
#include <atlbase.h>
#include <d3d9.h>
// #include "CCritSec.h"

#pragma comment(lib, "d3d9.lib")

// a simple D3D9 engine
// no device lost handling

class SampleEngine : public D3D9UIEngine
{
public:
	SampleEngine(HWND hwnd);
	~SampleEngine();

	HRESULT HandleDeviceState();
	friend class SampleObject;

protected:
	void init_d3d9(void);
	void render(void);
	HWND g_hWnd;
	CComPtr<IDirect3D9> m_D3D;
	CComPtr<IDirect3DDevice9> m_device;
	CComPtr<IDirect3DSwapChain9> m_swap;
	CComPtr<IDirect3DPixelShader9> m_lanczos;
	CComPtr<IDirect3DSurface9> m_hittest_cpu;
	CComPtr<IDirect3DSurface9> m_hittest_gpu;
	D3DPRESENT_PARAMETERS   m_pp;
	int m_last_hittest_result;

	HANDLE m_rendering_thread;
	bool m_exit;
	static DWORD WINAPI rendering_thread(LPVOID param);


	// D3D9UIEngine interface
	virtual HRESULT LoadResource(ID3D9UIResource **out, void *data, int type);
	virtual HRESULT Draw(ID3D9UIResource *resource, RECTF *src, RECTF *dst, int hittest_id);
	virtual HRESULT Draw(ID3D9UIResource *resource, RECT *src, RECT *dst, int hittest_id);
	virtual HRESULT DrawText(ID3D9UIResource *resource, const wchar_t *text, RECT *rect, DWORD format, DWORD color);
	virtual HRESULT ReleaseResource(ID3D9UIResource *res);
	virtual HRESULT Hittest(int x, int y, ID3D9UIObject **hit);
	virtual HRESULT Hittest(ID3D9UIObject **hit);					// for performance reason, this function use last render time's mouse position for hittesting.
	virtual HRESULT RenderNow();

};

ID3D9UIEngine *create_engine(HWND hwnd);
