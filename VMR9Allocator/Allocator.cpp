//------------------------------------------------------------------------------
// File: Allocator.cpp
//
// Desc: DirectShow sample code - implementation of the CAllocator class
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "util.h"
#include "Allocator.h"


void DoEvents()
{
	MSG msg;

	while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		if ( ::GetMessage(&msg, NULL, 0, 0))
		{
			::TranslateMessage(&msg);
			:: DispatchMessage(&msg);
		}
		else
			break;
	}
}

inline unsigned __int64 GetCycleCount()
{
	__asm	_emit	0x0F
		__asm	_emit	0x31
}

inline double CycleTime()
{
	return (double)GetCycleCount() / 3200000000;
}


HRESULT mylog(wchar_t *format, ...)
{
	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfW(tmp, format, valist);
	va_end(valist);

	wsprintfW(tmp, L"%s(tid=%d)", tmp, GetCurrentThreadId());
	OutputDebugStringW(tmp);
	return S_OK;
}


HRESULT mylog(const char *format, ...)
{
	char tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfA(tmp, format, valist);
	va_end(valist);

	wsprintfA(tmp, "%s(tid=%d)", tmp, GetCurrentThreadId());
	OutputDebugStringA(tmp);
	return S_OK;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CAllocator::CAllocator(HRESULT& hr, HWND wnd, IDirect3D9* d3d, IDirect3DDevice9* d3dd)
: m_refCount(1)
, m_D3D(d3d)
, m_D3DDev(d3dd)
, m_window( wnd )
, m_work_exit(false)
, m_pp_inited(false)
{
    CAutoLock Lock(&m_ObjectLock);
    hr = E_FAIL;

	m_reseting = false;
	m_dshow_presenting = 0;
	m_work_exit = false;
	m_pending_reset = false;
	m_worker_thread = CreateThread(0,0,worker_thread, this, NULL, NULL);

	if( IsWindow( wnd ) == FALSE )
    {
        hr = E_INVALIDARG;
        return;
    }

    if( m_D3D == NULL )
    {
        ASSERT( d3dd ==  NULL ); 

        m_D3D.Attach( Direct3DCreate9(D3D_SDK_VERSION) );
        if (m_D3D == NULL) {
            hr = E_FAIL;
            return;
        }
    }

	m_last_present_result = S_OK;
	m_use_work_thread = false;
    if( m_D3DDev == NULL )
    {
        hr = CreateDevice();
    }
	m_use_work_thread = true;
}

CAllocator::~CAllocator()
{
	m_work_exit = true;
	while (WaitForSingleObject(m_worker_thread, 0) == WAIT_TIMEOUT)
		Pump();

    DeleteSurfaces();
	m_use_work_thread = false;
	Destroy();
}


DWORD WINAPI CAllocator::worker_thread(LPVOID lpParame)
{
	CAllocator *_this = (CAllocator*) lpParame;
	CAutoPtr<work> p;

	while (!_this->m_work_exit)
	{
		//_this->Present();
		Sleep(1);
	}
	return 0;

	/*
sleep:
	SuspendThread(GetCurrentThread());
	if (_this->m_work_exit)
		return 0;
next:
	{
		CAutoLock queuelock(&(_this->m_work_sec));
		if (_this->m_works.IsEmpty())
			goto sleep;
		p = _this->m_works.RemoveHead();
	}

	if (p->command == CREATE)
	{
		*(HRESULT*)p->out =	_this->m_D3D->CreateDevice(  D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			_this->m_window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING | 
			D3DCREATE_MULTITHREADED,
			&_this->pp,
			&_this->m_D3DDev);

	}
	else if (p->command == TestCooperateLevel)
	{
		*(HRESULT*)p->out = _this->m_D3DDev->TestCooperativeLevel();
	}
	else if (p->command == DESTROY)
	{
		_this->m_D3DDev = NULL;
		*(HRESULT*)p->out = S_OK;

		OutputDebugStringA("D3DDevice Destroyed.\n");
	}
	else if (p->command == RESET)
	{
		*(HRESULT*)p->out = _this->m_D3DDev->Reset(&_this->pp);
	}
	SetEvent(p->complete);
	goto next;

	*/
	return 0;
}

HRESULT CAllocator::Present()
{
	//if (should_direct_render())
	//	return S_OK;
	if (m_drawing)
		return S_FALSE;

	char tmp[256];
	double lock=CycleTime();
	sprintf(tmp, "(%d,%f)LockTime:%f,", GetCurrentThreadId(), CycleTime(), CycleTime()-lock);
	mylog(tmp);

	HRESULT hr = S_FALSE;
	m_device_sec.Lock();
	if (m_D3DDev == NULL)
	{
		m_device_sec.Unlock();
	}
	else
	{
		{
			double time1 = CycleTime();
			//m_D3DDev->BeginScene();
			double time2 = CycleTime();
			{
			CAutoLock lck(&m_image_sec);
			//m_scene.draw_to_my_surface(m_D3DDev, m_texture_to_draw, pp.Windowed);
			m_scene.draw_to_my_surface(m_D3DDev, m_privateTexture, pp.Windowed);
			}
			m_scene.copy_to_back_buffer(m_D3DDev);
			double time3 = CycleTime();
			//m_D3DDev->EndScene();
			double time4 = CycleTime();
			
			hr = m_D3DDev->Present(NULL, NULL, NULL, NULL);
			
			double time5 = CycleTime();

			sprintf(tmp, "%f,%f,%f,%f\n", time2-time1, time3-time2, time4-time3, time5-time4);
			OutputDebugStringA(tmp);
		}
		
		m_device_sec.Unlock();
	}

	return hr;
}

HRESULT CAllocator::Pump()
{
	CAutoPtr<work> p;
	{
		CAutoLock queuelock(&m_work_sec);
		if (!m_works.IsEmpty())
			p = m_works.RemoveHead();
	}

	if (p == NULL)
	{
		return S_FALSE;
	}

	{
		CAutoLock lck(&m_device_sec);

		if (p->command == CREATE)
		{
			m_D3DDev = NULL;
			*(HRESULT*)p->out =	m_D3D->CreateDevice(  D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL,
				m_window,
				D3DCREATE_HARDWARE_VERTEXPROCESSING | 
				D3DCREATE_MULTITHREADED,
				&pp,
				&m_D3DDev);
			OutputDebugStringA("D3DDevice Created.\n");

		}
		else if (p->command == TestCooperateLevel)
		{
			*(HRESULT*)p->out = m_D3DDev->TestCooperativeLevel();
		}
		else if (p->command == DESTROY)
		{
			m_D3DDev = NULL;
			*(HRESULT*)p->out = S_OK;

			OutputDebugStringA("D3DDevice Destroyed.\n");
		}
		else if (p->command == RESET)
		{
			*(HRESULT*)p->out = m_D3DDev->Reset(&pp);
		}
		SetEvent(p->complete);
	}

	return S_OK;
}
HRESULT CAllocator::TestCopperativelLevel()
{
	if (!m_use_work_thread)
		return m_D3DDev->TestCooperativeLevel();

	HRESULT hr;
	HANDLE complete  = CreateEvent(NULL, TRUE, FALSE, NULL);
	{
		CAutoPtr<work> w;
		w.Attach(new work);
		w->command = TestCooperateLevel;
		w->complete = complete;
		w->out = &hr;
		CAutoLock lck(&m_work_sec);
		m_works.AddTail(w);
	}
	WaitForSingleObject(complete, INFINITE);

	return hr;
}

HRESULT CAllocator::Destroy()
{
	char tmp[256];sprintf(tmp, "Destroying From thread %d\n", GetCurrentThreadId());OutputDebugStringA(tmp);
	if (!m_use_work_thread)
	{
		CAutoLock lck(&m_device_sec);
		m_D3DDev = NULL;
		return S_OK;
	}
	HRESULT hr;
	HANDLE complete  = CreateEvent(NULL, TRUE, FALSE, NULL);
	{
		CAutoPtr<work> w;
		w.Attach(new work);
		w->command = DESTROY;
		w->complete = complete;
		w->out = &hr;
		CAutoLock lck(&m_work_sec);
		m_works.AddTail(w);
	}
	WaitForSingleObject(complete, INFINITE);

	return hr;
}


HRESULT CAllocator::Reset()
{
	if (!m_use_work_thread)
	{
		CAutoLock lck(&m_device_sec);
		return m_D3DDev->Reset(&pp);
	}

	HRESULT hr;
	HANDLE complete  = CreateEvent(NULL, TRUE, FALSE, NULL);
	{
		CAutoPtr<work> w;
		w.Attach(new work);
		w->command = RESET;
		w->complete = complete;
		w->out = &hr;
		CAutoLock lck(&m_work_sec);
		m_works.AddTail(w);
	}
	WaitForSingleObject(complete, INFINITE);

	return hr;
}
HRESULT CAllocator::CreateDevice()
{
	char tmp[256];sprintf(tmp, "Creating From thread %d\n", GetCurrentThreadId());OutputDebugStringA(tmp);


	HRESULT hr;
	D3DDISPLAYMODE dm;
    hr = m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &dm);
    //D3DPRESENT_PARAMETERS pp;
	if (!m_pp_inited)
	{
		m_pp_inited = true;
		ZeroMemory(&pp, sizeof(pp));
		pp.Windowed = TRUE;
		pp.hDeviceWindow = m_window;
		pp.SwapEffect = D3DSWAPEFFECT_COPY;
		pp.BackBufferFormat = dm.Format;

		// 3d vision
		//pp.Windowed = FALSE;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
		pp.BackBufferFormat = D3DFMT_A8R8G8B8;  // set the back buffer format to 32 bit
		pp.BackBufferWidth = 1680;
		pp.BackBufferHeight = 1050;
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		pp.BackBufferCount = 1;

	}

	if (!m_use_work_thread)
	{
		CAutoLock lck(&m_device_sec);
		m_D3DDev = NULL;
		FAIL_RET( m_D3D->CreateDevice(  D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			m_window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&pp,
			&m_D3DDev.p) );
	}
	else
	{
		hr = E_FAIL;
		HANDLE complete  = CreateEvent(NULL, TRUE, FALSE, NULL);
		{
			CAutoPtr<work> w;
			w.Attach(new work);
			w->command = CREATE;
			w->complete = complete;
			w->out = &hr;
			CAutoLock lck(&m_work_sec);
			m_works.AddTail(w);
		}
		WaitForSingleObject(complete, INFINITE);
		FAIL_RET(hr);
	}

	// NV3D part
	StereoHandle h3d;
	NvAPI_Status res;
	res = NvAPI_Stereo_CreateHandleFromIUnknown(m_D3DDev, &h3d);
	res = NvAPI_Stereo_Activate(h3d);

	if (res == NVAPI_OK)
	{
		nv3d_actived = true;
	}

	res = NvAPI_Stereo_SetNotificationMessage(h3d, (NvU64)m_window, NV_NOTIFY);
	res = NvAPI_Stereo_DestroyHandle(h3d);

	m_renderTarget = NULL; 
	hr = m_D3DDev->GetRenderTarget( 0, & m_renderTarget.p );
	return hr;
}


