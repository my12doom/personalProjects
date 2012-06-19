/**
 * Copyright (C) 2002-2005
 * W3L GmbH
 * Technologiezentrum Ruhr
 * Universitätsstraße 142
 * D-44799 Bochum
 * 
 * Author: Dipl.Ing. Doga Arinir
 * E-Mail: doga.arinir@w3l.de
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the author or the company be held liable 
 * for any damages arising from the use of this software. EXPECT BUGS!
 * 
 * You may use this software in compiled form in any way you desire PROVIDING it is
 * not sold for profit without the authors written permission, and providing that this
 * notice and the authors name is included. If the source code in this file is used in 
 * any commercial application then acknowledgement must be made to the author of this file.
 */

#include "Hyperlink.h"
#include "shellapi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define STATIC_HYPER_WINDOW_CLASS L"STATIC_HYPER"
#define STATIC_HYPER_WINDOW_CLASSA "STATIC_HYPER"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CHyperlink::_autoinitializer::_autoinitializer() : hModule(NULL)
{
	//First, let's register the window class
	WNDCLASS hc;

	hc.style = 0;
	hc.lpfnWndProc = (WNDPROC)CHyperlink::WndProc;
	hc.cbClsExtra = 0;
	hc.cbWndExtra = sizeof(CHyperlink*);
	hc.hInstance = NULL;
	hc.hIcon = NULL;
	hc.hCursor = NULL;
	hc.hbrBackground = NULL;
	hc.lpszMenuName = NULL;
	hc.lpszClassName = STATIC_HYPER_WINDOW_CLASS;
	RegisterClass(&hc);


	//Now, try to find a hand icon...
	CHyperlink::handcursor = ::LoadCursor(NULL, MAKEINTRESOURCE(32649)); //32649 == IDC_HAND
	if (CHyperlink::handcursor == NULL)
	{
		TCHAR szWindowsDir[MAX_PATH];
		GetWindowsDirectory(szWindowsDir ,MAX_PATH);
		_tcscat(szWindowsDir,_T("\\winhlp32.exe"));
		hModule = LoadLibrary(szWindowsDir);		
		if (hModule) 
			CHyperlink::handcursor = ::LoadCursor(hModule, MAKEINTRESOURCE(106));
	}
}

CHyperlink::_autoinitializer::~_autoinitializer()
{
	if (hModule != NULL)
		FreeLibrary(hModule);
}

CHyperlink::_autoinitializer CHyperlink::__autoinitializer;
HCURSOR CHyperlink::handcursor = NULL;

CHyperlink::CHyperlink() : m_hWnd(NULL)
{

}

CHyperlink::~CHyperlink()
{
	if (m_hWnd)
		::DestroyWindow(m_hWnd);
}


bool CHyperlink::create(int resourceid, HWND parent)
{
	HWND old = ::GetDlgItem(parent, resourceid);
	if (old != NULL)
	{
		RECT rect; char url[256];

		::GetWindowTextA(old, url, sizeof(url));
		::GetWindowRect(old, &rect);

		//GetWindowRect return bounding box in screen coordinates.
		POINT pos;pos.x = rect.left;pos.y = rect.top;
		rect.left = pos.x; rect.top = pos.y;
		//calculate them down to client coordinates of the according dialog box...
		ScreenToClient(parent, (POINT*)&rect);
		ScreenToClient(parent, ((POINT*)&rect)+1);

		//finally, destroy the old label
		if (create(rect, url, parent))
		::DestroyWindow(old);
	}
	return m_hWnd != NULL;
}

bool CHyperlink::create(RECT rect, const char *url, HWND parent)
{
	if (url != NULL)
		m_Url = url;

	m_hWnd = ::CreateWindowA( STATIC_HYPER_WINDOW_CLASSA, m_Url.c_str(), WS_CHILD | WS_VISIBLE, 
									rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
									parent, NULL, NULL, NULL);

	::SetWindowLong( m_hWnd, GWL_USERDATA, (LONG)this);

	return m_hWnd != NULL;
}
bool CHyperlink::create(int x1, int y1, int x2, int y2, const char *url, HWND parent)
{
	RECT rect; 
	rect.left = x1;
	rect.top = y1;
	rect.right = x2;
	rect.bottom = y2;
	return create(rect, url, parent);
}




int CHyperlink::WndProc(HWND hwnd,WORD wMsg,WPARAM wParam,LPARAM lParam)
{
	CHyperlink *hl = (CHyperlink*)GetWindowLong(hwnd, GWL_USERDATA);

	switch (wMsg)  
	{
	case WM_LBUTTONDOWN:
		if (((UINT)::ShellExecuteA(NULL, "open", hl->m_Url.c_str(), NULL, NULL, SW_SHOWNORMAL)) <= 32)
			MessageBeep(0);
		break;
	case WM_MOUSEMOVE:

		break;
	case WM_PAINT:
		{
			HDC hDC; PAINTSTRUCT ps;
			hDC = ::BeginPaint(hwnd, &ps);
			if (hl == NULL)
				return 0;

			RECT rect;
			::GetClientRect(hwnd, &rect);

			HFONT font = ::CreateFont( 16, //height
										7, //average char width
										0, //angle of escapement
										0, //base-line orientation angle
										FW_NORMAL,	//font weight
										FALSE,		//italic
										TRUE,		//underline
										FALSE,		//strikeout
										ANSI_CHARSET,			//charset identifier
										OUT_DEFAULT_PRECIS,		//ouput precision
										CLIP_DEFAULT_PRECIS,	//clipping precision
										DEFAULT_QUALITY,	//output quality
										DEFAULT_PITCH,			//pitch and family
										L"Arial");
				
			::SelectObject(hDC, font);
			::SetTextColor(hDC, RGB(0,0,200));
			::SetBkMode(hDC, TRANSPARENT);
			::DrawTextA(hDC, hl->m_Url.c_str(), hl->m_Url.length(), &rect, DT_VCENTER | DT_LEFT);
			::DeleteObject(font);

			::EndPaint(hwnd, &ps);
						
			return TRUE;
		}
		case WM_SETCURSOR:
		{
			if (CHyperlink::handcursor)
				::SetCursor(CHyperlink::handcursor);
		}
		default:
			DefWindowProc(hwnd, wMsg, wParam, lParam);
	}

	return TRUE;
}
