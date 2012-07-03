#include "SampleEngine.h"
#include <stdio.h>


HWND g_hWnd;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
				   LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int n = 128;

class TestObject : public D3D9UIObject
{
public:
	TestObject(ID3D9UIEngine *engine): D3D9UIObject(engine)
	{
		BaseUIFontDesc fd = {
			15, 0, FW_BOLD, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, 
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial"};

		memset (&m_rect, 0, sizeof(m_rect));
		m_res2 = m_res = NULL;
		m_engine->LoadResource(&m_res, L"512.png", ResourceType_FileImage);
		m_engine->LoadResource(&m_res2, &fd, ResourceType_Font);
	}
	~TestObject()
	{
		if (m_res)
			delete m_res;
		if (m_res2)
			delete m_res2;
	}
	HRESULT RenderThis(bool HitTest)
	{
		ID3D9UIObject *hit = NULL;
		m_engine->Hittest(&hit);

		int id = 0;
		bool mouseover = false;
		if (hit && hit->GetID(&id) == S_OK && id == m_id)
			mouseover = true;

		m_engine->Draw(m_res, NULL, &m_rect, HitTest ? m_id : -1);

		if (!HitTest)
		m_engine->DrawText(m_res2, mouseover ? L"Hello!" : L"...", &m_rect, NULL, 0xffffffff);


		return S_OK;
	}

	void set_rect(RECT rect)
	{
		m_rect = rect;
	}

	ID3D9UIResource *m_res, *m_res2;
	RECT m_rect;
};

ID3D9UIEngine *engine;
TestObject *obj;
TestObject *obj2;

int main()
{
	WNDCLASSEXW winClass; 
	MSG        uMsg;

	memset(&uMsg,0,sizeof(uMsg));

	winClass.lpszClassName = L"MY_WINDOWS_CLASS";
	winClass.cbSize        = sizeof(WNDCLASSEX);
	winClass.style         = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc   = WindowProc;
	winClass.hInstance     = NULL;
 	winClass.hIcon         = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
 	winClass.hIconSm       = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
	winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = NULL;
	winClass.lpszMenuName  = NULL;
	winClass.cbClsExtra    = 0;
	winClass.cbWndExtra    = 0;

	if( !RegisterClassExW(&winClass) )
		return E_FAIL;

	g_hWnd = CreateWindowExW( NULL, L"MY_WINDOWS_CLASS", 
		L"D3D9UIEngine Test",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, 640, 480, NULL, NULL, NULL, NULL );

	if( g_hWnd == NULL )
		return E_FAIL;

	ShowWindow( g_hWnd, SW_SHOW );
	UpdateWindow( g_hWnd );
	SetTimer(g_hWnd, 1, 100, NULL);

	engine = create_engine(g_hWnd);
	obj = new TestObject(engine);
	obj2 = new TestObject(engine);

	RECT dst = {100,100,100+128,100+(float)n/5};
	RECT dst2 = {150,150,150+128,150+(float)n/5};

	obj->set_rect(dst);
	obj2->set_rect(dst2);


	ID3D9UIObject *root = NULL;
	if (SUCCEEDED(engine->GetUIRoot(&root)) && root)
	{
		root->AddChild(obj);
		root->AddChild(obj2);
	}

	while( uMsg.message != WM_QUIT )
	{
		if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
		{ 
			TranslateMessage( &uMsg );
			DispatchMessage( &uMsg );
		}
		else
			if (engine->HandleDeviceState() == S_FALSE)
				Sleep(1);
	}

	delete engine;
	delete obj;
	delete obj2;

	UnregisterClassW( L"MY_WINDOWS_CLASS", winClass.hInstance );

	return 0;
}

bool dragging = false;
int y = 0;
LRESULT CALLBACK WindowProc( HWND   hWnd,
							UINT   msg,
							WPARAM wParam,
							LPARAM lParam )
{
	switch( msg )
	{
	case WM_TIMER:
		{
			char tmp[256];
			int id = GetCurrentThreadId();
			sprintf(tmp, "%d\n", id);
			//OutputDebugStringA(tmp);
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

	case WM_LBUTTONDOWN:
		dragging = true;
		y = HIWORD(lParam) + n;
		break;
	
	case WM_LBUTTONUP:
		dragging = false;
		break;

	case WM_MOUSEMOVE:
		if (dragging)
		{
			n = y - HIWORD(lParam);

			RECT dst = {100,100,100+128,100+(float)n/5};
			RECT dst2 = {150,150,150+128,150+(float)n/5};

			obj->set_rect(dst);
			obj2->set_rect(dst2);
		}
		break;

	case WM_SIZE:
		{
			//engine->RenderNow();
		}
		break;

	case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
				PostQuitMessage(0);
			else if (wParam == VK_UP)
				n++;
			else if (wParam == VK_DOWN)
				n--;
		}
		break;

	default:
		{
			return DefWindowProc( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return 0;
}