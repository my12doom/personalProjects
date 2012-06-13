#include "MediaInfo.h"
#include <time.h>
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>
#include "MediaInfoDLL.h"
#include "global_funcs.h"
using namespace MediaInfoDLL;


HRESULT FillTree(HWND root, const wchar_t *filename);
HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text, HTREEITEM htiParent);
void DoEvents();


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

MediaInfoWindow::MediaInfoWindow(const wchar_t *filename)
{
	// generate a random string
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
	MediaInfoWindow *_this = (MediaInfoWindow*)GetWindowLongPtr(hWnd, GWL_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return _this->MainWndProc(hWnd, message, wParam, lParam);
}

LRESULT MediaInfoWindow::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	const int IDC_TREE = 200;

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
				400,
				600,
				hWnd,
				(HMENU) IDC_TREE,
				GetModuleHandle(NULL),
				NULL);

			FillTree(tree, m_filename);
			SetWindowTextW(hWnd, m_filename);

			SendMessage(hWnd, WM_SIZE, 0, 0);
		}
		break;

	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			MoveWindow(tree, 0, 0, rect.right, rect.bottom, TRUE);
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

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);


HRESULT FillTree(HWND root, const wchar_t *filename)
{
	HTREEITEM file = InsertTreeviewItem(root, filename, TVI_ROOT);
	InsertTreeviewItem(root, C(L"Reading Infomation ...."), file);
	SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)file);
	DoEvents();

	MediaInfo MI;
	MI.Open(filename);
	MI.Option(_T("Complete"));
	MI.Option(_T("Inform"));

	String str = MI.Inform().c_str();
	MI.Close();
	wchar_t *p = (wchar_t*)str.c_str();
	wchar_t *p2 = wcsstr(p, L"\n");
	wchar_t tmp[1024];
	bool next_is_a_header = true;
	
	TreeView_DeleteAllItems (root);
	file = InsertTreeviewItem(root, filename, TVI_ROOT);
	HTREEITEM insert_position = file;
	HTREEITEM headers[4096] = {0};
	int headers_count = 0;

	while (true)
	{
		if (p2)
		{
			p2[0] = NULL;
			p2 ++;
		}
		
		wcscpy(tmp, p);
		wcstrim(tmp);
		wcstrim(tmp, L'\n');
		wcstrim(tmp, L'\r');
		wcs_replace(tmp, L"  ", L" ");

		if (tmp[0] == NULL || tmp[0] == L'\n' || tmp[0] == L'\r')
		{
			next_is_a_header = true;
		}		
		else if (next_is_a_header)
		{
			next_is_a_header = false;
			
			headers[headers_count++] = insert_position = InsertTreeviewItem(root, tmp, file);
		}
		else
		{
			InsertTreeviewItem(root, tmp, insert_position);
		}


		if (!p2)
			break;

		p = p2;
		p2 = wcsstr(p2, L"\n");
	}

	//add some items to the the tree view common control
	SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)file);
	for (int i=0; i<headers_count; i++)
		SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)headers[i]);

	TreeView_SelectItem (root, file, NULL);

	return S_OK;
}

HRESULT show_media_info(const wchar_t *filename)
{
	new MediaInfoWindow(filename);

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