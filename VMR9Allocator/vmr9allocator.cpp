//------------------------------------------------------------------------------
// File: vmr9allocator.cpp
//
// Desc: DirectShow sample code - main implementation file
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#pragma warning(disable: 4710)  // "function not inlined"

#include "stdafx.h"
#include "resource.h"
#include "util.h"
#include "Allocator.h"
#include "vmrutil.h"

#define MAX_LOADSTRING 100
bool nv3d_enabled = false;
bool nv3d_actived = false;

#define DS_EVENT (WM_USER + 4)

// Global Variables:
HINSTANCE hInst;                       
TCHAR szTitle[MAX_LOADSTRING];         
TCHAR szWindowClass[MAX_LOADSTRING];  

// Forward declarations of functions included in this code module:
BOOL                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, HWND& );
void                PaintWindow(HWND hWnd);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HRESULT             StartGraph(HWND window);
HRESULT             CloseGraph(HWND window);
HRESULT             SetAllocatorPresenter( CComPtr<IBaseFilter> filter, HWND window );

DWORD_PTR                       g_userId = 0xACDCACDC;
HWND                            g_hWnd;

// DirectShow interfaces
CComPtr<IGraphBuilder>          g_graph;
CComPtr<IBaseFilter>            g_filter;
CComPtr<IMediaControl>          g_mediaControl;
CComPtr<IMediaSeeking>          g_mediaSeeking;
CAllocator*  g_allocator;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    int ret = -1;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	NvAPI_Status res = NvAPI_Initialize();
	NvU8 enabled3d;
	res = NvAPI_Stereo_IsEnabled(&enabled3d);
	if (res == NVAPI_OK)
	{
		nv3d_enabled = (bool)enabled3d;
	}

    // Verify that the VMR9 is present on this system
    if(!VerifyVMR9())
    {
        CoUninitialize();
        return FALSE;
    }

    __try 
    {
        MSG msg;
        HACCEL hAccelTable;

        // Initialize global strings
        LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadString(hInstance, IDC_ALLOCATOR9, szWindowClass, MAX_LOADSTRING);
        MyRegisterClass(hInstance);

        // Perform application initialization:
        if (!InitInstance (hInstance, nCmdShow, g_hWnd)) 
        {
            // Exit the try block gracefully.  Returning FALSE here would
            // lead to a performance penalty unwinding the stack.
            __leave;
        }

        hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_ALLOCATOR9);

        // Main message loop:
		memset(&msg,0,sizeof(msg));
		while( msg.message != WM_QUIT )
		{
			if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{ 
				TranslateMessage( &msg );
				DispatchMessage( &msg );
				//if (g_allocator)
				//	((CAllocator*)(IVMRSurfaceAllocator9*)g_allocator)->Present();
			}
			else
			{
				if (g_allocator)
				{
					while (g_allocator->Pump() != S_FALSE)
						;
				}
				Sleep(1);
			}
		}

		/*
        while (GetMessage(&msg, NULL, 0, 0)) 
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
		*/

        ret = (int) msg.wParam;
    }
    __finally
    {
        // make sure to release everything at the end
        // regardless of what's happening
        CloseGraph(g_hWnd);
        CoUninitialize();
    }

    return ret;
}

_bstr_t GetMoviePath()
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    TCHAR  szBuffer[MAX_PATH];
    szBuffer[0] = NULL;

    static const TCHAR szFilter[]  
                            = TEXT("Video Files (.ASF, .AVI, .MPG, .MPEG, .VOB, .QT, .WMV)\0*.ASF;*.AVI;*.MPG;*.MPEG;*.VOB;*.QT;*.WMV\0") \
                              TEXT("All Files (*.*)\0*.*\0\0");
    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = g_hWnd;
    ofn.hInstance           = NULL;
    ofn.lpstrFilter         = szFilter;
    ofn.nFilterIndex        = 1;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.lpstrFile           = szBuffer;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrTitle          = TEXT("Select a video file to play...");
    ofn.Flags               = OFN_HIDEREADONLY;
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = TEXT("AVI");
    ofn.lCustData           = 0L;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName  = NULL; 
    
    if (GetOpenFileName (&ofn))  // user specified a file
    {
        return _bstr_t( szBuffer );
    }

   return "";
}