void CAllocator::DeleteSurfaces()
{
    CAutoLock Lock(&m_ObjectLock);
	CAutoLock lck(&m_device_sec);

    // clear out the private texture
    m_privateTexture = NULL;
	m_renderTarget = NULL;
	m_texture_to_draw = NULL;

    for( size_t i = 0; i < m_surfaces.size(); ++i ) 
    {
        m_surfaces[i] = NULL;
    }

	m_scene.UnInit();
}


//IVMRSurfaceAllocator9
HRESULT CAllocator::InitializeDevice( 
            /* [in] */ DWORD_PTR dwUserID,
            /* [in] */ VMR9AllocationInfo *lpAllocInfo,
            /* [out][in] */ DWORD *lpNumBuffers)
{
    D3DCAPS9 d3dcaps;
    DWORD dwWidth = 1;
    DWORD dwHeight = 1;
    float fTU = 1.f;
    float fTV = 1.f;
	m_drawing = false;

	OutputDebugStringA("Init Device\n");
    if( lpNumBuffers == NULL )
    {
        return E_POINTER;
    }

    if( m_lpIVMRSurfAllocNotify == NULL )
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

	CAutoLock lck(&m_device_sec);
	if (m_D3DDev == NULL)
	{
		OutputDebugStringA("InitializeDevice with m_D3DDev = NULL\n");
		return S_OK;		//fixme:wrong?
	}
    m_D3DDev->GetDeviceCaps( &d3dcaps );
    if( d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2 )
    {
        while( dwWidth < lpAllocInfo->dwWidth )
            dwWidth = dwWidth << 1;
        while( dwHeight < lpAllocInfo->dwHeight )
            dwHeight = dwHeight << 1;

        fTU = (float)(lpAllocInfo->dwWidth) / (float)(dwWidth);
        fTV = (float)(lpAllocInfo->dwHeight) / (float)(dwHeight);
        m_scene.SetSrcRect( fTU, fTV );
        lpAllocInfo->dwWidth = dwWidth;
        lpAllocInfo->dwHeight = dwHeight;
    }

    // NOTE:
    // we need to make sure that we create textures because
    // surfaces can not be textured onto a primitive.
    lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;

    //lpAllocInfo->dwFlags &= ~VMR9AllocFlag_TextureSurface;
    //lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface;
    DeleteSurfaces();
    m_surfaces.resize(*lpNumBuffers);
    hr = m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, & m_surfaces.at(0) );
    
    // If we couldn't create a texture surface and 
    // the format is not an alpha format,
    // then we probably cannot create a texture.
    // So what we need to do is create a private texture
    // and copy the decoded images onto it.
    if(FAILED(hr) && !(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget))
    {
        DeleteSurfaces();            

		mylog("creating private texture\n");
        // is surface YUV ?
        if (lpAllocInfo->Format > '0000') 
        {           
            D3DDISPLAYMODE dm; 
            FAIL_RET( m_D3DDev->GetDisplayMode(NULL,  & dm ) );

            // create the private texture
            FAIL_RET( m_D3DDev->CreateTexture(lpAllocInfo->dwWidth, lpAllocInfo->dwHeight,
                                    1, 
                                    D3DUSAGE_RENDERTARGET, 
                                    dm.Format, 
                                    D3DPOOL_DEFAULT /* default pool - usually video memory */, 
                                    & m_privateTexture.p, NULL ) );
        }

        
        lpAllocInfo->dwFlags &= ~VMR9AllocFlag_TextureSurface;
        lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface;

        FAIL_RET( m_lpIVMRSurfAllocNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, & m_surfaces.at(0) ) );
    }

	OutputDebugStringA("Init Scene.\n");
    return m_scene.Init(m_D3DDev);
}
            
