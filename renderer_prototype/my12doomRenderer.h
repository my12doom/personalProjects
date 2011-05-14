#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>

struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_my12doomRenderer;

// this renderer must have a valid first window, if not, connection will fail.
// setting window to invalid window during 
// you can enter dual projector mode without a second window, but you won't get second image until you set a second window

class my12doomRenderer : public CBaseVideoRenderer
{
public:
	my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 = NULL, HWND hwnd2 = NULL);
	~my12doomRenderer();

	OAFilterState State(){return m_State;}
protected:
	// dshow functions
	HRESULT CheckMediaType(const CMediaType *pmt );
	HRESULT SetMediaType(const CMediaType *pmt );
	HRESULT DoRenderSample(IMediaSample *pMediaSample);
	HRESULT	BreakConnect();
	HRESULT CompleteConnect(IPin *pRecievePin);

	// dx9 functions and variables
	enum device_state
	{
		fine,							// device is fine
		need_reset_object,				// objects size changed, should recreate objects
		need_reset,						// reset requested by program, usually to change back buffer size, but program can continue rendering without reset
		device_lost,					// device lost, can't continue
		need_create,					// device not created, or need to recreate, can't continue
		create_failed,					// 
		device_state_max,				// not used
	} m_device_state;					// need_create

	HRESULT handle_device_state();							//handle device create/recreate/lost/reset
	HRESULT shutdown();
	HRESULT create_render_thread();
	HRESULT terminate_render_thread();
	HRESULT invalidate_objects();
	HRESULT restore_objects();
	HRESULT render_nolock(bool forced = false);
	HRESULT render(bool forced = false);
	static DWORD WINAPI render_thread(LPVOID param);

	// dx9 helper functions
	HRESULT load_image(bool forced = false);
	HRESULT calculate_vertex();
	HRESULT generate_mask();
	HRESULT set_fullscreen(bool full);
	HRESULT set_device_state(device_state new_state);

	// variables
	HWND g_hWnd;
	HWND g_hWnd2;
	CComPtr<IDirect3D9>		g_pD3D;
	CComPtr<IDirect3DDevice9> g_pd3dDevice;
	CComPtr<IDirect3DSwapChain9> g_swap1;
	CComPtr<IDirect3DSwapChain9> g_swap2;
	D3DPRESENT_PARAMETERS   g_new_pp;
	D3DPRESENT_PARAMETERS   g_active_pp;
	D3DPRESENT_PARAMETERS   g_active_pp2;
	CCritSec m_frame_lock;
	CCritSec m_device_lock;
	int g_temp_width;
	int g_temp_height;
	int m_device_threadid;

	CComPtr<IDirect3DVertexBuffer9> g_VertexBuffer;
	CComPtr <IDirect3DPixelShader9> g_ps_yv12_to_rgb;

	CComPtr<IDirect3DTexture9> g_tex_Y;						// source texture x3, 
	CComPtr<IDirect3DTexture9> g_tex_U;
	CComPtr<IDirect3DTexture9> g_tex_V;
	CComPtr<IDirect3DTexture9> g_tex_rgb_left;				// source texture, converted to RGB32
	CComPtr<IDirect3DTexture9> g_tex_rgb_right;
	CComPtr<IDirect3DTexture9> g_tex_mask;					// mask txture
	CComPtr<IDirect3DTexture9> g_mask_temp_left;			// two temp texture, you may need it in some case
	CComPtr<IDirect3DTexture9> g_mask_temp_right;

	CComPtr<IDirect3DSurface9> g_sbs_surface;				// nv3d temp surface

	// render thread variables
	HANDLE m_render_thread;
	bool m_render_thread_exit;
	DWORD m_color1;
	DWORD m_color2;
	LONG g_style, g_exstyle;
	RECT g_window_pos;

public:
	HRESULT handle_reset();
	CCritSec m_data_lock;
	BYTE *m_data;
	bool m_data_changed;
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height
};