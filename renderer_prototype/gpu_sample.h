#pragma once
class gpu_sample
{
public:
	gpu_sample(IMediaSample *memory_sample, CTextureAllocator *allocator, int width, int height, CLSID format, bool topdown_RGB32, bool interlaced = false, bool do_cpu_test = false, bool remux_mode = false, D3DPOOL pool = D3DPOOL_SYSTEMMEM, DWORD PC_LEVEL = 0);
	gpu_sample(const wchar_t *filename, CTextureAllocator *allocator);
	gpu_sample(CTextureAllocator *allocator, HFONT font, const wchar_t *text, RGBQUAD color, RECT *dst_rect = NULL, DWORD flag = DT_CENTER | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL);
	gpu_sample(IDirect3DDevice9 *device, IDirect3DSurface9 *surface, CTextureAllocator *allocator);
	gpu_sample(IDirect3DDevice9 *device, int width, int height, CTextureAllocator *allocator);		// for render target
	gpu_sample(IDirect3DDevice9 *device, int width, int height, void *data, CTextureAllocator *allocator);	// raw pre-multiplied RGBA data
	~gpu_sample();
	HRESULT commit();
	HRESULT decommit();
	HRESULT commitGDI();
	HRESULT decommitGDI();
	HRESULT convert_to_RGB32(IDirect3DDevice9 *device, IDirect3DPixelShader9 *ps_yv12, IDirect3DPixelShader9 *ps_nv12, IDirect3DPixelShader9 *ps_P016, IDirect3DPixelShader9 *ps_yuy2, IDirect3DVertexBuffer9 *vb, int time);
	HRESULT convert_to_RGB32_CPU(void *Y, void*U, void*V, int stride, int width, int height);
	HRESULT do_stereo_test(IDirect3DDevice9 *device, IDirect3DPixelShader9 *shader_sbs, IDirect3DPixelShader9 *shader_tb, IDirect3DVertexBuffer9 *vb);
	HRESULT get_strereo_test_result(IDirect3DDevice9 *device, int *out);		// S_FALSE: unkown, S_OK: out = (input_layout_types)
	HRESULT set_interlace(bool interlace);

	bool m_ready;
	int m_width;
	int m_height;
	REFERENCE_TIME m_start;
	REFERENCE_TIME m_end;
	int m_fn;
	CLSID m_format;
	bool m_interlaced;
	bool m_GDI_prepared;
	bool m_need_backup_when_decommitting;
	bool m_no_pool;
	RECT *m_ROI;		// ROI for rendering, usally for subtitle
	RECT *m_ROIex;		// ROI for committing, usually with a black padding

	CPooledTexture *m_tex_gpu_RGB32;				// GPU RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_gpu_Y;					// GPU Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_gpu_YV12_UV;				// GPU UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_gpu_NV12_UV;				// GPU UV plane of NV12, in A8L8
	CPooledTexture *m_tex_gpu_YUY2_UV;				// GPU YUY2 planes, in A8R8G8B8, half width
	DWORD m_interlace_flags;
protected:
	void zero(IDirect3DDevice9 *device, CTextureAllocator *allocator);
	bool is_ignored_line(int line);
	CTextureAllocator *m_allocator;
	bool m_prepared_for_rendering;
	bool m_converted;
	bool m_cpu_stereo_tested;
	input_layout_types m_cpu_tested_result;

	CPooledTexture *m_tex_RGB32;					// RGB32 planes, in A8R8G8B8, full width
	CPooledTexture *m_tex_Y;						// Y plane of YV12/NV12, in L8
	CPooledTexture *m_tex_YV12_UV;					// UV plane of YV12, in L8, double height
	CPooledTexture *m_tex_NV12_UV;					// UV plane of NV12, in A8L8
	CPooledTexture *m_tex_YUY2_UV;						// YUY2 planes, in A8R8G8B8, half width


	CPooledSurface *m_surf_YUY2;
	CPooledSurface *m_surf_YV12;
	CPooledSurface *m_surf_NV12;

	CPooledSurface *m_surf_gpu_YUY2;
	CPooledSurface *m_surf_gpu_YV12;
	CPooledSurface *m_surf_gpu_NV12;

	bool m_StretchRect;								// StretchRect level correct

	CPooledTexture *m_tex_stereo_test;
	CPooledTexture *m_tex_stereo_test_cpu;
	IDirect3DDevice9 *m_device;

	D3DPOOL m_pool;
	CCritSec m_sample_lock;
};