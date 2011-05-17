// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include "PixelShaders/YV12.h"
#include "PixelShaders/NV12.h"
#include "PixelShaders/YUY2.h"
#include "3dvideo.h"


HRESULT mylog(wchar_t *format, ...)
{
#ifdef DEBUG
	wchar_t tmp[10240];
	wchar_t tmp2[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wsprintfW(tmp2, L"(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringW(tmp2);
#endif
	return S_OK;
}


HRESULT mylog(const char *format, ...)
{
#ifdef DEBUG
	char tmp[10240];
	char tmp2[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfA(tmp, format, valist);
	va_end(valist);

	wsprintfA(tmp2, "(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringA(tmp2);
#endif
	return S_OK;
}

enum vertex_types
{
	vertex_pass1_types_count = 5,
	vertex_point_per_type = 4,

	vertex_pass1_whole = 0,
	vertex_pass1_left = 4,
	vertex_pass1_right = 8,
	vertex_pass1_top = 12,
	vertex_pass1_bottom = 16,

	vertex_pass2_main = 20,
	vertex_pass2_second = 24,
	vertex_pass3 = 28,

	vertex_bmp = 32,

	vertex_ui = 36,
};

const DWORD FVF_Flags = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;

HRESULT my12doomRenderer::handle_device_state()							//handle device create/recreate/lost/reset
{
	if (!m_pInputPin ||  !m_pInputPin->IsConnected())
		return VFW_E_NOT_CONNECTED;
	if (GetCurrentThreadId() != m_device_threadid)
	{
		if (m_device_state < device_lost)
			return S_OK;
		else
			return E_FAIL;
	}

	if (m_device_state == fine)
		return S_OK;

	else if (m_device_state == create_failed)
		return E_FAIL;

	else if (m_device_state == need_reset_object)
	{
		CAutoLock lck(&m_device_lock);
		invalidate_objects();
		restore_objects();
		m_device_state = fine;
	}

	else if (m_device_state == need_reset)
	{
		CAutoLock lck(&m_device_lock);
		invalidate_objects();
		HRESULT hr = m_Device->Reset( &m_new_pp );
		if( FAILED(hr ) )
		{
			m_device_state = device_lost;
			return hr;
		}
		m_active_pp = m_new_pp;
		restore_objects();
		m_device_state = fine;
	}

	else if (m_device_state == device_lost)
	{
		Sleep(100);
		if( m_Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
		{
			CAutoLock lck(&m_device_lock);
			invalidate_objects();
			HRESULT hr = m_Device->Reset( &m_new_pp );

			if( FAILED(hr ) )
			{
				m_device_state = device_lost;
				return hr;
			}
			m_active_pp = m_new_pp;
			restore_objects();
			
			m_device_state = fine;
			return hr;
		}

		else
			return E_FAIL;
	}

	else if (m_device_state == need_create)
	{

		ZeroMemory( &m_active_pp, sizeof(m_active_pp) );
		m_active_pp.Windowed               = TRUE;
		m_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
		m_active_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
		m_active_pp.BackBufferCount = 1;
		m_active_pp.Flags = D3DPRESENTFLAG_VIDEO;

		m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
		m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
		GetWindowRect(m_hWnd, &m_window_pos);

		set_fullscreen(false);

		UINT AdapterToUse=D3DADAPTER_DEFAULT;
		D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;
		for (UINT Adapter=0;Adapter<m_D3D->GetAdapterCount();Adapter++)
		{ 
			D3DADAPTER_IDENTIFIER9  Identifier; 
			HRESULT       Res; 

			Res = m_D3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
			if (strstr(Identifier.Description,"PerfHUD") != 0) 
			{ 
				AdapterToUse=Adapter; 
				DeviceType=D3DDEVTYPE_REF; 
				break; 
			}
		}
		HRESULT hr = m_D3D->CreateDevice( AdapterToUse, DeviceType,
			m_hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&m_active_pp, &m_Device );

		if (FAILED(hr))
		{
			m_device_state = create_failed;
			return hr;
		}
		m_new_pp = m_active_pp;
		restore_objects();
		m_device_state = fine;
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_device_state(device_state new_state)
{
	m_device_state = max(m_device_state, new_state);
	return S_OK;
}
HRESULT my12doomRenderer::shutdown()
{	
	CAutoLock lck(&m_device_lock);
    invalidate_objects();
	g_tex_Y = NULL;
	g_tex_UV = NULL;
	g_tex_bmp = NULL;
	m_Device = NULL;

	set_device_state(need_create);

	return S_OK;
}
HRESULT my12doomRenderer::create_render_thread()
{
	if (INVALID_HANDLE_VALUE != m_render_thread)
		terminate_render_thread();
	m_render_thread_exit = false;
	m_render_thread = CreateThread(0,0,render_thread, this, NULL, NULL);
	return S_OK;
}
HRESULT my12doomRenderer::terminate_render_thread()
{
	if (INVALID_HANDLE_VALUE == m_render_thread)
		return S_FALSE;
	m_render_thread_exit = true;
	WaitForSingleObject(m_render_thread, INFINITE);
	m_render_thread = INVALID_HANDLE_VALUE;

	return S_OK;
}
HRESULT my12doomRenderer::invalidate_objects()
{
	terminate_render_thread();
	m_ps_YUV2RGB = NULL;
	g_tex_mask = NULL;
	g_tex_rgb_left = NULL;
	g_tex_rgb_right = NULL;
	g_mask_temp_left = NULL;
	g_mask_temp_right = NULL;
	g_sbs_surface = NULL;
	g_VertexBuffer = NULL;

	m_swap1 = NULL;
	m_swap2 = NULL;

	return S_OK;
}
HRESULT my12doomRenderer::restore_objects()
{
	int l = timeGetTime();
	m_pass1_width = m_lVidWidth;
	m_pass1_height = m_lVidHeight;
	if (m_input_layout == side_by_side)
		m_pass1_width /= 2;
	else if (m_input_layout == top_bottom)
		m_pass1_height /= 2;


	HRESULT hr = m_Device->GetSwapChain(0, &m_swap1);
	m_active_pp2 = m_active_pp;
	RECT rect;
	GetClientRect(m_hWnd2, &rect);
	m_active_pp2.BackBufferWidth = rect.right - rect.left;
	m_active_pp2.BackBufferHeight = rect.bottom - rect.top;
	hr = m_Device->CreateAdditionalSwapChain(&m_active_pp2, &m_swap2);

	if (g_tex_Y == NULL)
	{
		if (m_format == MEDIASUBTYPE_YUY2)
		{
			hr = m_Device->CreateTexture(m_lVidWidth/2, m_lVidHeight, 1, NULL, D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,	&g_tex_Y, NULL);
		}
		else if (m_format == MEDIASUBTYPE_NV12)
		{
			hr = m_Device->CreateTexture(m_lVidWidth, m_lVidHeight, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_Y, NULL);
			hr = m_Device->CreateTexture(m_lVidWidth/2, m_lVidHeight/2, 1, NULL, D3DFMT_A8L8,D3DPOOL_MANAGED,	&g_tex_UV, NULL);
		}
		else if (m_format == MEDIASUBTYPE_YV12)
		{
			hr = m_Device->CreateTexture(m_lVidWidth, m_lVidHeight, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_Y, NULL);
			hr = m_Device->CreateTexture(m_lVidWidth/2, m_lVidHeight, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_UV, NULL);
		}
	}

	hr = m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_left, NULL);
	hr = m_Device->CreateTexture(m_pass1_width, m_pass1_height, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_right, NULL);
	hr = m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_mask_temp_left, NULL);
	hr = m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_mask_temp_right, NULL);
	hr = m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &g_tex_mask, NULL);
	hr = m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1, m_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &g_sbs_surface, NULL);

	if(g_tex_bmp == NULL) hr = m_Device->CreateTexture(4096, 4096, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,	&g_tex_bmp, NULL);

	mylog("restore 1st time:%dms\n", timeGetTime()-l);
	generate_mask();
	mylog("restore 2nd time:%dms\n", timeGetTime()-l);

	// add 3d vision tag at last line
	if (m_output_mode == NV3D)
	{
		D3DLOCKED_RECT lr;
		RECT lock_tar={0, m_active_pp.BackBufferHeight, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1};
		g_sbs_surface->LockRect(&lr,&lock_tar,0);
		LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
		pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pSIH->dwBPP = 32;
		pSIH->dwFlags = SIH_SIDE_BY_SIDE;
		pSIH->dwWidth = m_active_pp.BackBufferWidth;
		pSIH->dwHeight = m_active_pp.BackBufferHeight;
		g_sbs_surface->UnlockRect();
	}
	mylog("restore 3rd time:%dms\n", timeGetTime()-l);


	// vertex
	m_Device->CreateVertexBuffer( sizeof(m_vertices), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, FVF_Flags, D3DPOOL_DEFAULT, &g_VertexBuffer, NULL );
	mylog("restore 4th time:%dms\n", timeGetTime()-l);
	calculate_vertex();
	mylog("restore 5th time:%dms\n", timeGetTime()-l);

	// pixel shader
	if (m_format == MEDIASUBTYPE_YV12)
		hr = m_Device->CreatePixelShader(g_code_YV12toRGB, &m_ps_YUV2RGB);
	else if (m_format == MEDIASUBTYPE_NV12)
		hr = m_Device->CreatePixelShader(g_code_NV12toRGB, &m_ps_YUV2RGB);
	else if (m_format == MEDIASUBTYPE_YUY2)
		hr = m_Device->CreatePixelShader(g_code_YUY2toRGB, &m_ps_YUV2RGB);
	else
		return VFW_E_INVALID_FILE_FORMAT;


	// Create the pixel shader
	if (FAILED(hr)) return E_FAIL;
	mylog("restore 6th time:%dms\n", timeGetTime()-l);

	load_image_convert();


	// create render thread
	create_render_thread();
	mylog("restore last time:%dms\n", timeGetTime()-l);
	return S_OK;
}
HRESULT my12doomRenderer::render_nolock(bool forced)
{

	// device state check and restore
	if (FAILED(handle_device_state()))
		return E_FAIL;


	// image loading and idle check
	if (load_image() != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!m_Device)
		return E_FAIL;

	static int last_render_time = timeGetTime();

	HRESULT hr;

	CAutoLock lck(&m_frame_lock);
	// pass 2: drawing to back buffer!
	hr = m_Device->BeginScene();
	hr = m_Device->SetPixelShader(NULL);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);

	// set render target to back buffer
	CComPtr<IDirect3DSurface9> back_buffer;
	hr = m_Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	hr = m_Device->SetRenderTarget(0, back_buffer);

	// back ground
	#ifdef DEBUG
	m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
	#else
	m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
	#endif

	// reset texture blending stages
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
	m_Device->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);
	m_Device->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	m_Device->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 );
	m_Device->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	m_Device->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 0 );

	if (m_output_mode == NV3D)
	{
		// draw left
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, g_tex_rgb_left );
		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

		// copy left to nv3d surface
		RECT dst = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
		hr = m_Device->StretchRect(back_buffer, NULL, g_sbs_surface, &dst, D3DTEXF_NONE);

		// draw right
		m_Device->SetTexture( 0, g_tex_rgb_right );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

		// copy right to nv3d surface
		dst.left += m_active_pp.BackBufferWidth;
		dst.right += m_active_pp.BackBufferWidth;
		hr = m_Device->StretchRect(back_buffer, NULL, g_sbs_surface, &dst, D3DTEXF_LINEAR);

		// StretchRect to backbuffer!, this is how 3D vision works
		RECT tar = {0,0, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight};
		hr = m_Device->StretchRect(g_sbs_surface, &tar, back_buffer, NULL, D3DTEXF_NONE);		//source is as previous, tag line not overwrited
	}

	else if (m_output_mode == anaglyph)
	{
		m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		m_Device->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);	// current = texture * diffuse(red)

		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_SPECULAR );
		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG0, D3DTA_CURRENT );
		m_Device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD ); // Modulate then add...
		m_Device->SetTextureStageState( 1, D3DTSS_RESULTARG, D3DTA_CURRENT);     // current = texture * specular(cyan) + current

		m_Device->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);

		m_Device->SetTexture( 0, g_tex_rgb_left );
		m_Device->SetTexture( 1, g_tex_rgb_right );

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );
	}

	else if (m_output_mode == masking)
	{
		// pass 2: render seperate to two temp texture

		// draw left
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = g_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		hr = m_Device->SetRenderTarget(0, temp_surface);
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, g_tex_rgb_left );
		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
