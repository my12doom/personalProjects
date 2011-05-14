// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include "PixelShaders/ps.h"
#include "3dvideo.h"


struct MyVertex
{
	float x , y, z;
	float w;
	DWORD diffuse;
	DWORD specular;
	float tu, tv;
};
const DWORD FVF_Flags = D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR;

extern MyVertex g_myVertices[28];/* =
{
	//     x      y     z     w   	diffuse					specular	 			  tu   tv
	// pass1 whole
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},
	// pass1 left
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.5f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.5f,1.0f},
	// pass1 right
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.5f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.5f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},
	// pass1 top
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.5f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.5f},
	// pass1 bottom
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.5f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.5f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},

	// pass2 whole texture, fixed aspect output for main back buffer
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},

	// pass2 whole texture, fixed aspect output for second back buffer
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},

	// pass3 whole texture, whole back buffer output
	{-1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,0.0f},
	{ 1.0f, 1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,0.0f},
	{-1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  0.0f,1.0f},
	{ 1.0f,-1.0f, 0.0f, 1.0f, D3DCOLOR_XRGB(255,0,0), D3DCOLOR_XRGB(0,255,255),  1.0f,1.0f},
};*/
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
};

extern enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, dual_window, out_side_by_side, out_top_bottom, output_mode_types_max
} output_mode;// = NV3D;

extern enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max
} input_layout;// = side_by_side;
extern bool g_swapeyes;// = false;

extern enum mask_mode_types
{
	row_interlace, line_interlace, checkboard_interlace, mask_mode_types_max
} mask_mode;// = row_interlace;

