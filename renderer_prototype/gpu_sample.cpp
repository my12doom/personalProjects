#include <streams.h>
#include <atlbase.h>
#include <d3d9.h>
#include "TextureAllocator.h"
#include "my12doomRendererTypes.h"

#include "gpu_sample.h"
#include "..\ZBuffer\stereo_test.h"
#include "unpack_YUY2.h"

extern HRESULT mylog(wchar_t *format, ...);
extern HRESULT mylog(const char *format, ...);

gpu_sample::~gpu_sample()
{
	safe_delete(m_tex_RGB32);
	safe_delete(m_tex_Y);
	safe_delete(m_tex_YV12_UV);
	safe_delete(m_tex_NV12_UV);
	safe_delete(m_tex_YUY2_UV);

	safe_delete(m_tex_gpu_RGB32);
	safe_delete(m_tex_gpu_Y);
	safe_delete(m_tex_gpu_YV12_UV);
	safe_delete(m_tex_gpu_NV12_UV);
	safe_delete(m_tex_gpu_YUY2_UV);

	safe_delete(m_tex_stereo_test);
	safe_delete(m_tex_stereo_test_cpu);

	safe_delete(m_surf_YV12);
	safe_delete(m_surf_NV12);
	safe_delete(m_surf_YUY2);

	safe_delete(m_surf_gpu_YV12);
	safe_delete(m_surf_gpu_NV12);
	safe_delete(m_surf_gpu_YUY2);
}

CCritSec g_gpu_lock;
HRESULT gpu_sample::commit()
{
	// 	LARGE_INTEGER counter1, counter2, fre;
	// 	QueryPerformanceCounter(&counter1);
	// 	QueryPerformanceFrequency(&fre);
	CAutoLock lck(&m_sample_lock);

	if (m_prepared_for_rendering)
		return S_FALSE;

	if (m_tex_RGB32) 
		m_tex_RGB32->Unlock();

	if (m_tex_YUY2_UV) 
		m_tex_YUY2_UV->Unlock();

	if (m_tex_Y)
		m_tex_Y->Unlock();

	if (m_tex_YV12_UV)
		m_tex_YV12_UV->Unlock();

	if (m_tex_NV12_UV) 
		m_tex_NV12_UV->Unlock();

	if (m_surf_YV12)
		m_surf_YV12->Unlock();
	if (m_surf_NV12)
		m_surf_NV12->Unlock();
	if (m_surf_YUY2)
		m_surf_YUY2->Unlock();

	HRESULT hr = S_OK;

	JIF( m_allocator->CreateTexture(m_width, m_height, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_gpu_RGB32));

	if (m_format == MEDIASUBTYPE_YUY2)
	{
		JIF( m_allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
		JIF( m_allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_A8L8, D3DPOOL_DEFAULT,	&m_tex_gpu_YUY2_UV));
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		JIF( m_allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
		JIF( m_allocator->CreateTexture(m_width/2, m_height/2, NULL, D3DFMT_A8L8,D3DPOOL_DEFAULT,	&m_tex_gpu_NV12_UV));
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		JIF( m_allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_Y));
		JIF( m_allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_L8,D3DPOOL_DEFAULT,	&m_tex_gpu_YV12_UV));
	}

	if (m_tex_RGB32) JIF(m_allocator->UpdateTexture(m_tex_RGB32, m_tex_gpu_RGB32));
	if (m_tex_Y) JIF(m_allocator->UpdateTexture(m_tex_Y, m_tex_gpu_Y));
	if (m_tex_YV12_UV) JIF(m_allocator->UpdateTexture(m_tex_YV12_UV, m_tex_gpu_YV12_UV));
	if (m_tex_NV12_UV) JIF(m_allocator->UpdateTexture(m_tex_NV12_UV, m_tex_gpu_NV12_UV));
	if (m_tex_YUY2_UV) JIF(m_allocator->UpdateTexture(m_tex_YUY2_UV, m_tex_gpu_YUY2_UV));

	m_prepared_for_rendering = true;
	// 	QueryPerformanceCounter(&counter2);
	// 
	// 	mylog("prepare_rendering() cost %d cycle(%.3fms).\n", (int)(counter2.QuadPart - counter1.QuadPart), (double)(counter2.QuadPart-counter1.QuadPart)/fre.QuadPart);

	return S_OK;

clearup:
	decommit();
// 	mylog("%08x commit() fail", this);
	return E_FAIL;
}

