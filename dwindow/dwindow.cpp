#include <tchar.h>
#include "dwindow.h"
#include "resource.h"
#include <stdio.h>

#define DS_SHOW_MOUSE (WM_USER + 3)

LRESULT DecodeGesture(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	// Create a structure to populate and retrieve the extra message info.
	GESTUREINFO gi;  

	ZeroMemory(&gi, sizeof(GESTUREINFO));

	gi.cbSize = sizeof(GESTUREINFO);

	BOOL bResult  = GetGestureInfo((HGESTUREINFO)lParam, &gi);
	BOOL bHandled = FALSE;

	if (bResult){
		// now interpret the gesture
		switch (gi.dwID){
		   case GID_ZOOM:
			   // Code for zooming goes here
			   //gi.

			   printf("GID_ZOOM, %d, (%d,%d)\n", (int)gi.ullArguments, gi.ptsLocation.x, gi.ptsLocation.y);
			   bHandled = TRUE;
			   break;
		   case GID_PAN:
			   // Code for panning goes here
			   printf("GID_PAN\n");
			   bHandled = TRUE;
			   break;
		   case GID_ROTATE:
			   printf("GID_ROTATE\n");
			   // Code for rotation goes here
			   bHandled = TRUE;
			   break;
		   case GID_TWOFINGERTAP:
			   printf("GID_TWOFINGERTAP\n");
			   // Code for two-finger tap goes here
			   bHandled = TRUE;
			   break;
		   case GID_PRESSANDTAP:
			   printf("GID_PRESSANDTAP\n");
			   // Code for roll over goes here
			   bHandled = TRUE;
			   break;
		   case GID_BEGIN:
			   printf("GID_BEGIN, (%d-%d), seq %d\n", gi.ptsLocation.x, gi.ptsLocation.y, gi.dwSequenceID);
			   break;
		   case GID_END:
			   printf("GID_END, (%d-%d), seq %d\n", gi.ptsLocation.x, gi.ptsLocation.y, gi.dwSequenceID);
			   break;
		   default:
			   printf("Unkown GESTURE : dwID=%d\n", gi.dwID);
			   // A gesture was not recognized
			   break;
		}
	}else{
		DWORD dwErr = GetLastError();
		if (dwErr > 0){
			//MessageBoxW(hWnd, L"Error!", L"Could not retrieve a GESTUREINFO structure.", MB_OK);
		}
	}
	if (bHandled){
		return 0;
	}else{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}


LRESULT CALLBACK dwindow::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	


	LRESULT lr = S_FALSE;
	dwindow *_this = NULL;
	_this = (dwindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);
	int id = _this->hwnd_to_id(hWnd);
	if (id == 0 )
		return DefWindowProc(hWnd, message, wParam, lParam);

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

/*
	if (message ==  WM_TOUCH || message == WM_GESTURE)
	{
		printf("%s : \n", message == WM_TOUCH ? "TOUCH":"GESTURE");
		if (message == WM_GESTURE)
		{
			DecodeGesture(hWnd, message, wParam, lParam);
		}

		if (message == WM_TOUCH)
		{
			UINT cInputs = LOWORD(wParam);
			PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
			if (NULL != pInputs)
			{
				if (GetTouchInputInfo((HTOUCHINPUT)lParam,
					cInputs,
					pInputs,
					sizeof(TOUCHINPUT)))
				{
					// process pInputs
					for(int i=0; i<cInputs; i++)
					{
						TOUCHINPUT p = pInputs[i];
						printf("point#%d, touchID=%d, %d-%d", i, p.dwID, p.x, p.y);
					}

					printf("\n");


					if (!CloseTouchInputHandle((HTOUCHINPUT)lParam))
					{
						// error handling
					}
				}
				else
				{
					// GetLastError() and error handling
				}
				delete [] pInputs;
			}
			else
			{
				// error handling, presumably out of memory
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}

*/
	switch (message)
	{
	case WM_INITDIALOG:
		lr =_this->on_init_dialog(id, wParam, lParam);
		{
		//int o = RegisterTouchWindow(_this->id_to_hwnd(1), 0);
		//o = RegisterTouchWindow(_this->id_to_hwnd(2), 0);
		}
		break;

	case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		lr = lr = _this->on_paint(id, hdc);
		EndPaint(hWnd, &ps);
		break;

	case WM_MOVE:
		lr = _this->on_move(id, xPos, yPos);
		break;

	case WM_SYSCOMMAND:
		lr = _this->on_sys_command(id, wParam, lParam);
		break;

	case WM_GETMINMAXINFO:
		lr = _this->on_getminmaxinfo(id, (MINMAXINFO*)lParam);
		break;

	case WM_COMMAND:
		lr = _this->on_command(id, wParam, lParam);
		break;

	case WM_SIZE:
		lr = _this->on_size(id, (int)wParam, xPos, yPos);
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	case WM_DISPLAYCHANGE:
		lr = _this->on_display_change();
		break;

	case WM_NCMOUSEMOVE:
		lr = _this->on_nc_mouse_move(id, xPos, yPos);
		break;

	case WM_TIMER:
		lr = _this->on_timer((int)wParam);
		break;

	case WM_LBUTTONDOWN:
		lr = _this->on_mouse_down(id, VK_LBUTTON, xPos, yPos);
		break;
		
	case WM_RBUTTONDOWN:
		lr = _this->on_mouse_down(id, VK_RBUTTON, xPos, yPos);
		break;

	case WM_LBUTTONDBLCLK:
		lr = _this->on_double_click(id, VK_LBUTTON, xPos, yPos);
		break;

	case WM_RBUTTONDBLCLK:
		lr = _this->on_double_click(id, VK_RBUTTON, xPos, yPos);
		break;

	case WM_LBUTTONUP:
		lr = _this->on_mouse_up(id, VK_LBUTTON, xPos, yPos);
		break;

	case WM_RBUTTONUP:
		lr = _this->on_mouse_up(id, VK_RBUTTON, xPos, yPos);
		break;

	case WM_MOUSEMOVE:
		lr = _this->on_mouse_move(id, xPos, yPos);
		break;

	case WM_MOUSEWHEEL:
		lr = _this->on_mouse_wheel(id, HIWORD(wParam), LOWORD(wParam), xPos, yPos);
		break;

	case DS_SHOW_MOUSE:
		ShowCursor((BOOL)wParam);
		break;

	case WM_CLOSE:
		if (SUCCEEDED(lr = _this->on_close(1)))
			ShowWindow(_this->id_to_hwnd(1), SW_HIDE);
		if (SUCCEEDED(lr = _this->on_close(2)))
			ShowWindow(_this->id_to_hwnd(2), SW_HIDE);
		break;

	case WM_DROPFILES:
		{
			HDROP hDropInfo = (HDROP)wParam;
			int count = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
			if (count>0)
			{
				wchar_t **filenames = (wchar_t**)malloc(sizeof(wchar_t*)*count);
				for(int i=0; i<count; i++)
				{
					filenames[i] = (wchar_t*)malloc(sizeof(wchar_t) * MAX_PATH);
					DragQueryFileW(hDropInfo, i, filenames[i], MAX_PATH);
				}
				lr = _this->on_dropfile(id, count, filenames);
				for(int i=0; i<count; i++) free(filenames[i]);
				free(filenames);
			}
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		lr = _this->on_key_down(id, (int)wParam);
		break;

	default:
		return _this->on_unhandled_msg(id, message, wParam, lParam);
	}

	if (lr == S_FALSE)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}

DWORD WINAPI dwindow::WindowThread(LPVOID lpParame)
{
	HINSTANCE hinstance = GetModuleHandle(NULL);

	HWND hwnd; 	// Create the main window. 
	hwnd = CreateWindowExA(
		WS_EX_ACCEPTFILES,
		"DWindowClass",        // name of window class 
		"",					 // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		CW_USEDEFAULT,       // default width 
		CW_USEDEFAULT,       // default height 
		(HWND) NULL,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return FALSE; 
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)((window_proc_param*)lpParame)->that);

	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
	*((window_proc_param*)lpParame)->hwnd1 = hwnd;
	((window_proc_param*)lpParame)->that->m_thread_id1 = GetCurrentThreadId();

	hwnd = CreateWindowExA(
		WS_EX_ACCEPTFILES,
		"DWindow2Class",        // name of window class 
		"",					 // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		CW_USEDEFAULT,       // default width 
		CW_USEDEFAULT,       // default height 
		(HWND) hwnd,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return FALSE; 
	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)((window_proc_param*)lpParame)->that);

	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
	*((window_proc_param*)lpParame)->hwnd2 = hwnd;

	dwindow * that = ((window_proc_param*)lpParame)->that;
	MSG msg;
	memset(&msg,0,sizeof(msg));
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if(TranslateAccelerator(that->m_hwnd1, that->m_accel, &msg))
				continue;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			if (that->on_idle_time() == S_FALSE)
				Sleep(1);
		}
	}
	return (DWORD)msg.wParam; 
} 