#ifdef DEBUG
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
#else
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
#endif
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

		// draw right
		temp_surface = NULL;
		hr = g_mask_temp_right->GetSurfaceLevel(0, &temp_surface);
		hr = m_Device->SetRenderTarget(0, temp_surface);
		m_Device->SetTexture( 0, g_tex_rgb_right );
#ifdef DEBUG
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
#else
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
#endif
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );


		// pass 3: render to backbuffer with masking
		m_Device->SetRenderTarget(0, back_buffer);
		m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );	// current = texture(mask)

		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
		m_Device->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SUBTRACT ); // subtract... to show only black 
		m_Device->SetTextureStageState( 1, D3DTSS_RESULTARG, D3DTA_TEMP);	  // temp = texture(left) - current(mask)

		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG2, D3DTA_CURRENT );
		m_Device->SetTextureStageState( 2, D3DTSS_COLORARG0, D3DTA_TEMP );		 // arg3 = arg0...
		m_Device->SetTextureStageState( 2, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD ); // Modulate then add... to show only white then add with temp(black)
		m_Device->SetTextureStageState( 2, D3DTSS_RESULTARG, D3DTA_CURRENT);     // current = texture(right) * current(mask) + temp

		m_Device->SetTexture( 0, g_tex_mask );
		m_Device->SetTexture( 1, g_mask_temp_left );
		m_Device->SetTexture( 2, g_mask_temp_right );

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass3, 2 );
	}

	else if (m_output_mode == mono)
	{
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, g_tex_rgb_left );

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

	}

	else if (m_output_mode == pageflipping)
	{
		static bool left = true;
		left = !left;

		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, left? g_tex_rgb_left : g_tex_rgb_right );

		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );
	}

	else if (m_output_mode == dual_window)
	{
		// draw left
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, g_tex_rgb_left );
		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

		// set render target to swap chain2
		if (m_swap2)
		{
			CComPtr<IDirect3DSurface9> back_buffer2;
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
			hr = m_Device->SetRenderTarget(0, back_buffer2);

			// back ground
			#ifdef DEBUG
			m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
			#else
			m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
			#endif

			// draw right
			m_Device->SetTexture( 0, g_tex_rgb_right );
			hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_second, 2 );
		}

	}

	else if (m_output_mode == out_side_by_side || m_output_mode == out_top_bottom)
	{
		// pass 2: render seperate to two temp texture

		// draw left
		CComPtr<IDirect3DSurface9> temp_surface;
		hr = g_mask_temp_left->GetSurfaceLevel(0, &temp_surface);
		hr = m_Device->SetRenderTarget(0, temp_surface);
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetTexture( 0, g_tex_rgb_left );
		hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
#ifdef DEBUG
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
#else
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
#endif
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );

		// draw right
		CComPtr<IDirect3DSurface9> temp_surface2;
		hr = g_mask_temp_right->GetSurfaceLevel(0, &temp_surface2);
		hr = m_Device->SetRenderTarget(0, temp_surface2);
		m_Device->SetTexture( 0, g_tex_rgb_right );