HRESULT             
CloseGraph(HWND window)
{
    if( g_mediaControl != NULL ) 
    {
        OAFilterState state;
        do {
            g_mediaControl->Stop();
            g_mediaControl->GetState(0, & state );
        } while( state != State_Stopped ) ;
    }

	g_mediaSeeking = NULL;
    g_mediaControl = NULL;        
    g_filter       = NULL;
	if (g_allocator) g_allocator->AdviseNotify(NULL);
    g_graph        = NULL;
	if (g_allocator)
	{
		delete g_allocator;
		g_allocator = NULL;
	}


    ::InvalidateRect( window, NULL, true );
    return S_OK;
}

HRESULT StartGraph(HWND window)
{
    // Clear DirectShow interfaces (COM smart pointers)
    CloseGraph(window);

    _bstr_t path = GetMoviePath();
    if( ! path.length() )
    {
        return E_FAIL;
    }

    HRESULT hr;
    
    FAIL_RET( g_graph.CoCreateInstance(CLSID_FilterGraph) );

    FAIL_RET( g_filter.CoCreateInstance(CLSID_VideoMixingRenderer9, NULL, CLSCTX_INPROC_SERVER) );

    CComPtr<IVMRFilterConfig9> filterConfig;
    FAIL_RET( g_filter->QueryInterface(IID_IVMRFilterConfig9, reinterpret_cast<void**>(&filterConfig)) );

    //FAIL_RET( filterConfig->SetNumberOfStreams(1) );

    FAIL_RET( filterConfig->SetRenderingMode( VMR9Mode_Renderless ) );

    FAIL_RET( SetAllocatorPresenter( g_filter, window ) );

    FAIL_RET( g_graph->AddFilter(g_filter, L"Video Mixing Renderer 9") );
    
    FAIL_RET( g_graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&g_mediaControl)) );

	FAIL_RET( g_graph->QueryInterface(IID_IMediaSeeking, reinterpret_cast<void**>(&g_mediaSeeking)) );

    FAIL_RET( g_graph->RenderFile( path, NULL ) );

	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(g_graph);
	event_ex->SetNotifyWindow((OAHWND)window, DS_EVENT, 0);

    FAIL_RET( g_mediaControl->Run() );

    return hr;
}