HRESULT dwindow::show_window(int id, bool show)
{
	if(id == 1 && show)
		ShowWindow(m_hwnd1, SW_SHOW);
	else if (id == 1 && !show)
		ShowWindow(m_hwnd1, SW_HIDE);

	if(id == 2 && show)
		ShowWindow(m_hwnd2, SW_SHOW);
	else if (id == 2 && !show)
		ShowWindow(m_hwnd2, SW_HIDE);

	return S_OK;
}

HRESULT dwindow::set_window_text(int id, const wchar_t *text)
{
	SetWindowTextW(id_to_hwnd(id), text);

	return S_OK;
}

HRESULT dwindow::reset_timer(int id, int new_interval)
{
	KillTimer(m_hwnd1, id);
	SetTimer(m_hwnd1, id, new_interval, NULL);

	return S_OK;
}

HRESULT dwindow::show_mouse(bool show)
{
	if (show && !m_show_mouse)
	{
		m_show_mouse = true;
		SendMessage(m_hwnd1, DS_SHOW_MOUSE, TRUE, NULL);
		SendMessage(m_hwnd2, DS_SHOW_MOUSE, TRUE, NULL);
	}

	if (!show && m_show_mouse)
	{
		m_show_mouse = false;
		SendMessage(m_hwnd1, DS_SHOW_MOUSE, FALSE, NULL);
		SendMessage(m_hwnd2, DS_SHOW_MOUSE, FALSE, NULL);
	}

	return S_OK;
}