#ifdef DEBUG
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
#else
		m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
#endif
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );


		// pass 3: copy to backbuffer
		if (m_output_mode == out_side_by_side)
		{
			RECT src = {0, 0, m_active_pp.BackBufferWidth/2, m_active_pp.BackBufferHeight};
			RECT dst = src;

			m_Device->StretchRect(temp_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

			dst.left += m_active_pp.BackBufferWidth/2;
			dst.right += m_active_pp.BackBufferWidth/2;
			m_Device->StretchRect(temp_surface2, &src, back_buffer, &dst, D3DTEXF_NONE);
		}

		else if (m_output_mode == out_top_bottom)
		{
			RECT src = {0, 0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight/2};
			RECT dst = src;

			m_Device->StretchRect(temp_surface, &src, back_buffer, &dst, D3DTEXF_NONE);

			dst.top += m_active_pp.BackBufferHeight/2;
			dst.bottom += m_active_pp.BackBufferHeight/2;
			m_Device->StretchRect(temp_surface2, &src, back_buffer, &dst, D3DTEXF_NONE);

		}
	}

	// Alpha Bitmap
	m_Device->SetRenderTarget(0, back_buffer);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_Device->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
	m_Device->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);
	m_Device->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	m_Device->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	hr = m_Device->SetTexture( 0, g_tex_bmp );
	hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_bmp, 2 );

	// UI
	if (m_showui)
	{
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_ui, 2 );
	}

	m_Device->EndScene();

	if (m_output_mode == dual_window)
	{
		if (m_swap1->Present(NULL, NULL, m_hWnd, NULL, D3DPRESENT_DONOTWAIT) == D3DERR_DEVICELOST)
			set_device_state(device_lost);

		if(m_swap2) if (m_swap2->Present(NULL, NULL, m_hWnd2, NULL, D3DPRESENT_DONOTWAIT) == D3DERR_DEVICELOST)
			set_device_state(device_lost);
	}

	else
	{
		if (m_Device->Present( NULL, NULL, NULL, NULL ) ==  D3DERR_DEVICELOST)
			set_device_state(device_lost);
	}

	return S_OK;
}
HRESULT my12doomRenderer::render(bool forced)
{
	CAutoLock lck(&m_device_lock);
	if (m_Device)
		return render_nolock(forced);
	else
		return E_FAIL;
	return S_OK;
}
DWORD WINAPI my12doomRenderer::render_thread(LPVOID param)
{
	my12doomRenderer *_this = (my12doomRenderer*)param;

	int l = timeGetTime();
	while (timeGetTime() - l < 0 && !_this->m_render_thread_exit)
		Sleep(1);

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while(!_this->m_render_thread_exit)
	{
		if (_this->m_output_mode != pageflipping)
		{
			if (_this->m_State == State_Running)
			{
				Sleep(1);
				continue;
			}
			else
			{
				_this->render_nolock(true);
				l = timeGetTime();
				while (timeGetTime() - l < 333 && !_this->m_render_thread_exit)
					Sleep(1);
			}
		}
		else if (_this->render_nolock(true) == S_FALSE)
			Sleep(1);
	}	return S_OK;
}