HRESULT CAllocator::TerminateDevice( 
        /* [in] */ DWORD_PTR dwID)
{
	OutputDebugStringA("TerminateDevice\n");
	CAutoLock lck(&m_device_sec);
    DeleteSurfaces();
    return S_OK;
}
    
HRESULT CAllocator::GetSurface( 
        /* [in] */ DWORD_PTR dwUserID,
        /* [in] */ DWORD SurfaceIndex,
        /* [in] */ DWORD SurfaceFlags,
        /* [out] */ IDirect3DSurface9 **lplpSurface)
{
    if( lplpSurface == NULL )
    {
        return E_POINTER;
    }

    if (SurfaceIndex >= m_surfaces.size() ) 
    {
        return E_FAIL;
    }

    CAutoLock Lock(&m_ObjectLock);

    return m_surfaces[SurfaceIndex].CopyTo(lplpSurface) ;
}
    
HRESULT CAllocator::AdviseNotify( 
        /* [in] */ IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
    CAutoLock Lock(&m_ObjectLock);

    HRESULT hr;

	m_lpIVMRSurfAllocNotify = lpIVMRSurfAllocNotify;

	if (m_lpIVMRSurfAllocNotify == NULL)
		return S_OK;

    HMONITOR hMonitor = m_D3D->GetAdapterMonitor( D3DADAPTER_DEFAULT );
    FAIL_RET( m_lpIVMRSurfAllocNotify->SetD3DDevice( m_D3DDev, hMonitor ) );

    return hr;
}