HRESULT dwindow::set_fullscreen(int id, bool full, bool nosave)
{
	if (id == 1 && full && !m_full1)
	{
		m_full1 = true;
		if (!nosave)GetWindowRect(m_hwnd1, &m_normal1);
		m_style1 = GetWindowLongPtr(m_hwnd1, GWL_STYLE);
		m_exstyle1 = GetWindowLongPtr(m_hwnd1, GWL_EXSTYLE);

		LONG f = m_style1 & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  m_exstyle1 & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

		SetWindowLongPtr(m_hwnd1, GWL_STYLE, f);
		SetWindowLongPtr(m_hwnd1, GWL_EXSTYLE, exf);

		SetWindowPos(m_hwnd1, HWND_TOPMOST, m_screen1.left, m_screen1.top,
					m_screen1.right - m_screen1.left, m_screen1.bottom - m_screen1.top,
					SWP_NOOWNERZORDER);

	}
	else if (id == 1 && !full && m_full1)
	{
		m_full1 = false;
		SetWindowLongPtr(m_hwnd1, GWL_STYLE, m_style1);
		SetWindowLongPtr(m_hwnd1, GWL_EXSTYLE, m_exstyle1);

		SetWindowPos(m_hwnd1, HWND_NOTOPMOST, m_normal1.left, m_normal1.top,
					m_normal1.right - m_normal1.left, m_normal1.bottom - m_normal1.top,
					SWP_NOOWNERZORDER);
	}



	if (id == 2 && full && !m_full2)
	{
		m_full2 = true;
		if(!nosave)GetWindowRect(m_hwnd2, &m_normal2);
		m_style2 = GetWindowLongPtr(m_hwnd2, GWL_STYLE);
		m_exstyle2 = GetWindowLongPtr(m_hwnd2, GWL_EXSTYLE);

		LONG f = m_style2 & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  m_exstyle2 & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;
		
		SetWindowLongPtr(m_hwnd2, GWL_STYLE, f);
		SetWindowLongPtr(m_hwnd2, GWL_EXSTYLE, exf);

		SetWindowPos(m_hwnd2, HWND_TOPMOST, m_screen2.left, m_screen2.top,
			m_screen2.right - m_screen2.left, m_screen2.bottom - m_screen2.top,
			SWP_NOOWNERZORDER);

	}
	else if (id == 2 && !full && m_full2)
	{
		m_full2 = false;
		SetWindowLongPtr(m_hwnd2, GWL_STYLE, m_style2);
		SetWindowLongPtr(m_hwnd2, GWL_EXSTYLE, m_exstyle2);

		SetWindowPos(m_hwnd2, HWND_NOTOPMOST, m_normal2.left, m_normal2.top,
			m_normal2.right - m_normal2.left, m_normal2.bottom - m_normal2.top,
			SWP_NOOWNERZORDER);
	}



	return S_OK;
}