// dx9 helper functions
HRESULT my12doomRenderer::load_image(bool forced /* = false */)
{
	CAutoLock lck(&m_frame_lock);
	if (!m_Device || !g_tex_rgb_left ||!g_tex_rgb_right)
		return E_FAIL;
	HRESULT hr;
	{
		CAutoLock lck(&m_data_lock);
		if (!forced && !m_data_changed)
			return S_FALSE;

		m_data_changed = false;

		D3DLOCKED_RECT d3dlr;
		BYTE * src = m_data;
		BYTE * dst;

		if (m_format == MEDIASUBTYPE_YUY2)
		{
			// loading YUY2 image as one ARGB half width texture
			if( FAILED(hr = g_tex_Y->LockRect(0, &d3dlr, 0, NULL)))
				return hr;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_lVidHeight; i++)
			{
				memcpy(dst, src, m_lVidWidth*2);
				src += m_lVidWidth*2;
				dst += d3dlr.Pitch;
			}
			g_tex_Y->UnlockRect(0);
		}

		else if (m_format == MEDIASUBTYPE_NV12)
		{
			// loading NV12 image as one L8 texture and one A8L8 texture

			//load Y
			if( FAILED(hr = g_tex_Y->LockRect(0, &d3dlr, 0, NULL)))
				return hr;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_lVidHeight; i++)
			{
				memcpy(dst, src, m_lVidWidth);
				src += m_lVidWidth;
				dst += d3dlr.Pitch;
			}
			g_tex_Y->UnlockRect(0);

			// load UV
			if( FAILED(hr = g_tex_UV->LockRect(0, &d3dlr, 0, NULL)))
				return hr;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_lVidHeight/2; i++)
			{
				memcpy(dst, src, m_lVidWidth);
				src += m_lVidWidth;
				dst += d3dlr.Pitch;
			}
			g_tex_UV->UnlockRect(0);
		}

		else if (m_format == MEDIASUBTYPE_YV12)
		{
			// loading YV12 image as two L8 texture
			// load Y
			if( FAILED(hr = g_tex_Y->LockRect(0, &d3dlr, 0, NULL)))
				return hr;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_lVidHeight; i++)
			{
				memcpy(dst, src, m_lVidWidth);
				src += m_lVidWidth;
				dst += d3dlr.Pitch;
			}
			g_tex_Y->UnlockRect(0);

			// load UV
			if( FAILED(hr = g_tex_UV->LockRect(0, &d3dlr, 0, NULL)))
				return hr;
			dst = (BYTE*)d3dlr.pBits;
			for(int i=0; i<m_lVidHeight; i++)
			{
				memcpy(dst, src, m_lVidWidth/2);
				src += m_lVidWidth/2;
				dst += d3dlr.Pitch;
			}
			g_tex_UV->UnlockRect(0);
		}

	}

	return load_image_convert();
}

