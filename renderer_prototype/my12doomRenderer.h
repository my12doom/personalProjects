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


class Imy12doomRendererCallback
{
public:
	virtual HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn) = 0;
};


class DRendererInputPin : public CRendererInputPin
{
public:
	DRendererInputPin(__inout CBaseRenderer *pRenderer,	__inout HRESULT *phr, __in_opt LPCWSTR Name) : CRendererInputPin(pRenderer, phr, Name){}
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};

class DBaseVideoRenderer: public CBaseVideoRenderer
{
public:
	DBaseVideoRenderer(REFCLSID clsid,LPCTSTR name , LPUNKNOWN pUnk,HRESULT *phr)
		: CBaseVideoRenderer(clsid, name, pUnk, phr){};
	virtual CBasePin * GetPin(int n);
protected:
	friend class DRendererInputPin;
	REFERENCE_TIME m_thisstream;
};

class my12doomRenderer : public DBaseVideoRenderer
{
public:
	my12doomRenderer(LPUNKNOWN pUnk,HRESULT *phr, HWND hwnd1 = NULL, HWND hwnd2 = NULL);
	~my12doomRenderer();

	HRESULT pump();
	HRESULT repaint_video();

	// set
	HRESULT set_input_layout(int layout);
	HRESULT set_output_mode(int mode);
	HRESULT set_mask_mode(int mode);
	HRESULT set_mask_color(int id, DWORD color);
	HRESULT set_swap_eyes(bool swap);
	HRESULT set_fullscreen(bool full);
	HRESULT set_offset(int dimention, double offset);		// dimention1 = x, dimention2 = y
	HRESULT set_aspect(double aspect);
	HRESULT set_window(HWND wnd, HWND wnd2);
	HRESULT set_bmp(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop);
	HRESULT set_ui(void* data, int pitch);
	HRESULT set_ui_visible(bool visible);
	HRESULT set_callback(Imy12doomRendererCallback *cb){m_cb = cb; return S_OK;}
	bool m_showui;
	int m_last_ui_draw;
	int m_bmp_width, m_bmp_height;
	float m_bmp_fleft, m_bmp_ftop, m_bmp_fwidth, m_bmp_fheight;
	Imy12doomRendererCallback *m_cb;

	// get
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
	REFERENCE_TIME m_t;
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{m_t = tStart; return S_OK;}

	// dshow variables
	CCritSec m_data_lock;
	BYTE *m_data;
	bool m_data_changed;
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height
	double m_offset_x /*= -0.0*/;
	double m_offset_y /*= 0.0*/;
	double m_source_aspect /*= (double)m_lVidWidth / m_lVidHeight*/;
	int m_pass1_width;
	int m_pass1_height;
	GUID m_format;

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
	HRESULT load_image_convert();
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
	} m_vertices[40];
	bool m_swapeyes;
	output_mode_types m_output_mode;
	input_layout_types m_input_layout;
	mask_mode_types m_mask_mode;
	HWND m_hWnd;
	HWND m_hWnd2;
	CComPtr<IDirect3D9>		m_D3D;
	CComPtr<IDirect3DDevice9> m_Device;
	CComPtr<IDirect3DSwapChain9> m_swap1;
	CComPtr<IDirect3DSwapChain9> m_swap2;
	D3DPRESENT_PARAMETERS   m_new_pp;
	D3DPRESENT_PARAMETERS   m_active_pp;
	D3DPRESENT_PARAMETERS   m_active_pp2;
	CCritSec m_frame_lock;
	CCritSec m_device_lock;
	int m_device_threadid;

	CComPtr<IDirect3DVertexBuffer9> g_VertexBuffer;
	CComPtr <IDirect3DPixelShader9> m_ps_YUV2RGB;

	CComPtr<IDirect3DTexture9> g_tex_Y;						// source texture x3, 
	CComPtr<IDirect3DTexture9> g_tex_UV;
	CComPtr<IDirect3DTexture9> g_tex_rgb_left;				// source texture, converted to RGB32
	CComPtr<IDirect3DTexture9> g_tex_rgb_right;
	CComPtr<IDirect3DTexture9> g_tex_mask;					// mask txture
	CComPtr<IDirect3DTexture9> g_mask_temp_left;			// two temp texture, you may need it in some case
	CComPtr<IDirect3DTexture9> g_mask_temp_right;

	CComPtr<IDirect3DTexture9> g_tex_bmp;

	CComPtr<IDirect3DSurface9> g_sbs_surface;				// nv3d temp surface

	// render thread variables
	HANDLE m_render_thread;
	bool m_render_thread_exit;
	DWORD m_color1;
	DWORD m_color2;
	LONG m_style, m_exstyle;
	RECT m_window_pos;
};