dwindow::dwindow(RECT screen1, RECT screen2)
{
	if (memcmp(&screen1, &screen2, sizeof(RECT)) == 0)
	{
		screen2.left += screen1.right - screen1.left;
		screen2.right += screen1.right - screen1.left;
	}

	m_show_mouse = true;
	m_screen1 = screen1;
	m_screen2 = screen2;
	m_full1 = false;
	m_full2 = false;
	m_hwnd1 = NULL;
	m_hwnd2 = NULL;

	HINSTANCE hinstance = GetModuleHandle(NULL);
	WNDCLASSEX wcx; 
	wcx.cbSize = sizeof(wcx);          // size of structure 
	wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;// redraw if size changes 
	wcx.lpfnWndProc = MainWndProc;     // points to window procedure 
	wcx.cbClsExtra = 0;                // no extra class memory 
	wcx.cbWndExtra = 0;                // no extra window memory 
	wcx.hInstance = hinstance;         // handle to instance 
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);// predefined app. icon 
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);  // predefined arrow 
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName =  _T("MainMenu");    // name of menu resource 
	wcx.lpszClassName = _T("DWindowClass");  // name of window class 
	wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), 
		GetSystemMetrics(SM_CYSMICON), 
		LR_DEFAULTCOLOR); 

	// Register the window class. 
	if (!RegisterClassEx(&wcx))
		return;

	wcx.lpszClassName = _T("DWindow2Class");

	if (!RegisterClassEx(&wcx))
		return;

	window_proc_param param = {this, &m_hwnd1, &m_hwnd2};
	m_thread1 = CreateThread(0,0,WindowThread, &param, NULL, NULL);
	while(m_hwnd1 == NULL || m_hwnd2 == NULL)
		Sleep(1);

	m_style1 = GetWindowLongPtr(m_hwnd1, GWL_STYLE);
	m_exstyle1 = GetWindowLongPtr(m_hwnd1, GWL_EXSTYLE);
	m_style2 = GetWindowLongPtr(m_hwnd2, GWL_STYLE);
	m_exstyle2 = GetWindowLongPtr(m_hwnd2, GWL_EXSTYLE);
}


dwindow::~dwindow()
{
	close_and_kill_thread();
}
void dwindow::close_and_kill_thread()
{
	SendMessage(m_hwnd1, WM_DESTROY, 0, 0);
	SendMessage(m_hwnd2, WM_DESTROY, 0, 0);

	WaitForSingleObject(m_thread1, INFINITE);
	WaitForSingleObject(m_thread2, INFINITE);

	UnregisterClass(_T("DWindowClass"), GetModuleHandle(NULL));
}
HWND dwindow::id_to_hwnd(int id)
{
	if(id == 1)
		return m_hwnd1;

	if(id == 2)
		return m_hwnd2;

	return NULL;
}
int dwindow::hwnd_to_id(HWND hwnd)
{
	if (hwnd == m_hwnd1)
		return 1;

	if (hwnd == m_hwnd2)
		return 2;

	return 0;
}

bool dwindow::is_visible(int id)
{
	if (IsWindowVisible(id_to_hwnd(id)))
		return true;
	return false;
}