HRESULT my12doomRenderer::handle_device_state()							//handle device create/recreate/lost/reset
{
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
		HRESULT hr = g_pd3dDevice->Reset( &g_new_pp );
		if( FAILED(hr ) )
		{
			m_device_state = device_lost;
			return hr;
		}
		restore_objects();
		m_device_state = fine;
		g_active_pp = g_new_pp;
	}

	else if (m_device_state == device_lost)
	{
		Sleep(100);
		if( g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
		{
			CAutoLock lck(&m_device_lock);
			invalidate_objects();
			HRESULT hr = g_pd3dDevice->Reset( &g_new_pp );

			if( FAILED(hr ) )
			{
				m_device_state = device_lost;
				return hr;
			}
			g_active_pp = g_new_pp;
			restore_objects();
			
			m_device_state = fine;
			g_active_pp = g_new_pp;

			return hr;
		}

		else
			return E_FAIL;
	}

	else if (m_device_state == need_create)
	{
		timeBeginPeriod(1);
		g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );

		ZeroMemory( &g_active_pp, sizeof(g_active_pp) );
		g_active_pp.Windowed               = TRUE;
		g_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
		g_active_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;
		g_active_pp.BackBufferCount = 1;
		g_active_pp.Flags = D3DPRESENTFLAG_VIDEO;

		g_style = GetWindowLongPtr(g_hWnd, GWL_STYLE);
		g_exstyle = GetWindowLongPtr(g_hWnd, GWL_EXSTYLE);
		GetWindowRect(g_hWnd, &g_window_pos);

		set_fullscreen(false);

		UINT AdapterToUse=D3DADAPTER_DEFAULT;
		D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;
		for (UINT Adapter=0;Adapter<g_pD3D->GetAdapterCount();Adapter++)
		{ 
			D3DADAPTER_IDENTIFIER9  Identifier; 
			HRESULT       Res; 

			Res = g_pD3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
			if (strstr(Identifier.Description,"PerfHUD") != 0) 
			{ 
				AdapterToUse=Adapter; 
				DeviceType=D3DDEVTYPE_REF; 
				break; 
			}
		}
		HRESULT hr = g_pD3D->CreateDevice( AdapterToUse, DeviceType,
			g_hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&g_active_pp, &g_pd3dDevice );

		if (FAILED(hr))
		{
			m_device_state = create_failed;
			return hr;
		}
		g_new_pp = g_active_pp;
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
	g_pd3dDevice = NULL;
    g_pD3D = NULL;

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
	g_ps_yv12_to_rgb = NULL;
	g_tex_mask = NULL;
	g_tex_rgb_left = NULL;
	g_tex_rgb_right = NULL;
	g_mask_temp_left = NULL;
	g_mask_temp_right = NULL;
	g_sbs_surface = NULL;
	g_tex_Y = NULL;
	g_tex_V = NULL;
	g_tex_U = NULL;
	g_VertexBuffer = NULL;

	g_swap1 = NULL;
	g_swap2 = NULL;

	return S_OK;
}
HRESULT my12doomRenderer::restore_objects()
{
	g_temp_width = m_lVidWidth;
	g_temp_height = m_lVidHeight;
	if (input_layout == side_by_side)
		g_temp_width /= 2;
	else if (input_layout == top_bottom)
		g_temp_height /= 2;


	HRESULT hr = g_pd3dDevice->GetSwapChain(0, &g_swap1);
	hr = g_pd3dDevice->CreateAdditionalSwapChain(&g_active_pp2, &g_swap2);

	hr = g_pd3dDevice->CreateTexture(m_lVidWidth, m_lVidHeight, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_Y, NULL);
	hr = g_pd3dDevice->CreateTexture(m_lVidWidth/2, m_lVidHeight/2, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_U, NULL);	    
	hr = g_pd3dDevice->CreateTexture(m_lVidWidth/2, m_lVidHeight/2, 1, NULL, D3DFMT_L8,D3DPOOL_MANAGED,	&g_tex_V, NULL);
	hr = g_pd3dDevice->CreateTexture(g_temp_width, g_temp_height, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_left, NULL);
	hr = g_pd3dDevice->CreateTexture(g_temp_width, g_temp_height, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_tex_rgb_right, NULL);
	hr = g_pd3dDevice->CreateTexture(g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_mask_temp_left, NULL);
	hr = g_pd3dDevice->CreateTexture(g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, g_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &g_mask_temp_right, NULL);
	hr = g_pd3dDevice->CreateTexture(g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &g_tex_mask, NULL);
	hr = g_pd3dDevice->CreateRenderTarget(g_active_pp.BackBufferWidth*2, g_active_pp.BackBufferHeight+1, g_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &g_sbs_surface, NULL);


	generate_mask();


	// add 3d vision tag at last line
	D3DLOCKED_RECT lr;
	RECT lock_tar={0, g_active_pp.BackBufferHeight, g_active_pp.BackBufferWidth*2, g_active_pp.BackBufferHeight+1};
	g_sbs_surface->LockRect(&lr,&lock_tar,0);
	LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
	pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
	pSIH->dwBPP = 32;
	pSIH->dwFlags = SIH_SIDE_BY_SIDE;
	pSIH->dwWidth = g_active_pp.BackBufferWidth;
	pSIH->dwHeight = g_active_pp.BackBufferHeight;
	g_sbs_surface->UnlockRect();


	// vertex
	g_pd3dDevice->CreateVertexBuffer( sizeof(g_myVertices), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, FVF_Flags, D3DPOOL_DEFAULT, &g_VertexBuffer, NULL );
	calculate_vertex();

	// pixel shader
	hr = g_pd3dDevice->CreatePixelShader(g_code_planar_rgb, &g_ps_yv12_to_rgb);

	// Create the pixel shader
	if (FAILED(hr)) return E_FAIL;

	load_image(true);


	// create render thread
	create_render_thread();
	return S_OK;
}
HRESULT my12doomRenderer::render_nolock(bool forced)
{

	// device state check and restore
	if (FAILED(handle_reset()))
		return E_FAIL;


	// image loading and idle check
	if (load_image() != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!g_pd3dDevice)
		return E_FAIL;

	static int last_render_time = timeGetTime();

	HRESULT hr;

	CAutoLock lck(&m_frame_lock);
	// pass 2: drawing to back buffer!
	hr = g_pd3dDevice->BeginScene();
	hr = g_pd3dDevice->SetPixelShader(NULL);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);

	// set render target to back buffer
	CComPtr<IDirect3DSurface9> back_buffer;
	hr = g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);
	hr = g_pd3dDevice->SetRenderTarget(0, back_buffer);

	// back ground
	#ifdef DEBUG
	g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255,128,0), 1.0f, 0L );// debug: orange background
	#else
	g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );  // black background
	#endif

	calculate_vertex();
	// relock vertex
	/*
	void *pVertices = NULL;
	g_pVertexBuffer->Lock( 0, sizeof(g_myVertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, g_myVertices, sizeof(g_myVertices) );
	g_pVertexBuffer->Unlock();
	*/

	// reset texture blending stages
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_RESULTARG, D3DTA_CURRENT);
	g_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
	g_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	g_pd3dDevice->SetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, 0 );
	g_pd3dDevice->SetTextureStageState( 2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
	g_pd3dDevice->SetTextureStageState( 2, D3DTSS_TEXCOORDINDEX, 0 );

	//else if (output_mode == mono)
	{
		g_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		g_pd3dDevice->SetTexture( 0, g_tex_rgb_left );

		hr = g_pd3dDevice->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
		hr = g_pd3dDevice->SetFVF( FVF_Flags );
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass2_main, 2 );
	}

	g_pd3dDevice->EndScene();

	if ((hr = g_pd3dDevice->Present( NULL, NULL, NULL, NULL )) ==  D3DERR_DEVICELOST)
		set_device_state(device_lost);


	char tmp[256];
	static int t = timeGetTime();
	sprintf(tmp, "%d\n", timeGetTime()-t);
	if (timeGetTime()-t > 18)
		OutputDebugStringA("lost sync\n");
	t = timeGetTime();
	OutputDebugStringA(tmp);

	return S_OK;}
