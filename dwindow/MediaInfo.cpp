#include "MediaInfo.h"
#include <time.h>
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>

HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text,
							 HTREEITEM htiParent);


class MediaInfoWindow
{
public:
	MediaInfoWindow(const wchar_t *filename);

protected:
	~MediaInfoWindow() {}			// this class will suicide after window closed, so, don't delete
	static LRESULT CALLBACK DummyMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI pump_thread(LPVOID lpParame){return ((MediaInfoWindow*)lpParame)->pump();}
	DWORD pump();

	wchar_t m_filename[MAX_PATH];
	wchar_t m_classname[100];		// just a random class name

	HWND tree;
};

HRESULT show_media_info(const wchar_t *filename)
{
	new MediaInfoWindow(filename);

	return S_OK;
}

MediaInfoWindow::MediaInfoWindow(const wchar_t *filename)
{
	// generate a random string
	srand((unsigned int)time(NULL));
	for(int i=0; i<50; i++)
		m_classname[i] = L'a' + rand()%25;
	m_classname[50] = NULL;

	// store filename
	wcscpy(m_filename, filename);

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
		400,       // default width 
		600,       // default height 
		(HWND) NULL,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return -2;

	SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)this);
	SendMessage(hwnd, WM_INITDIALOG, 0, 0);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

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
	MediaInfoWindow *_this = (MediaInfoWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return _this->MainWndProc(hWnd, message, wParam, lParam);
}

LRESULT MediaInfoWindow::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	const int IDC_TREE = 200;

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			DWORD style = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
			tree = CreateWindowEx(0,
				WC_TREEVIEW,
				0,
				style,
				0,
				0,
				40,
				30,
				hWnd,
				(HMENU) IDC_TREE,
				GetModuleHandle(NULL),
				NULL);
			//add some items to the the tree view common control
			HTREEITEM file=InsertTreeviewItem(tree, m_filename,TVI_ROOT);

			//sub items of first item
			HTREEITEM video = InsertTreeviewItem(tree, L"Video", file);
			InsertTreeviewItem(tree, L"Video1", video);
			InsertTreeviewItem(tree, L"Video2", video);
			InsertTreeviewItem(tree, L"Video3", video);

			//sub items of first item
			HTREEITEM audio = InsertTreeviewItem(tree, L"Audio", file);
			InsertTreeviewItem(tree, L"Audio1", audio);
			InsertTreeviewItem(tree, L"Audio2", audio);
			InsertTreeviewItem(tree, L"Audio3", audio);

			//sub items of first item
			HTREEITEM subtitle = InsertTreeviewItem(tree, L"Subtitle", file);
			InsertTreeviewItem(tree, L"Subtitle1", subtitle);
			InsertTreeviewItem(tree, L"Subtitle2", subtitle);
			InsertTreeviewItem(tree, L"Subtitle3", subtitle);


			SendMessage(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)file);
			SendMessage(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)video);
			SendMessage(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)audio);
			SendMessage(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)subtitle);
		}
		break;

	case WM_SIZE:
		{
			MoveWindow(tree, 0, 0, xPos, yPos, TRUE);

			char tmp[200];
			sprintf(tmp, "%dx%d\n", xPos, yPos);
			OutputDebugStringA(tmp);
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


HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text,
							 HTREEITEM htiParent)
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

	return reinterpret_cast<HTREEITEM>(SendMessageW(hTv,TVM_INSERTITEM,0,
		reinterpret_cast<LPARAM>(&tvis)));
}