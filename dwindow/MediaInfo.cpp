#include "MediaInfo.h"
#include <time.h>
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>
#include <assert.h>
#include "MediaInfoDLL.h"
#include "global_funcs.h"
#include "dwindow_log.h"
#include "..\hookdshow\hookdshow.h"
using namespace MediaInfoDLL;


HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text, HTREEITEM htiParent);
void DoEvents();

class MediaInfoWindow
{
public:
	MediaInfoWindow(HWND parent);

// protected:
	~MediaInfoWindow() {free(m_msg);}			// this class will suicide after window closed, so, don't delete
	static LRESULT CALLBACK DummyMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI pump_thread(LPVOID lpParame){return ((MediaInfoWindow*)lpParame)->pump();}
	DWORD pump();
	HRESULT FillTree();

	wchar_t m_classname[100];		// just a random class name
	wchar_t *m_msg;
	HWND m_parent;

	HWND tree;
	HWND copy_button;
};

MediaInfoWindow::MediaInfoWindow(HWND parent)
{
	m_parent = parent;

	// generate a random string
	for(int i=0; i<50; i++)
		m_classname[i] = L'a' + rand()%25;
	m_classname[50] = NULL;

	// alloc message space
	m_msg = (wchar_t*)malloc(1024*1024);		// 1M ought to be enough for everybody
	m_msg[0] = NULL;

	// GOGOGO
	CreateThread(NULL, NULL, pump_thread, this, NULL, NULL);
}

DWORD MediaInfoWindow::pump()
{
	HINSTANCE hinstance = GetModuleHandle(NULL);
	WNDCLASSEX wcx; 
	wcx.cbSize = sizeof(wcx);          // size of structure 
	wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;// redraw if size changes 
	wcx.lpfnWndProc = DummyMainWndProc;     // points to window procedure 
	wcx.cbClsExtra = 0;                // no extra class memory 
	wcx.cbWndExtra = 0;                // no extra window memory 
	wcx.hInstance = hinstance;         // handle to instance 
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);// predefined app. icon 
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);  // predefined arrow 
	wcx.hbrBackground =  GetStockBrush(WHITE_BRUSH);
	wcx.lpszMenuName =  _T("");    // name of menu resource 
	wcx.lpszClassName = m_classname;  // name of window class 
	wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), 
		GetSystemMetrics(SM_CYSMICON), 
		LR_DEFAULTCOLOR); 

	// Register the window class. 
	if (!RegisterClassEx(&wcx))
		return -1;

	HWND hwnd; 	// Create the main window. 
	hwnd = CreateWindowExW(
		WS_EX_ACCEPTFILES,
		m_classname,        // name of window class 
		L"",					 // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		GET_CONST("MediaInfoWidth"),       // default width 
		GET_CONST("MediaInfoHeight"),       // default height 
		(HWND) m_parent,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return -2;

	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SendMessage(hwnd, WM_INITDIALOG, 0, 0);

	MSG msg;
	memset(&msg,0,sizeof(msg));
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			Sleep(1);
		}
	}


	// 
	UnregisterClass(m_classname, GetModuleHandle(NULL));

	delete this;

	return (DWORD)msg.wParam; 
}
LRESULT CALLBACK MediaInfoWindow::DummyMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MediaInfoWindow *_this = (MediaInfoWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return _this->MainWndProc(hWnd, message, wParam, lParam);
}