HRESULT my12doomRenderer::render(bool forced)
{
	CAutoLock lck(&m_device_lock);
	if (g_pd3dDevice)
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
		if (output_mode != pageflipping)
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
	if (!g_pd3dDevice || !g_tex_rgb_left ||!g_tex_rgb_right)
		return E_FAIL;
	HRESULT hr;
	// loading YV12 image as three L8 texture
	{
		CAutoLock lck(&m_data_lock);
		if (!forced && !m_data_changed)
			return S_FALSE;

		m_data_changed = false;

		D3DLOCKED_RECT d3dlr;
		BYTE * src = m_data;
		BYTE * dst;

		if (!g_tex_Y || !g_tex_U ||!g_tex_V)
			return E_FAIL;

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
		// black level test
		// memset(d3dlr.pBits, 0, d3dlr.Pitch * 100);
		g_tex_Y->UnlockRect(0);

		// load V
		if( FAILED(hr = g_tex_V->LockRect(0, &d3dlr, 0, NULL)))
			return hr;
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_lVidHeight/2; i++)
		{
			memcpy(dst, src, m_lVidWidth/2);
			src += m_lVidWidth/2;
			dst += d3dlr.Pitch;
		}
		// black level test
		// memset(d3dlr.pBits, 128, d3dlr.Pitch * 50);
		g_tex_V->UnlockRect(0);

		// load U
		if( FAILED(hr = g_tex_U->LockRect(0, &d3dlr, 0, NULL)))
			return hr;
		dst = (BYTE*)d3dlr.pBits;
		for(int i=0; i<m_lVidHeight/2; i++)
		{
			memcpy(dst, src, m_lVidWidth/2);
			src += m_lVidWidth/2;
			dst += d3dlr.Pitch;
		}
		// black level test
		//memset(d3dlr.pBits, 128, d3dlr.Pitch * 50);
		g_tex_U->UnlockRect(0);
	}


	// pass 1: render full resolution to two seperate texture
	hr = g_pd3dDevice->BeginScene();
	hr = g_pd3dDevice->SetPixelShader(g_ps_yv12_to_rgb);
	hr = g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	CComPtr<IDirect3DSurface9> left_surface;
	CComPtr<IDirect3DSurface9> right_surface;
	hr = g_tex_rgb_left->GetSurfaceLevel(0, &left_surface);
	hr = g_tex_rgb_right->GetSurfaceLevel(0, &right_surface);
	if (!g_swapeyes)
		hr = g_pd3dDevice->SetRenderTarget(0, left_surface);
	else
		hr = g_pd3dDevice->SetRenderTarget(0, right_surface);


	// drawing
	hr = g_pd3dDevice->SetTexture( 0, g_tex_Y );
	hr = g_pd3dDevice->SetTexture( 1, g_tex_U );
	hr = g_pd3dDevice->SetTexture( 2, g_tex_V );
	hr = g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	hr = g_pd3dDevice->SetStreamSource( 0, g_VertexBuffer, 0, sizeof(MyVertex) );
	hr = g_pd3dDevice->SetFVF( FVF_Flags );

	// reload vertex to avoid buffer miss
	void *pVertices = NULL;
	g_VertexBuffer->Lock( 0, sizeof(g_myVertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, g_myVertices, sizeof(g_myVertices) );
	g_VertexBuffer->Unlock();

	if (input_layout == side_by_side)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_left, 2 );
	else if (input_layout == top_bottom)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_top, 2 );
	else if (input_layout == mono2d)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );

	if (!g_swapeyes)
		hr = g_pd3dDevice->SetRenderTarget(0, right_surface);
	else
		hr = g_pd3dDevice->SetRenderTarget(0, left_surface);

	if (input_layout == side_by_side)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_right, 2 );
	else if (input_layout == top_bottom)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_bottom, 2 );
	else if (input_layout == mono2d)
		hr = g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, vertex_pass1_whole, 2 );


	hr = g_pd3dDevice->EndScene();
	return S_OK;
}
HRESULT my12doomRenderer::calculate_vertex()
{
	CAutoLock lck(&m_frame_lock);
	// w,color
	for(int i=0; i<sizeof(g_myVertices) / sizeof(MyVertex); i++)
	{
		g_myVertices[i].diffuse = m_color1;
		g_myVertices[i].specular = m_color2;
		g_myVertices[i].w = 1.0f;
	}
	// texture coordinate
	// pass1 whole
	g_myVertices[0].tu = 0.0f; g_myVertices[0].tv = 0.0f;
	g_myVertices[1].tu = 1.0f; g_myVertices[1].tv = 0.0f;
	g_myVertices[2].tu = 0.0f; g_myVertices[2].tv = 1.0f;
	g_myVertices[3].tu = 1.0f; g_myVertices[3].tv = 1.0f;

	// pass1 left
	g_myVertices[4].tu = 0.0f; g_myVertices[4].tv = 0.0f;
	g_myVertices[5].tu = 0.5f; g_myVertices[5].tv = 0.0f;
	g_myVertices[6].tu = 0.0f; g_myVertices[6].tv = 1.0f;
	g_myVertices[7].tu = 0.5f; g_myVertices[7].tv = 1.0f;

	// pass1 right
	g_myVertices[8].tu = 0.5f; g_myVertices[8].tv = 0.0f;
	g_myVertices[9].tu = 1.0f; g_myVertices[9].tv = 0.0f;
	g_myVertices[10].tu = 0.5f; g_myVertices[10].tv = 1.0f;
	g_myVertices[11].tu = 1.0f; g_myVertices[11].tv = 1.0f;

	// pass1 top
	g_myVertices[12].tu = 0.0f; g_myVertices[12].tv = 0.0f;
	g_myVertices[13].tu = 1.0f; g_myVertices[13].tv = 0.0f;
	g_myVertices[14].tu = 0.0f; g_myVertices[14].tv = 0.5f;
	g_myVertices[15].tu = 1.0f; g_myVertices[15].tv = 0.5f;

	// pass1 bottom
	g_myVertices[16].tu = 0.0f; g_myVertices[16].tv = 0.5f;
	g_myVertices[17].tu = 1.0f; g_myVertices[17].tv = 0.5f;
	g_myVertices[18].tu = 0.0f; g_myVertices[18].tv = 1.0f;
	g_myVertices[19].tu = 1.0f; g_myVertices[19].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for main back buffer
	g_myVertices[20].tu = 0.0f; g_myVertices[20].tv = 0.0f;
	g_myVertices[21].tu = 1.0f; g_myVertices[21].tv = 0.0f;
	g_myVertices[22].tu = 0.0f; g_myVertices[22].tv = 1.0f;
	g_myVertices[23].tu = 1.0f; g_myVertices[23].tv = 1.0f;

	// pass2 whole texture, fixed aspect output for second back buffer
	g_myVertices[24].tu = 0.0f; g_myVertices[20].tv = 0.0f;
	g_myVertices[25].tu = 1.0f; g_myVertices[21].tv = 0.0f;
	g_myVertices[26].tu = 0.0f; g_myVertices[22].tv = 1.0f;
	g_myVertices[27].tu = 1.0f; g_myVertices[23].tv = 1.0f;

	// pass3 whole texture, whole back buffer output
	g_myVertices[28].tu = 0.0f; g_myVertices[28].tv = 0.0f;
	g_myVertices[29].tu = 1.0f; g_myVertices[29].tv = 0.0f;
	g_myVertices[30].tu = 0.0f; g_myVertices[30].tv = 1.0f;
	g_myVertices[31].tu = 1.0f; g_myVertices[31].tv = 1.0f;

	// pass1 coordinate
	for(int i=0; i<vertex_pass1_types_count; i++)
	{
		MyVertex * tmp = g_myVertices + vertex_pass1_whole + vertex_point_per_type * i ;
		tmp[0].x = -0.5f; tmp[0].y = -0.5f;
		tmp[1].x = g_temp_width-0.5f; tmp[1].y = -0.5f;
		tmp[2].x = -0.5f; tmp[2].y = g_temp_height-0.5f;
		tmp[3].x =  g_temp_width-0.5f; tmp[3].y =g_temp_height-0.5f;
	}

	// pass2-3 coordinate
	// main window coordinate
	RECT tar = {0,0, g_active_pp.BackBufferWidth, g_active_pp.BackBufferHeight};
	if (output_mode == out_side_by_side)
		tar.right /= 2;
	else if (output_mode == out_top_bottom)
		tar.bottom /= 2;


	double source_aspect = (double)m_lVidWidth / m_lVidHeight;
	if (source_aspect> 2.425)
		source_aspect /= 2;
	else if (source_aspect< 1.2125)
		source_aspect *= 2;

	double offset_x = -0.0;
	double offset_y = 0.0;

	int delta_w = (int)(tar.right - tar.bottom * source_aspect + 0.5);
	int delta_h = (int)(tar.bottom - tar.right  / source_aspect + 0.5);
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
	tar.left += (LONG)(tar_width * offset_x);
	tar.right += (LONG)(tar_width * offset_x);
	tar.top += (LONG)(tar_height * offset_y);
	tar.bottom += (LONG)(tar_height * offset_y);

	MyVertex *tmp = g_myVertices + vertex_pass2_main;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f;

	tmp = g_myVertices + vertex_pass3;
	tmp[0].x = -0.5f; tmp[0].y = -0.5f;
	tmp[1].x = g_active_pp.BackBufferWidth-0.5f; tmp[1].y = -0.5f;
	tmp[2].x = -0.5f; tmp[2].y = g_active_pp.BackBufferHeight-0.5f;
	tmp[3].x = g_active_pp.BackBufferWidth-0.5f; tmp[3].y = g_active_pp.BackBufferHeight-0.5f;

	// second window coordinate
	tar.left = tar.top = 0;
	tar.right = g_active_pp2.BackBufferWidth; tar.bottom = g_active_pp2.BackBufferHeight;
	delta_w = (int)(tar.right - tar.bottom * source_aspect + 0.5);
	delta_h = (int)(tar.bottom - tar.right  / source_aspect + 0.5);
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
	tar.left += (LONG)(tar_width * offset_x);
	tar.right += (LONG)(tar_width * offset_x);
	tar.top += (LONG)(tar_height * offset_y);
	tar.bottom += (LONG)(tar_height * offset_y);

	tmp = g_myVertices + vertex_pass2_second;
	tmp[0].x = tar.left-0.5f; tmp[0].y = tar.top-0.5f;
	tmp[1].x = tar.right-0.5f; tmp[1].y = tar.top-0.5f;
	tmp[2].x = tar.left-0.5f; tmp[2].y = tar.bottom-0.5f;
	tmp[3].x = tar.right-0.5f; tmp[3].y = tar.bottom-0.5f;

	void *pVertices = NULL;
	g_VertexBuffer->Lock( 0, sizeof(g_myVertices), (void**)&pVertices, D3DLOCK_DISCARD );
	memcpy( pVertices, g_myVertices, sizeof(g_myVertices) );
	g_VertexBuffer->Unlock();	return S_OK;
}
HRESULT my12doomRenderer::generate_mask()
{
	if (output_mode != masking)
		return S_FALSE;

	if (!g_tex_mask)
		return VFW_E_NOT_CONNECTED;

	CAutoLock lck(&m_frame_lock);
	D3DLOCKED_RECT locked;
	HRESULT hr = g_tex_mask->LockRect(0, &locked, NULL, D3DLOCK_DISCARD);
	if (FAILED(hr)) return hr;

	RECT rect;
	GetWindowRect(g_hWnd, &rect);
	BYTE *dst = (BYTE*) locked.pBits;

	if (mask_mode == row_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (mask_mode == line_interlace)
	{
		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memset(dst, (i+rect.top)%2 == 0 ? 0 : 255 ,g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	else if (mask_mode == checkboard_interlace)
	{
		// init row mask texture
		BYTE one_line[4096];
		for(DWORD i=0; i<4096; i++)
			one_line[i] = (i+rect.left)%2 == 0 ? 0 : 255;

		for(DWORD i=0; i<g_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), g_active_pp.BackBufferWidth);
			dst += locked.Pitch;
		}
	}
	hr = g_tex_mask->UnlockRect(0);

	return hr;
}
HRESULT my12doomRenderer::set_fullscreen(bool full)
{
	D3DDISPLAYMODE d3ddm;
	g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	g_new_pp.BackBufferFormat       = d3ddm.Format;
	if(full)
	{
		GetWindowRect(g_hWnd, &g_window_pos);
		g_style = GetWindowLongPtr(g_hWnd, GWL_STYLE);
		g_exstyle = GetWindowLongPtr(g_hWnd, GWL_EXSTYLE);

		LONG f = g_style & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  g_exstyle & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

		SetWindowLongPtr(g_hWnd, GWL_STYLE, f);
		SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, exf);
		if ((DWORD)(LOBYTE(LOWORD(GetVersion()))) < 6)
			SendMessage(g_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		g_new_pp.Windowed = FALSE;
		g_new_pp.BackBufferWidth = d3ddm.Width;
		g_new_pp.BackBufferHeight = d3ddm.Height;
		g_new_pp.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
	}
	else
	{
		g_new_pp.Windowed = TRUE;
		g_new_pp.BackBufferWidth = 0;
		g_new_pp.BackBufferHeight = 0;
		g_new_pp.hDeviceWindow = g_hWnd;
		g_new_pp.FullScreen_RefreshRateInHz = 0;
		SetWindowLongPtr(g_hWnd, GWL_STYLE, g_style);
		SetWindowLongPtr(g_hWnd, GWL_EXSTYLE, g_exstyle);
		SetWindowPos(g_hWnd, g_exstyle & WS_EX_TOPMOST ? HWND_TOPMOST : HWND_NOTOPMOST,
			g_window_pos.left, g_window_pos.top, g_window_pos.right - g_window_pos.left, g_window_pos.bottom - g_window_pos.top, SWP_FRAMECHANGED);
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
	HRESULT hr = __super::CompleteConnect(pRecievePin);
	set_device_state(need_reset);
	handle_device_state();

	return hr;
}