HRESULT SetAllocatorPresenter( CComPtr<IBaseFilter> filter, HWND window )
{
    if( filter == NULL )
    {
        return E_FAIL;
    }

    HRESULT hr;

    CComPtr<IVMRSurfaceAllocatorNotify9> lpIVMRSurfAllocNotify;
    FAIL_RET( filter->QueryInterface(IID_IVMRSurfaceAllocatorNotify9, reinterpret_cast<void**>(&lpIVMRSurfAllocNotify)) );

    // create our surface allocator
    g_allocator = new CAllocator( hr, window );
    if( FAILED( hr ) )
    {
        g_allocator = NULL;
        return hr;
    }

    // let the allocator and the notify know about each other
    FAIL_RET( lpIVMRSurfAllocNotify->AdviseSurfaceAllocator( g_userId, g_allocator ) );
    FAIL_RET( g_allocator->AdviseNotify(lpIVMRSurfAllocNotify) );
    
    return hr;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
BOOL MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));

    // Set the members of the window class structure.
    //
    // Don't provide a background brush, because we process the WM_PAINT
    // messages in OnPaint().  If a movie is active, we tell the VMR to
    // repaint the window; otherwise, we repaint with COLOR_WINDOW+1.
    // If a background brush is provided, you will see a white flicker
    // whenever you resize the main application window, because Windows
    // will repaint the window before the application also repaints.
    wc.hInstance     = hInstance;
    wc.lpfnWndProc   = WndProc;
    wc.lpszClassName = szWindowClass;
    wc.lpszMenuName  = (LPCTSTR)IDC_ALLOCATOR9;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hbrBackground = NULL;        // No background brush
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ALLOCATOR9));
    if(!RegisterClass(&wc))
    {
        return FALSE;
    }

    return TRUE;
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& hWnd)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                       100, 100, 600, 500, 
                       NULL, NULL, hInstance, NULL);

   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void PaintWindow(HWND hWnd)
{
    bool bNeedPaint = false;

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    if( g_mediaControl == NULL ) // we know that there is nothing loaded
    {
        bNeedPaint = true;
    }
    else
    {
        // If we have a movie loaded, we only need to repaint
        // when the graph is stopped
        OAFilterState state;
        if( SUCCEEDED( g_mediaControl->GetState(0, & state ) ) )
        {
            bNeedPaint = ( state != State_Running );
        }
    }

    if ( bNeedPaint ) 
    {
        RECT rc2;
        GetClientRect(hWnd, &rc2);
		if(g_allocator)
		{
			HRESULT hr;
			if (FAILED(hr = g_allocator->Present()))
			{
				printf("WM_Paint:Present() failed: 0x%08x.\n", hr);
				if (g_mediaControl)
				{
					//g_mediaControl->Run();
				}
			}
		}
		else
		{
			FillRect(hdc, &rc2, (HBRUSH)(COLOR_WINDOW+1));
		}
    }

    EndPaint( hWnd, &ps );
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    int wmId, wmEvent;

    switch (message) 
    {
		case WM_ACTIVATEAPP:
			if(false)
			{
				if (wParam)
					MessageBoxA(0, "activing", "...", MB_OK);
				else
					MessageBoxA(0, "DEactiving", "...", MB_OK);

			}
			break;
		case NV_NOTIFY:
		{
			BOOL actived = LOWORD(wParam);
			WORD separation = HIWORD(wParam);
			char tmp[256];
			sprintf(tmp, "%d:%d", actived, separation);
			//MessageBoxA(hWnd, tmp, "...", MB_OK);
			if (actived)
			{
				nv3d_actived = true;
			}
			else
			{
				nv3d_actived = false;
				//sprintf(tmp, "%d", nv3d_actived);
				//MessageBoxA(NULL, tmp, "", MB_OK);
			}
		}
		break;

		case DS_EVENT:
			{
				OutputDebugStringA("DS_EVENT\n");
				CComQIPtr<IMediaEvent, &IID_IMediaEvent> event(g_graph);

				long event_code;
				LONG_PTR param1;
				LONG_PTR param2;
				if (FAILED(event->GetEvent(&event_code, &param1, &param2, 0)))
					return S_OK;

				if (event_code == EC_COMPLETE)
				{
					OutputDebugStringA("EC_COMPLETE\n");
					g_mediaControl->Stop();
					REFERENCE_TIME target = 0;
					g_mediaSeeking->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
					g_mediaControl->Pause();
				}
			}
			break;

        case WM_COMMAND:
            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 

            // Parse the menu selections:
            switch (wmId)
            {
                case IDM_PLAY_FILE:
                    hr = StartGraph(g_hWnd);
                    if( FAILED(hr) )
                    {
                        return 0;
                    }
                    break;

                case ID_FILE_CLOSE:
                    CloseGraph( hWnd );
                    break;

                case IDM_ABOUT:
                   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
                   break;

                case IDM_EXIT:
                   DestroyWindow(hWnd);
                   break;

                default:
                   return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;

        case WM_PAINT:
            PaintWindow(hWnd);
            break;

		case WM_KEYDOWN:
			{
				int keycode = (int)wParam;
				if (keycode == VK_F1)
					g_mediaControl->Run();
				else if (keycode == VK_F2)
					g_mediaControl->Pause();
				else if (keycode == VK_F3)
					g_mediaControl->StopWhenReady();
				else if (keycode == VK_F11)
				{
					OAFilterState state_before;
					g_mediaControl->GetState(INFINITE, &state_before);

					if (state_before != State_Running)
						g_mediaControl->Run();

					g_allocator->m_pending_reset = true;
					g_allocator->m_dshow_presenting = 1;
					while (g_allocator->m_pending_reset || g_allocator->m_dshow_presenting < 3)
					{
						g_allocator->Pump();
					}
					if (state_before != State_Running)
					{
						g_mediaControl->Stop();
						g_allocator->m_dshow_presenting = 0;
					}

					g_allocator->Present();

					mylog("m_presenting = %d\n", g_allocator->m_dshow_presenting);
				}
				else if (keycode == VK_F9)
				{
					if (g_allocator)
					{
						g_allocator->Present();
					}
				}
			}
			break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
                return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }

    return FALSE;
}