LRESULT MediaInfoWindow::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	const int IDC_TREE = 200;
	const int IDC_COPY = 201;

	switch (message)
	{
	case WM_COMMAND:
		{
		size_t size = sizeof(wchar_t)*(1+wcslen(m_msg));
		OutputDebugStringW(m_msg);
		HGLOBAL hResult = GlobalAlloc(GMEM_MOVEABLE, size); 
		LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hResult); 
		memcpy(lptstrCopy, m_msg, size); 
		GlobalUnlock(hResult);
		OpenClipboard(hWnd);
		EmptyClipboard();
		if (SetClipboardData( CF_UNICODETEXT, hResult ) == NULL )
			GlobalFree(hResult);		//Free buffer if clipboard didn't take it.
		CloseClipboard();
		}
		break;

	case WM_INITDIALOG:
		{
			DWORD style = WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
			tree = CreateWindowEx(0,
				WC_TREEVIEW,
				0,
				style,
				0,
				100,
				GET_CONST("MediaInfoWidth"),
				GET_CONST("MediaInfoHeight"),
				hWnd,
				(HMENU) IDC_TREE,
				GetModuleHandle(NULL),
				NULL);


			style = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
			copy_button = CreateWindowW(L"Button", C(L"Copy To Clipboard"), style, 0, 0, 200, 100, hWnd, NULL, GetModuleHandle(NULL), NULL);

			SendMessage(hWnd, WM_SIZE, 0, 0);

			SetWindowTextW(hWnd, C(L"Media Info"));
			FillTree();
		}
		break;

	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			RECT pos;
			GetWindowRect(hWnd, &pos);
			GET_CONST("MediaInfoWidth") = pos.right - pos.left;
			GET_CONST("MediaInfoHeight") = pos.bottom - pos.top;
			MoveWindow(tree, 0, 0, rect.right, rect.bottom-50, TRUE);
			MoveWindow(copy_button, 0, rect.bottom-50, rect.right, 50, TRUE);
			SetWindowTextW(copy_button, C(L"Copy To Clipboard"));
			ShowWindow(tree, SW_SHOW);
			ShowWindow(copy_button, SW_SHOW);
			BringWindowToTop(copy_button);
		}
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text,HTREEITEM htiParent)
{
	TVITEM tvi = {0};

	tvi.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;


	//copy the text into a temporary array (vector) so it's in a suitable form
	//for the pszText member of the TVITEM struct to use. This avoids using
	//const_cast on 'txt.c_str()' or variations applied directly to the string that
	//break its constant nature.

	tvi.pszText = (LPWSTR)text;
	tvi.cchTextMax =static_cast<int>(wcslen(text));  //length of item label
	tvi.iImage = 0;  //non-selected image index

	TVINSERTSTRUCT tvis={0};

	tvi.iSelectedImage = 1;   //selected image index
	tvis.item = tvi; 
	tvis.hInsertAfter = 0;
	tvis.hParent = htiParent; //parent item of item to be inserted

	return reinterpret_cast<HTREEITEM>(SendMessageW(hTv,TVM_INSERTITEM,0,reinterpret_cast<LPARAM>(&tvis)));
}

static int insert_item(lua_State *L)
{
	assert(lua_gettop(L)==4);
	MediaInfoWindow *window = (MediaInfoWindow*)lua_touserdata(L, 1);
	HWND hTv = window->tree;
	UTF82W string(lua_tostring(L,2));
	HTREEITEM htiParent = (HTREEITEM)lua_touserdata(L, 3);
	int level = lua_tointeger(L, 4);

	wchar_t space[500] = {0};
	for(int i=0; i<level*2; i++)
		space[i] = L' ';

	wcscat(window->m_msg, space);
	wcscat(window->m_msg, string);
	wcscat(window->m_msg, L"\r\n");

	if (htiParent == NULL)
		htiParent = TVI_ROOT;

	SendMessage(hTv, TVM_EXPAND, TVE_EXPAND, (LPARAM)htiParent);

	lua_pushlightuserdata(L, InsertTreeviewItem(hTv, string, htiParent));
	return 1;
}

HRESULT MediaInfoWindow::FillTree()
{
	luaState L;
	lua_getglobal(L, "show_media_info");
	lua_pushlightuserdata(L, this);
	lua_pushcfunction(L, &insert_item);
	lua_mypcall(L, 2, 0, 0);

	return S_OK;
}

HRESULT show_media_info(HWND parent)
{
	new MediaInfoWindow(parent);

	return S_OK;
}


void DoEvents()
{
	MSG msg;
	BOOL result;

	while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		result = ::GetMessage(&msg, NULL, 0, 0);
		if (result == 0) // WM_QUIT
		{                
			::PostQuitMessage(msg.wParam);
			break;
		}
		else if (result == -1)
		{
			// Handle errors/exit application, etc.
		}
		else 
		{
			::TranslateMessage(&msg);
			:: DispatchMessage(&msg);
		}
	}
}
