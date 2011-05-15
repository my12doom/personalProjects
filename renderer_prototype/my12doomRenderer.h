#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>

struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_my12doomRenderer;

// this renderer must have a valid first window, if not, connection will fail.
// setting window to invalid window during 
// you can enter dual projector mode without a second window, but you won't get second image until you set a second window

enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, dual_window, out_side_by_side, out_top_bottom, output_mode_types_max
};

enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max
};

enum mask_mode_types
{
	row_interlace, line_interlace, checkboard_interlace, mask_mode_types_max
};

class my12doomRenderer : public CBaseVideoRenderer
{
public:
	my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 = NULL, HWND hwnd2 = NULL);
	~my12doomRenderer();

	HRESULT handle_reset();

	HRESULT set_input_layout(int layout);
	HRESULT set_output_mode(int mode);
	HRESULT set_mask_mode(int mode);
	HRESULT set_mask_color(int id, DWORD color);
	HRESULT set_swap_eyes(bool swap);
	HRESULT set_fullscreen(bool full);
	HRESULT set_offset(int dimention, double offset);		// dimention1 = x, dimention2 = y
	HRESULT set_aspect(double aspect);
	HRESULT set_window(HWND wnd, HWND wnd2);


	DWORD get_mask_color(int id);
	bool get_swap_eyes();
	input_layout_types get_input_layout();
	output_mode_types get_output_mode();
	mask_mode_types get_mask_mode();
	bool get_fullscreen();
	double get_offset(int dimention);
	double get_aspect();

protected:
	// dshow functions
	HRESULT CheckMediaType(const CMediaType *pmt );
	HRESULT SetMediaType(const CMediaType *pmt );
	HRESULT DoRenderSample(IMediaSample *pMediaSample);
	HRESULT	BreakConnect();
	HRESULT CompleteConnect(IPin *pRecievePin);

	// dshow variables
	CCritSec m_data_lock;
	BYTE *m_data;
	bool m_data_changed;
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height
	double offset_x /*= -0.0*/;
	double offset_y /*= 0.0*/;
	double source_aspect /*= (double)m_lVidWidth / m_lVidHeight*/;


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
	HRESULT set_device_state(device_state new_state);

	// variables
	struct MyVertex
	{
		float x , y, z;
		float w;
		DWORD diffuse;
		DWORD specular;
		float tu, tv;
	} g_myVertices[32];
	bool g_swapeyes;
	output_mode_types output_mode;
	input_layout_types input_layout;
	mask_mode_types mask_mode;
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
};