#pragma  once
#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>
#include "my12doomRendererTypes.h"



typedef enum _anchor_point
{
	TOP = 1,
	BOTTOM = 2,
	LEFT = 4,
	RIGHT = 8,
	CENTER = 0x10,

	TOPLEFT = TOP + LEFT,
	TOPRIGHT = TOP + RIGHT,
	BOTTOMLEFT = BOTTOM + LEFT,
	BOTTOMRIGHT = BOTTOM + RIGHT,
} anchor_point;

typedef enum _hittest_result
{
	hit_out = -1,
	hit_logo = -2,
	hit_bg = 0,
	hit_button1 = 1,
	hit_button2 = 2,
	hit_button3 = 3,
	hit_button4 = 4,
	hit_play = 10,
	hit_pause = 11,
	hit_full = 12,
	hit_volume = 13,
	hit_progress = 14,
} hittest_result;

typedef struct _UI_element_fixed
{
	void *texture;
	int tx,ty,sx,sy,cx,cy;	// texture source: texture total width, total height, start x, start y, width, height
	int width,height;		// display width	not impl now

	anchor_point anchor;	// position
	int x,y;				// position offset
} UI_element_fixed;

typedef struct _UI_element_wrap
{
	void *texture;
	// float u,v :					// always whole texture
	anchor_point anchor1, anchor2;	// 2 anchor point
	int x1,y1, x2,y2;				// 2 position, only one dimention used per time.
	float tx, ty;
} UI_element_warp;

class ui_drawer_base
{
public:
	ui_drawer_base(){}
	virtual HRESULT init(int width, int height,IDirect3DDevice9 *device);
	virtual HRESULT uninit(int width, int height,IDirect3DDevice9 *device);
	virtual HRESULT init_gpu(int width, int height,IDirect3DDevice9 *device) PURE;
	virtual HRESULT init_cpu(int width, int height,IDirect3DDevice9 *device) PURE;
	virtual HRESULT invalidate_gpu() PURE;
	virtual HRESULT invalidate_cpu() PURE;
	virtual HRESULT draw_ui(IDirect3DSurface9 *surface, REFERENCE_TIME current, REFERENCE_TIME total, float volume, bool running, float alpha) PURE;
	virtual HRESULT draw_nonmovie_bg(IDirect3DSurface9 *surface, bool left_eye) PURE;
	virtual HRESULT hittest(int x, int y, int *out, double *outv = NULL) PURE;
};


class ui_drawer_dwindow : public ui_drawer_base
{
public:
	ui_drawer_dwindow(){}
	virtual HRESULT init_gpu(int width, int height, IDirect3DDevice9 *device);
	virtual HRESULT init_cpu(int width, int height, IDirect3DDevice9 *device);
	virtual HRESULT invalidate_gpu();
	virtual HRESULT invalidate_cpu();
	virtual HRESULT draw_ui(IDirect3DSurface9 *surface, REFERENCE_TIME current, REFERENCE_TIME total, float volume, bool running, float alpha);
	virtual HRESULT draw_nonmovie_bg(IDirect3DSurface9 *surface, bool left_eye);
	virtual HRESULT hittest(int x, int y, int *out, double *outv = NULL);

protected:
	int m_width;
	int m_height;
	IDirect3DDevice9 *m_Device;
	CComPtr<IDirect3DVertexBuffer9> m_vertex;
	CComPtr<IDirect3DTexture9> m_ui_logo_cpu;
	CComPtr<IDirect3DTexture9> m_ui_tex_cpu;
	CComPtr<IDirect3DTexture9> m_ui_background_cpu;
	CComPtr<IDirect3DTexture9> m_ui_logo_gpu;
	CComPtr<IDirect3DTexture9> m_ui_tex_gpu;
	CComPtr<IDirect3DTexture9> m_ui_background_gpu;
	CComPtr <IDirect3DPixelShader9> m_ps_UI;
	HRESULT init_ui2(IDirect3DSurface9 * surface);
	HRESULT draw_ui2(IDirect3DSurface9 * surface);

	//elements
	UI_element_fixed
		playbutton,
		pausebutton,
		current_time[5][10],
		colon[4],
		total_time[5][10],
		fullbutton,
		test_button,
		test_button2;

	UI_element_warp
		back_ground,
		volume,
		volume_top,
		volume_back,
		progressbar,
		progress_top,
		progress_bottom;

};