HRESULT my12doomRenderer::load_image_convert()
{
	//warnig: no lock

	// pass 1: render full resolution to two seperate texture
	HRESULT hr = m_Device->BeginScene();
	hr = m_Device->SetPixelShader(m_ps_YUV2RGB);
	float rect_data[4] = {m_lVidWidth, m_lVidHeight, m_lVidWidth/2, m_lVidHeight};
	hr = m_Device->SetPixelShaderConstantF(0, rect_data, 1);
	hr = m_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);
	if (!m_swapeyes)
		hr = m_Device->SetRenderTarget(0, left_surface);
	else
		hr = m_Device->SetRenderTarget(0, right_surface);


	// drawing
	hr = m_Device->SetTexture( 0, g_tex_Y );
	hr = m_Device->SetTexture( 1, g_tex_UV );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = m_Device->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = m_Device->SetFVF( FVF_Flags );

	if (m_input_layout == side_by_side)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_left, 2 );
	else if (m_input_layout == top_bottom)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_top, 2 );
	else if (m_input_layout == mono2d)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );

	if (!m_swapeyes)
		hr = m_Device->SetRenderTarget(0, right_surface);
	else
		hr = m_Device->SetRenderTarget(0, left_surface);

	if (m_input_layout == side_by_side)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_right, 2 );
	else if (m_input_layout == top_bottom)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_bottom, 2 );
	else if (m_input_layout == mono2d)
		hr = m_Device->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );


	hr = m_Device->EndScene();
	return hr;
}