HRESULT gpu_sample::decommit()
{
	CAutoLock lck(&m_sample_lock);
	if (!m_prepared_for_rendering)
		return S_FALSE;

	safe_delete(m_tex_gpu_RGB32);
	safe_delete(m_tex_gpu_Y);
	safe_delete(m_tex_gpu_YV12_UV);
	safe_delete(m_tex_gpu_NV12_UV);
	safe_delete(m_tex_gpu_YUY2_UV);
	safe_delete(m_tex_stereo_test);
	safe_delete(m_surf_gpu_YV12);
	safe_delete(m_surf_gpu_NV12);
	safe_delete(m_surf_gpu_YUY2);

	m_converted = m_prepared_for_rendering = false;
	return S_OK;
}

HRESULT gpu_sample::convert_to_RGB32(IDirect3DDevice9 *device, IDirect3DPixelShader9 *ps_yv12, IDirect3DPixelShader9 *ps_nv12, IDirect3DPixelShader9 *ps_yuy2,
									 IDirect3DVertexBuffer9 *vb, int time)
{
	CAutoLock lck(&m_sample_lock);
	if (!m_ready)
		return VFW_E_WRONG_STATE;

	if (m_converted)
		return S_FALSE;

	HRESULT hr = commit();
	if (FAILED(hr))
		return hr;

	if (m_format == MEDIASUBTYPE_RGB32 || m_format == MEDIASUBTYPE_ARGB32)
		return S_FALSE;

	if (m_StretchRect)
	{
		CComPtr<IDirect3DSurface9> rgb_surf;
		m_tex_gpu_RGB32->get_first_level(&rgb_surf);
		if (m_surf_YV12)
			hr = device->StretchRect(m_surf_YV12->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
		if (m_surf_NV12)
			hr = device->StretchRect(m_surf_NV12->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
		if (m_surf_YUY2)
			hr = device->StretchRect(m_surf_YUY2->surface, NULL, rgb_surf, NULL, D3DTEXF_LINEAR);
	}
	else
	{
		CComPtr<IDirect3DSurface9> rt;
		m_tex_gpu_RGB32->get_first_level(&rt);
		device->SetRenderTarget(0, rt);

		device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		hr = device->SetPixelShader(NULL);
		if (m_format == MEDIASUBTYPE_YV12) hr = device->SetPixelShader(ps_yv12);
		if (m_format == MEDIASUBTYPE_NV12 || m_format == MEDIASUBTYPE_YUY2) hr = device->SetPixelShader(ps_nv12);
		float rect_data[8] = {m_width, m_height, m_width/2, m_height, (float)time/100000, (float)timeGetTime()/100000};
		hr = device->SetPixelShaderConstantF(0, rect_data, 2);
		hr = device->SetRenderState(D3DRS_LIGHTING, FALSE);
		hr = device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		// vertex
		MyVertex vertex[4];
		vertex[0].x = 0; vertex[0].y = 0; vertex[0].tu = 0; vertex[0].tv = 0;
		vertex[1].x = m_width; vertex[1].y = 0; vertex[1].tu = 1; vertex[1].tv = 0;
		vertex[2].x = 0; vertex[2].y = m_height; vertex[2].tu = 0; vertex[2].tv = 1;
		vertex[3].x = m_width; vertex[3].y = m_height; vertex[3].tu = 1; vertex[3].tv = 1;
		for(int i=0; i<4; i++)
		{
			vertex[i].z = 1;
			vertex[i].w = 1;
			vertex[i].x -= 0.5;
			vertex[i].y -= 0.5;
		}


		// drawing
		hr = device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = device->SetSamplerState( 1, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = device->SetSamplerState( 2, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

		hr = device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

		hr = device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = device->SetStreamSource( 0, vb, 0, sizeof(MyVertex) );
		hr = device->SetFVF( FVF_Flags );

		hr = device->SetTexture( 0, m_tex_gpu_Y->texture );
		hr = device->SetTexture( 1, m_format == MEDIASUBTYPE_NV12 ? m_tex_gpu_NV12_UV->texture : (m_format == MEDIASUBTYPE_YUY2 ? m_tex_gpu_YUY2_UV->texture :m_tex_gpu_YV12_UV->texture));
		hr = device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex) );
	}

	if (SUCCEEDED(hr))
		m_converted = true;
	return hr;
}

HRESULT gpu_sample::do_stereo_test(IDirect3DDevice9 *device, IDirect3DPixelShader9 *shader_sbs, IDirect3DPixelShader9 *shader_tb, IDirect3DVertexBuffer9 *vb)
{
	if (m_cpu_stereo_tested)
		return S_FALSE;

	if (!device || !shader_sbs || !shader_tb ||!vb)
		return E_POINTER;

	if (m_tex_stereo_test)
		return S_FALSE;

	// 	HRESULT hr;
	// 	if (FAILED( hr = m_allocator->CreateTexture(stereo_test_texture_size, stereo_test_texture_size, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,	&m_tex_stereo_test)))
	// 		return hr;
	// 
	// 	CComPtr<IDirect3DSurface9> rt;
	// 	m_tex_stereo_test->get_first_level(&rt);
	// 	device->SetRenderTarget(0, rt);
	// 
	// 	device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	// 	device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	// 	device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	// 	hr = device->SetRenderState(D3DRS_LIGHTING, FALSE);
	// 	hr = device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	// 	CComPtr<IDirect3DSurface9> left_surface;
	// 	CComPtr<IDirect3DSurface9> right_surface;
	// 
	// 	// drawing
	// 	hr = device->SetStreamSource( 0, vb, 0, sizeof(MyVertex) );
	// 	hr = device->SetFVF( FVF_Flags );
	// 
	// 
	// 	hr = device->SetTexture( 0, m_tex_gpu_RGB32->texture );
	// 	hr = device->SetPixelShader(shader_sbs);
	// 	hr = device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_test_sbs, 2 );
	// 	hr = device->SetPixelShader(shader_tb);
	// 	hr = device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_test_tb, 2 );
	// 
	// 	//hr = device->EndScene();
	// 
	// 	mylog("test:%x:%d\n", this, timeGetTime());

	return S_OK;
}

HRESULT gpu_sample::get_strereo_test_result(IDirect3DDevice9 *device, int *out)
{
	if (m_cpu_stereo_tested)
	{
		*out = m_cpu_tested_result;
		return m_cpu_tested_result == input_layout_auto ? S_FALSE : S_OK;
	}

	if (!m_tex_stereo_test)
		return E_FAIL;	//call do_stereo_test() first

	HRESULT hr;
	HRESULT rtn = S_FALSE;
	bool do_transition = false;
	if (!m_tex_stereo_test_cpu)
	{
		if (FAILED( hr = m_allocator->CreateTexture(stereo_test_texture_size, stereo_test_texture_size, NULL, D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,	&m_tex_stereo_test_cpu)))
			return hr;

		do_transition = true;
	}

	m_tex_stereo_test_cpu->Unlock();

	mylog("get:%x:%d\n", this, timeGetTime());

	CComPtr<IDirect3DSurface9> gpu;
	CComPtr<IDirect3DSurface9> cpu;
	m_tex_stereo_test->get_first_level(&gpu);
	m_tex_stereo_test_cpu->get_first_level(&cpu);

	if (do_transition)
	{
		int l = timeGetTime();
		hr = device->GetRenderTargetData(gpu, cpu);
		if (timeGetTime()-l>3) mylog("GetRenderTargetData() cost %dms.\n", timeGetTime()-l);
	}

	D3DLOCKED_RECT locked;
	hr = m_tex_stereo_test_cpu->texture->LockRect(0, &locked, NULL, D3DLOCK_READONLY);
	if (FAILED(hr))
		return hr;

	BYTE* src = (BYTE*)locked.pBits;
	double average1 = 0;
	double average2 = 0;
	double delta1 = 0;
	double delta2 = 0;
	for(int y=0; y<stereo_test_texture_size; y++)
		for(int x=0; x<stereo_test_texture_size; x++)
		{
			double &average = x<stereo_test_texture_size/2 ?average1:average2;
			average += src[2];
			src += 4;
		}

		average1 /= stereo_test_texture_size/2*stereo_test_texture_size;
		average2 /= stereo_test_texture_size/2*stereo_test_texture_size;

		src = (BYTE*)locked.pBits;
		for(int y=0; y<stereo_test_texture_size; y++)
			for(int x=0; x<stereo_test_texture_size; x++)
			{
				double &average = x<stereo_test_texture_size/2 ?average1:average2;
				double &tdelta = x<stereo_test_texture_size/2 ? delta1 : delta2;

				int delta = abs(src[2] - average);
				tdelta += delta * delta;
				src += 4;
			}


			delta1 = sqrt((double)delta1)/(stereo_test_texture_size/2*stereo_test_texture_size-1);
			delta2 = sqrt((double)delta2)/(stereo_test_texture_size/2*stereo_test_texture_size-1);

			double times = 0;
			double var1 = average1 * delta1;
			double var2 = average2 * delta2;
			if ( (var1 > 0.001 && var2 > 0.001) || (var1>var2*10000) || (var2>var1*10000))
				times = var1 > var2 ? var1 / var2 : var2 / var1;

			printf("%f - %f, %f - %f, %f - %f, %f\r\n", average1, average2, delta1, delta2, var1, var2, times);

			if (times > 31.62/2)		// 10^1.5
			{
				mylog("stereo(%s).\r\n", var1 > var2 ? "tb" : "sbs");
				rtn = S_OK;
				*out = var1>var2 ? top_bottom : side_by_side;
			}
			else if ( 1.0 < times && times < 4.68 )
			{
				//m_normal ++;
				mylog("normal.\r\n");
				rtn = S_OK;
				*out = mono2d;
			}
			else
			{
				rtn = S_FALSE;
				mylog("unkown.\r\n");
			}
			m_tex_stereo_test_cpu->texture->UnlockRect(0);

			return rtn;
}

bool gpu_sample::is_ignored_line(int line)
{
	if (line == 1 || line == 136 || line == 272 || line == 408 || line == 544 || line == 680 || line == 816 || line == 952)
		return true;
	return false;
}

gpu_sample::gpu_sample(IMediaSample *memory_sample, CTextureAllocator *allocator, int width, int height, CLSID format,
					   bool topdown_RGB32, bool do_cpu_test, bool remux_mode, D3DPOOL pool, DWORD PC_LEVEL)
{
	//CAutoLock lck(&g_gpu_lock);
	m_allocator = allocator;
	m_interlace_flags = 0;
	m_tex_RGB32 = m_tex_YUY2_UV = m_tex_Y = m_tex_YV12_UV = m_tex_NV12_UV = NULL;
	m_tex_gpu_RGB32 = m_tex_gpu_YUY2_UV = m_tex_gpu_Y = m_tex_gpu_YV12_UV = m_tex_gpu_NV12_UV = NULL;
	m_surf_YV12 = m_surf_NV12 = m_surf_YUY2 = NULL;
	m_surf_gpu_YV12 = m_surf_gpu_NV12 = m_surf_gpu_YUY2 = NULL;
	m_tex_stereo_test = m_tex_stereo_test_cpu = NULL;
	m_width = width;
	m_height = height;
	m_ready = false;
	m_format = format;
	m_topdown = topdown_RGB32;
	m_prepared_for_rendering = false;
	m_converted = false;
	m_cpu_stereo_tested = false;
	m_cpu_tested_result = input_layout_auto;		// means unknown
	HRESULT hr;
	CComQIPtr<IMediaSample2, &IID_IMediaSample2> I2(memory_sample);
	if (!allocator || !memory_sample)
		goto clearup;

	if (remux_mode && format != MEDIASUBTYPE_YUY2)
		goto clearup;

	// time stamp
	memory_sample->GetTime(&m_start, &m_end);

	// interlace flags
	if (I2)
	{
		AM_SAMPLE2_PROPERTIES prop;
		if (SUCCEEDED(I2->GetProperties(sizeof(prop), (BYTE*) &prop)))
		{
			m_interlace_flags = prop.dwTypeSpecificFlags;
		}

		//printf("sample interlace_flag: %02x.\n", m_interlace_flags);
	}


	int l = timeGetTime();

	m_ready = true;
	//return;

	m_StretchRect = false;
	if (m_format == MEDIASUBTYPE_YUY2)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_YUY2))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2'), D3DPOOL_DEFAULT, &m_surf_YUY2));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8, pool,	&m_tex_Y));
			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_A8L8, pool,	&m_tex_YUY2_UV));
		}
	}

	else if (m_format == MEDIASUBTYPE_RGB32 || m_format == MEDIASUBTYPE_ARGB32)
	{
		JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_A8R8G8B8,pool,	&m_tex_RGB32));
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_NV12))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('N','V','1','2'), D3DPOOL_DEFAULT, &m_surf_NV12));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,pool,	&m_tex_Y));
			JIF( allocator->CreateTexture(m_width/2, m_height/2, NULL, D3DFMT_A8L8,pool,	&m_tex_NV12_UV));
		}
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		if ((PC_LEVEL & PCLEVELTEST_TESTED) && (PC_LEVEL &PCLEVELTEST_YV12))
		{
			m_StretchRect = true;
			JIF( allocator->CreateOffscreenSurface(m_width, m_height, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_surf_YV12));
		}
		else
		{
			JIF( allocator->CreateTexture(m_width, m_height, NULL, D3DFMT_L8,pool,	&m_tex_Y));
			JIF( allocator->CreateTexture(m_width/2, m_height, NULL, D3DFMT_L8,pool,	&m_tex_YV12_UV));
		}
	}

	m_pool = pool;
	m_ready = true;


	int l2 = timeGetTime();

	// data loading
	BYTE * src;
	BYTE * dst;
	memory_sample->GetPointer(&src);
	if (m_format == MEDIASUBTYPE_RGB32 || m_format == MEDIASUBTYPE_ARGB32)
	{
		for(int i=0; i<m_height; i++)
		{
			D3DLOCKED_RECT &d3dlr = m_tex_RGB32->locked_rect;
			memory_sample->GetPointer(&src);
			src += m_width*4*(topdown_RGB32?m_height-i-1:i);
			dst = (BYTE*)d3dlr.pBits + d3dlr.Pitch*i;

			memcpy(dst, src, m_width*4);
		}
		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<DWORD>(src, width, height, (int*)&m_cpu_tested_result);
	}

	else if (m_format == MEDIASUBTYPE_YUY2)
	{
		// loading YUY2 image as one L8 Y texture and one A8L8 UV half width texture
		D3DLOCKED_RECT d3dlr = m_tex_Y->locked_rect;
		D3DLOCKED_RECT d3dlr_UV;
		if (m_StretchRect)
			d3dlr_UV = m_surf_YUY2->locked_rect;
		else
			d3dlr_UV = m_tex_YUY2_UV->locked_rect;

		unpack_YUY2(width, height, src, width*2, d3dlr.pBits, d3dlr.Pitch, d3dlr_UV.pBits, d3dlr.Pitch);

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<WORD>(src, width, height, (int*)&m_cpu_tested_result);
	}

	else if (m_format == MEDIASUBTYPE_NV12)
	{

		if (m_StretchRect)
		{
			D3DLOCKED_RECT &d3dlr = m_surf_NV12->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height*3/2; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}
		}

		else
		{
			// loading NV12 image as one L8 texture and one A8L8 texture
			// load Y
			D3DLOCKED_RECT &d3dlr = m_tex_Y->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}

			// load UV
			D3DLOCKED_RECT &d3dlr2 = m_tex_NV12_UV->locked_rect;
			dst = (BYTE*)d3dlr2.pBits;
			for(int i=0; i<m_height/2; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr2.Pitch;
			}
		}

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<BYTE>(src, width, height, (int*)&m_cpu_tested_result);
	}

	else if (m_format == MEDIASUBTYPE_YV12)
	{
		if (m_StretchRect)
		{
			D3DLOCKED_RECT &d3dlr = m_surf_YV12->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width/2);
				src += m_width/2;
				dst += d3dlr.Pitch/2;
			}
		}

		else
		{
			// loading YV12 image as two L8 texture
			// load Y
			D3DLOCKED_RECT &d3dlr = m_tex_Y->locked_rect;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width);
				src += m_width;
				dst += d3dlr.Pitch;
			}

			// load UV
			D3DLOCKED_RECT &d3dlr2 = m_tex_YV12_UV->locked_rect;
			dst = (BYTE*)d3dlr2.pBits;
			for(int i=0; i<m_height; i++)
			{
				memcpy(dst, src, m_width/2);
				src += m_width/2;
				dst += d3dlr2.Pitch;
			}
		}

		memory_sample->GetPointer(&src);
		if (do_cpu_test) get_layout<BYTE>(src, width, height, (int*)&m_cpu_tested_result);
	}

	m_cpu_stereo_tested = do_cpu_test;


	//prepare_rendering();
	//if (timeGetTime() - l > 5)
	//mylog("load():createTexture time:%d ms, load data to GPU cost %d ms.\n", l2- l, timeGetTime()-l2);

	return;

clearup:
	safe_delete(m_tex_RGB32);
	safe_delete(m_tex_Y);
	safe_delete(m_tex_YV12_UV);
	safe_delete(m_tex_NV12_UV);
	safe_delete(m_tex_YUY2_UV);
}