HRESULT CAllocator::StartPresenting( 
    /* [in] */ DWORD_PTR dwUserID)
{
    CAutoLock Lock(&m_ObjectLock);
	m_dshow_presenting = 1;
	char tmp[256];sprintf(tmp, "StartPresenting From thread %d\n", GetCurrentThreadId());OutputDebugStringA(tmp);
	CAutoLock lck(&m_device_sec);
	ASSERT( m_D3DDev );
    if( m_D3DDev == NULL )
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CAllocator::StopPresenting( 
    /* [in] */ DWORD_PTR dwUserID)
{
	m_dshow_presenting = 0;
	char tmp[256];sprintf(tmp, "StopPresenting From thread %d\n", GetCurrentThreadId());OutputDebugStringA(tmp);
    return S_OK;
}

HRESULT CAllocator::PresentImage( 
    /* [in] */ DWORD_PTR dwUserID,
    /* [in] */ VMR9PresentationInfo *lpPresInfo)
{
    HRESULT hr;
	double time1=CycleTime();
	CAutoLock Lock(&m_ObjectLock);
	double time2=CycleTime();

    // if we are in the middle of the display change

	/*
    if( NeedToHandleDisplayChange() )
    {
        // NOTE: this piece of code is left as a user exercise.  
        // The D3DDevice here needs to be switched
        // to the device that is using another adapter
    }
	*/

	/*
	if (GetKeyState(VK_F11) < 0)
	{
		OutputDebugStringA("F11\n");
		pp.Windowed = !pp.Windowed;
		DeleteSurfaces();
		HMONITOR hMonitor = m_D3D->GetAdapterMonitor( D3DADAPTER_DEFAULT );
		while (FAILED(CreateDevice()))
			Sleep(50);

		hr = m_lpIVMRSurfAllocNotify->ChangeD3DDevice( m_D3DDev, hMonitor ) ;
		if (FAILED(hr))
		{
			OutputDebugString(L"Change Device Failed\n");
			return hr;
		}
		else
		{
			OutputDebugString(L"Change Device OK!\n");
		}
		
		return S_OK;
	}
	*/
    hr = PresentHelper( lpPresInfo );
	char tmp[256];
	sprintf(tmp, "(%d,%f)LockTime=%f, PresentHelper:%f\n", GetCurrentThreadId(), CycleTime(), time2-time1, CycleTime()-time2);
	OutputDebugStringA(tmp);
    // IMPORTANT: device can be lost when user changes the resolution
    // or when (s)he presses Ctrl + Alt + Delete.
    // We need to restore our video memory after that

	m_dshow_presenting++;

	if (FAILED(hr))
	{
		mylog("PresentHelper Failed with hr=0x%08x.\n", hr);
	}

	if (!m_reseting)
	{
		m_reseting = true;
		if(( hr == D3DERR_DEVICELOST || m_pending_reset) && should_direct_render())//
		{
			mylog(hr == D3DERR_DEVICELOST?"DEVICE LOST\n":"F11");
			pp.Windowed = hr == D3DERR_DEVICELOST ? TRUE : !pp.Windowed;

			//if (TestCopperativelLevel() == D3DERR_DEVICENOTRESET)
			mylog("test=0x%08x;\n", TestCopperativelLevel());
			{
				DeleteSurfaces();
				while (FAILED(CreateDevice()))
					Sleep(500);
				//pp.hDeviceWindow = NULL;
				//hr = m_D3DDev->Reset(&pp);


				m_pending_reset = false;

				CAutoLock lck(&m_device_sec);
				HMONITOR hMonitor = m_D3D->GetAdapterMonitor( D3DADAPTER_DEFAULT );
				hr = m_lpIVMRSurfAllocNotify->ChangeD3DDevice( m_D3DDev, hMonitor ) ;
				if (FAILED(hr))
				{
					OutputDebugString(L"Change Device Failed\n");
					m_reseting = false;
					return hr;
				}
				else
				{
					OutputDebugString(L"Change Device OK!\n");
				}

			}

			OutputDebugString(L"RESET DONE\n");

			hr = S_OK;
		}
		m_reseting = false;
	}

	if (FAILED(hr))
	{
		printf("hr = %08x\n", hr);
	}

    return hr;
}

HRESULT CAllocator::PresentHelper(VMR9PresentationInfo *lpPresInfo)
{
    // parameter validation
    if( lpPresInfo == NULL )
    {
        return E_POINTER;
    }
    else if( lpPresInfo->lpSurf == NULL )
    {
        return E_POINTER;
    }

    CAutoLock Lock(&m_ObjectLock);
    HRESULT hr = S_OK;

	m_drawing = true;
    //m_D3DDev->SetRenderTarget( 0, m_renderTarget );
    // if we created a  private texture
    // blt the decoded image onto the texture.
	//CAutoLock(&m_scene.m_sec);
	//CAutoLock lck(&m_device_sec);
	if(!m_D3DDev) return S_FALSE;
	if( m_privateTexture != NULL )
    {   
        CComPtr<IDirect3DSurface9> surface;
        FAIL_RET( m_privateTexture->GetSurfaceLevel( 0 , & surface.p ) );

        // copy the full surface onto the texture's surface
        FAIL_RET( m_D3DDev->StretchRect( lpPresInfo->lpSurf, NULL,
                             surface, NULL,
                             D3DTEXF_NONE ) );

		OutputDebugStringA("StretchRect on PresentHelper\n");

        if (should_direct_render())
			FAIL_RET( m_scene.draw_to_my_surface(m_D3DDev, m_privateTexture, pp.Windowed));
		else
		{
			CAutoLock lck(&m_image_sec);
			mylog("\nsaving image\n");
			m_texture_to_draw = NULL;
			m_texture_to_draw = m_privateTexture;
		}
    }
    else // this is the case where we have got the textures allocated by VMR
         // all we need to do is to get them from the surface
    {
        CComPtr<IDirect3DTexture9> texture;
        FAIL_RET( lpPresInfo->lpSurf->GetContainer( IID_IDirect3DTexture9, (LPVOID*) & texture.p ) );    
        
		if (should_direct_render())
			FAIL_RET( m_scene.draw_to_my_surface(m_D3DDev, texture, pp.Windowed));
		else
		{
			CAutoLock lck(&m_image_sec);
			mylog("\nsaving image\n");
			m_texture_to_draw = NULL;
			m_texture_to_draw = texture;
		}
    }

	hr = m_last_present_result;
	m_scene.copy_to_back_buffer(m_D3DDev);
	hr = m_D3DDev->Present(NULL, NULL, NULL, NULL);
	m_drawing = false;

    return hr;
}

bool CAllocator::NeedToHandleDisplayChange()
{
    if( ! m_lpIVMRSurfAllocNotify )
    {
        return false;
    }

    D3DDEVICE_CREATION_PARAMETERS Parameters;
    if( FAILED( m_D3DDev->GetCreationParameters(&Parameters) ) )
    {
        ASSERT( false );
        return false;
    }

    HMONITOR currentMonitor = m_D3D->GetAdapterMonitor( Parameters.AdapterOrdinal );

    HMONITOR hMonitor = m_D3D->GetAdapterMonitor( D3DADAPTER_DEFAULT );

    return hMonitor != currentMonitor;


}


// IUnknown
HRESULT CAllocator::QueryInterface( 
        REFIID riid,
        void** ppvObject)
{
    HRESULT hr = E_NOINTERFACE;

    if( ppvObject == NULL ) {
        hr = E_POINTER;
    } 
    else if( riid == IID_IVMRSurfaceAllocator9 ) {
        *ppvObject = static_cast<IVMRSurfaceAllocator9*>( this );
        AddRef();
        hr = S_OK;
    } 
    else if( riid == IID_IVMRImagePresenter9 ) {
        *ppvObject = static_cast<IVMRImagePresenter9*>( this );
        AddRef();
        hr = S_OK;
    } 
    else if( riid == IID_IUnknown ) {
        *ppvObject = 
            static_cast<IUnknown*>( 
            static_cast<IVMRSurfaceAllocator9*>( this ) );
        AddRef();
        hr = S_OK;    
    }

    return hr;
}

ULONG CAllocator::AddRef()
{
    return InterlockedIncrement(& m_refCount);
}

ULONG CAllocator::Release()
{
    ULONG ret = InterlockedDecrement(& m_refCount);
    if( ret == 0 )
    {
        delete this;
    }

    return ret;
}