HRESULT my12doomRenderer::calculate_vertex()
{
	CAutoLock lck(&m_frame_lock);
	// w,color
	for(int i=0; i<sizeof(m_vertices) / sizeof(MyVertex); i++)
	{
		m_vertices[i].diffuse = m_color1;
		m_vertices[i].specular = m_color2;
		m_vertices[i].w = 1.0f;
		m_vertices[i].z = 1.0f;
	}
	// texture coordinate
	// pass1 whole
	m_vertices[0].tu = 0.0f; m_vertices[0].tv = 0.0f;
	m_vertices[1].tu = 1.0f; m_vertices[1].tv = 0.0f;
	m_vertices[2].tu = 0.0f; m_vertices[2].tv = 1.0f;
	m_vertices[3].tu = 1.0f; m_vertices[3].tv = 1.0f;

	// pass1 left
	m_vertices[4].tu = 0.0f; m_vertices[4].tv = 0.0f;
	m_vertices[5].tu = 0.5f; m_vertices[5].tv = 0.0f;
	m_vertices[6].tu = 0.0f; m_vertices[6].tv = 1.0f;
	m_vertices[7].tu = 0.5f; m_vertices[7].tv = 1.0f;

	// pass1 right
	m_vertices[8].tu = 0.5f; m_vertices[8].tv = 0.0f;
	m_vertices[9].tu = 1.0f; m_vertices[9].tv = 0.0f;
	m_vertices[10].tu = 0.5f; m_vertices[10].tv = 1.0f;
	m_vertices[11].tu = 1.0f; m_vertices[11].tv = 1.0f;

	// pass1 top
	m_vertices[12].tu = 0.0f; m_vertices[12].tv = 0.0f;
	m_vertices[13].tu = 1.0f; m_vertices[13].tv = 0.0f;
	m_vertices[14].tu = 0.0f; m_vertices[14].tv = 0.5f;
	m_vertices[15].tu = 1.0f; m_vertices[15].tv = 0.5f;

	// pass1 bottom
	m_vertices[16].tu = 0.0f; m_vertices[16].tv = 0.5f;
	m_vertices[17].tu = 1.0f; m_vertices[17].tv = 0.5f;
	m_vertices[18].tu = 0.0f; m_vertices[18].tv = 1.0f;
	m_vertices[19].tu = 1.0f; m_vertices[19].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for main back buffer
	m_vertices[20].tu = 0.0f; m_vertices[20].tv = 0.0f;
	m_vertices[21].tu = 1.0f; m_vertices[21].tv = 0.0f;
	m_vertices[22].tu = 0.0f; m_vertices[22].tv = 1.0f;
	m_vertices[23].tu = 1.0f; m_vertices[23].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for second back buffer
	m_vertices[24].tu = 0.0f; m_vertices[24].tv = 0.0f;
	m_vertices[25].tu = 1.0f; m_vertices[25].tv = 0.0f;
	m_vertices[26].tu = 0.0f; m_vertices[26].tv = 1.0f;
	m_vertices[27].tu = 1.0f; m_vertices[27].tv = 1.0f;

	// pass3 whole texture, whole back buffer output
	m_vertices[28].tu = 0.0f; m_vertices[28].tv = 0.0f;
	m_vertices[29].tu = 1.0f; m_vertices[29].tv = 0.0f;
	m_vertices[30].tu = 0.0f; m_vertices[30].tv = 1.0f;
	m_vertices[31].tu = 1.0f; m_vertices[31].tv = 1.0f;

	// pass1 coordinate
	for(int i=0; i<vertex_pass1_types_count; i++)
	{
		MyVertex * tmp = m_vertices + vertex_pass1_whole + vertex_point_per_type * i ;
		tmp[0].x = -0.5f; tmp[0].y = -0.5f;
		tmp[1].x = m_pass1_width-0.5f; tmp[1].y = -0.5f;
		tmp[2].x = -0.5f; tmp[2].y = m_pass1_height-0.5f;
		tmp[3].x =  m_pass1_width-0.5f; tmp[3].y =m_pass1_height-0.5f;
	}

	// pass2-3 coordinate
	// main window coordinate
	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_output_mode == out_side_by_side)
		tar.right /= 2;
	else if (m_output_mode == out_top_bottom)
		tar.bottom /= 2;

	int delta_w = (int)(tar.right - tar.bottom * m_source_aspect + 0.5);
	int delta_h = (int)(tar.bottom - tar.right  / m_source_aspect + 0.5);
	if (delta_w > 0)
	{
		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}

	int tar_width = tar.right-tar.left;
	int tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_offset_x);
	tar.right += (LONG)(tar_width * m_offset_x);
	tar.top += (LONG)(tar_height * m_offset_y);
	tar.bottom += (LONG)(tar_height * m_offset_y);

	MyVertex *tmp = m_vertices + vertex_pass2_main;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f;

	MyVertex *bmp = m_vertices + vertex_bmp;
	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;
	bmp[0] = tmp[0];
	bmp[1] = tmp[1];
	bmp[2] = tmp[2];
	bmp[3] = tmp[3];
	bmp[0].x += m_bmp_fleft * tar_width; bmp[0].y += m_bmp_ftop * tar_height;
	bmp[1] = bmp[0]; bmp[1].x += m_bmp_fwidth * tar_width;
	bmp[3] = bmp[1]; bmp[3].y += m_bmp_fheight* tar_height;
	bmp[2] = bmp[3]; bmp[2].x -= m_bmp_fwidth * tar_width;
	bmp[0].tu = 0; bmp[0].tv = 0;
	bmp[1].tu = (m_bmp_width-1)/4096.0f; bmp[1].tv = 0;
	bmp[2].tu = 0; bmp[2].tv = (m_bmp_height-1)/4096.0f;
	bmp[3].tu = (m_bmp_width-1)/4096.0f; bmp[3].tv = (m_bmp_height-1)/4096.0f;

	tmp = m_vertices + vertex_pass3;
	tmp[0].x = -0.5f; tmp[0].y = -0.5f;
	tmp[1].x = m_active_pp.BackBufferWidth-0.5f; tmp[1].y = -0.5f;
	tmp[2].x = -0.5f; tmp[2].y = m_active_pp.BackBufferHeight-0.5f;
	tmp[3].x = m_active_pp.BackBufferWidth-0.5f; tmp[3].y = m_active_pp.BackBufferHeight-0.5f;

	// second window coordinate
	tar.left = tar.top = 0;
	tar.right = m_active_pp2.BackBufferWidth; tar.bottom = m_active_pp2.BackBufferHeight;
	delta_w = (int)(tar.right - tar.bottom * m_source_aspect + 0.5);
	delta_h = (int)(tar.bottom - tar.right  / m_source_aspect + 0.5);
	if (delta_w > 0)
	{
		tar.left += delta_w/2;
		tar.right -= delta_w/2;
	}
	else if (delta_h > 0)
	{
		tar.top += delta_h/2;
		tar.bottom -= delta_h/2;
	}

	tar_width = tar.right-tar.left;
	tar_height = tar.bottom - tar.top;
	tar.left += (LONG)(tar_width * m_offset_x);
	tar.right += (LONG)(tar_width * m_offset_x);
	tar.top += (LONG)(tar_height * m_offset_y);
	tar.bottom += (LONG)(tar_height * m_offset_y);

	tmp = m_vertices + vertex_pass2_second;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f + 2048.0f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f + 2048.0f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f + 2048.0f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f + 2048.0f;

	// ui zone
	tmp = m_vertices + vertex_ui;
	tmp[0].x = -0.5f; tmp[0].y = m_active_pp.BackBufferHeight-30-0.5f;
	tmp[1].x = m_active_pp.BackBufferWidth-0.5f; tmp[1].y = tmp[0].y;
	tmp[2].x = tmp[0].x; tmp[2].y = tmp[0].y + 30;
	tmp[3].x = tmp[1].x; tmp[3].y = tmp[1].y + 30;
	tmp[0].tu = 0; tmp[0].tv = 0.5f;
	tmp[1].tu = (m_active_pp.BackBufferWidth-1)/4096.0f; tmp[1].tv = 0.5f;
	tmp[2].tu = 0; tmp[2].tv = tmp[0].tv + (30-1)/4096.0f;
	tmp[3].tu = tmp[1].tu; tmp[3].tv = tmp[1].tv + (30-1)/4096.0f;

	if (!g_VertexBuffer)
		return S_FALSE;

	void *pVertices = NULL;
	g_VertexBuffer->Lock( 0, sizeof(m_vertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, m_vertices, sizeof(m_vertices) );
	g_VertexBuffer->Unlock();
	
	return S_OK;
}
HRESULT my12doomRenderer::generate_mask()
{
	if (m_output_mode != masking)
		return S_FALSE;

	if (!g_tex_mask)
		return VFW_E_NOT_CONNECTED;

	CAutoLock lck(&m_frame_lock);
	D3DLOCKED_RECT locked;
	HRESULT hr = g_tex_mask->LockRect(0, &locked, NULL, D3DLOCK_DISCARD);
	if (FAILED(hr)) return hr;

	RECT rect;
	GetWindowRect(m_hWnd, &rect);
	BYTE *dst = (BYTE*) locked.pBits;

	if (m_mask_mode == row_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == line_interlace)
	{
		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memset(dst, (i+rect.top)%2 == 0 ? 0 : 255 ,m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == checkboard_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), m_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	hr = g_tex_mask->UnlockRect(0);

	return hr;
}
HRESULT my12doomRenderer::set_fullscreen(bool full)
{
	D3DDISPLAYMODE d3ddm;
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	m_new_pp.BackBufferFormat       = d3ddm.Format;

	mylog("mode:%dx%d@%dHz, format:%d.\n", d3ddm.Width, d3ddm.Height, d3ddm.RefreshRate, d3ddm.Format);
	if(full && m_active_pp.Windowed)
	{
		GetWindowRect(m_hWnd, &m_window_pos);
		m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
		m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);

		LONG f = m_style & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  m_exstyle & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

		SetWindowLongPtr(m_hWnd, GWL_STYLE, f);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exf);
		if ((DWORD)(LOBYTE(LOWORD(GetVersion()))) < 6)
			SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		m_new_pp.Windowed = FALSE;
		m_new_pp.BackBufferWidth = d3ddm.Width;
		m_new_pp.BackBufferHeight = d3ddm.Height;
		m_new_pp.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}
	else if (!full && !m_active_pp.Windowed)
	{
		m_new_pp.Windowed = TRUE;
		m_new_pp.BackBufferWidth = 0;
		m_new_pp.BackBufferHeight = 0;
		m_new_pp.hDeviceWindow = m_hWnd;
		m_new_pp.FullScreen_RefreshRateInHz = 0;
		SetWindowLongPtr(m_hWnd, GWL_STYLE, m_style);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_exstyle);
		SetWindowPos(m_hWnd, m_exstyle & WS_EX_TOPMOST ? HWND_TOPMOST : HWND_NOTOPMOST,
			m_window_pos.left, m_window_pos.top, m_window_pos.right - m_window_pos.left, m_window_pos.bottom - m_window_pos.top, SWP_FRAMECHANGED);

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}

	return S_OK;
}

HRESULT	my12doomRenderer::BreakConnect()
{
	invalidate_objects();
	return __super::BreakConnect();
}

HRESULT my12doomRenderer::CompleteConnect(IPin *pRecievePin)
{
	set_device_state(need_reset);
	return __super::CompleteConnect(pRecievePin);
}

HRESULT my12doomRenderer::pump()
{
	RECT rect;
	if (m_hWnd)
	{
		GetClientRect(m_hWnd, &rect);
		if (m_active_pp.BackBufferWidth != rect.right-rect.left || m_active_pp.BackBufferHeight != rect.bottom - rect.top)
		{
			m_new_pp.BackBufferWidth = 0;
			m_new_pp.BackBufferHeight = 0;
			set_device_state(need_reset);
		}
	}

	if (m_hWnd2)
	{
		GetClientRect(m_hWnd2, &rect);
		if (m_active_pp2.BackBufferWidth != rect.right-rect.left || m_active_pp2.BackBufferHeight != rect.bottom - rect.top)
			set_device_state(need_reset_object);
	}

	return handle_device_state();
}

HRESULT my12doomRenderer::set_input_layout(int layout)
{
	m_input_layout = (input_layout_types)(layout % input_layout_types_max);
	set_device_state(need_reset);
	handle_device_state();
	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_output_mode(int mode)
{
	m_output_mode = (output_mode_types)(mode % output_mode_types_max);

	// add 3d vision tag at last line
	if (m_output_mode == NV3D)
	{
		CAutoLock lck(&m_device_lock);
		CAutoLock lck2(&m_frame_lock);
		D3DLOCKED_RECT lr;
		RECT lock_tar={0, m_active_pp.BackBufferHeight, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1};
		g_sbs_surface->LockRect(&lr,&lock_tar,0);
		LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
		pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pSIH->dwBPP = 32;
		pSIH->dwFlags = SIH_SIDE_BY_SIDE;
		pSIH->dwWidth = m_active_pp.BackBufferWidth;
		pSIH->dwHeight = m_active_pp.BackBufferHeight;
		g_sbs_surface->UnlockRect();
	}

	generate_mask();
	calculate_vertex();
	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_mode(int mode)
{
	m_mask_mode = (mask_mode_types)(mode % mask_mode_types_max);
	calculate_vertex();
	generate_mask();
	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_color(int id, DWORD color)
{
	if (id == 1)
		m_color1 = color;
	else if (id == 2)
		m_color2 = color;
	else
		return E_INVALIDARG;

	calculate_vertex();
	render(true);
	return S_OK;
}

DWORD my12doomRenderer::get_mask_color(int id)
{
	if (id == 1)
		return m_color1;
	else if (id == 2)
		return m_color2;
	else
		return 0;
}

HRESULT my12doomRenderer::set_swap_eyes(bool swap)
{
	m_swapeyes = swap;

	load_image(true);
	render(true);
	return S_OK;
}

input_layout_types my12doomRenderer::get_input_layout()
{
	return m_input_layout;
}

output_mode_types my12doomRenderer::get_output_mode()
{
	return m_output_mode;
}
mask_mode_types my12doomRenderer::get_mask_mode()
{
	return m_mask_mode;
}

bool my12doomRenderer::get_swap_eyes()
{
	return m_swapeyes;
}

bool my12doomRenderer::get_fullscreen()
{
	return !m_active_pp.Windowed;
}
HRESULT my12doomRenderer::set_offset(int dimention, double offset)		// dimention1 = x, dimention2 = y
{
	if (dimention == 1)
		m_offset_x = offset;
	else if (dimention == 2)
		m_offset_y = offset;
	else
		return E_INVALIDARG;

	calculate_vertex();
	render(true);
	return S_OK;
}
HRESULT my12doomRenderer::set_aspect(double aspect)
{
	m_source_aspect = aspect;
	calculate_vertex();
	load_image(true);
	render(true);
	return S_OK;
}
double my12doomRenderer::get_offset(int dimention)
{
	if (dimention == 1)
		return m_offset_x;
	else if (dimention == 2)
		return m_offset_y;
	else
		return 0.0;
}
double my12doomRenderer::get_aspect()
{
	return m_source_aspect;
}

HRESULT my12doomRenderer::set_window(HWND wnd, HWND wnd2)
{
	shutdown();
	m_hWnd = wnd;
	m_hWnd2 = wnd2;
	handle_device_state();
	return S_OK;
}

HRESULT my12doomRenderer::repaint_video()
{
	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_bmp(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop)
{
	if (g_tex_bmp == NULL)
		return VFW_E_WRONG_STATE;

	CAutoLock lck(&m_device_lock);
	CAutoLock lck2(&m_frame_lock);

	bool should_recalculate_vertex = true;
	if (data == NULL)
	{
		if (m_bmp_fleft < -9990.0 && m_bmp_ftop < -9990.0)
			should_recalculate_vertex = false;
		m_bmp_fleft = m_bmp_ftop = -9999.0;
		m_bmp_fwidth = m_bmp_fheight = 1.0;
		m_bmp_width = m_bmp_height = 4096;
	}
	else
	{
		RECT lock_rect = {0, 0, width, height};
		D3DLOCKED_RECT locked;
		HRESULT hr = g_tex_bmp->LockRect(0, &locked, &lock_rect, NULL);
		if (FAILED(hr))
			return hr;		// device might be lost

		m_bmp_fleft = fleft;
		m_bmp_ftop = ftop;
		m_bmp_fwidth = fwidth;
		m_bmp_fheight = fheight;
		m_bmp_width = width;
		m_bmp_height = height;


		BYTE *src = (BYTE*)data;
		BYTE *dst = (BYTE*) locked.pBits;
		for(int y=0; y<height; y++)
		{
			memcpy(dst, src, width*4);
			dst += locked.Pitch;
			src += width*4;
		}
		g_tex_bmp->UnlockRect(0);
	}

	if (should_recalculate_vertex)calculate_vertex();
	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_ui(void* data, int pitch)
{
	if (g_tex_bmp == NULL)
		return VFW_E_WRONG_STATE;

	CAutoLock lck(&m_device_lock);
	CAutoLock lck2(&m_frame_lock);

	bool should_recalculate_vertex = true;
	if (data == NULL)
	{
		m_showui = false;
	}
	else
	{
		int width = m_active_pp.BackBufferWidth;
		int height = 30;

		RECT lock_rect = {0, 2048, width, 2048+height};
		D3DLOCKED_RECT locked;
		HRESULT hr = g_tex_bmp->LockRect(0, &locked, &lock_rect, NULL);
		if (FAILED(hr))
			return hr;		// device might be lost

		BYTE *src = (BYTE*)data;
		BYTE *dst = (BYTE*) locked.pBits;
		for(int y=0; y<height; y++)
		{
			memcpy(dst, src, width*4);
			dst += locked.Pitch;
			src += pitch;
		}
		g_tex_bmp->UnlockRect(0);
		m_showui = true;
	}

	render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_ui_visible(bool visible)
{
	if (m_showui != visible)
	{
		m_showui = visible;
		render(true);
	}

	m_showui = visible;
	return S_OK;
}
