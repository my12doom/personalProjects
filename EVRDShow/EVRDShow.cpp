// EVRDShow.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <DShow.h>
#include <atlbase.h>
#include <evr.h>
#include <InitGuid.h>
#include "resource.h"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "mfuuid.lib")

DEFINE_GUID(CLSID_CustomEVRPresenter, 
			0x9707fc9c, 0x807b, 0x41e3, 0x98, 0xa8, 0x75, 0x17, 0x6f, 0x95, 0xa0, 0x62);

//{0B0EFF97-C750-462C-9488-B10E7D87F1A6}
DEFINE_GUID(CLSID_ffdshowDXVA, 
			0x0B0EFF97, 0xC750, 0x462C, 0x94, 0x88, 0xb1, 0x0e, 0x7d, 0x87, 0xf1, 0xa6);


CComPtr<IGraphBuilder> gb;
CComPtr<IMFVideoDisplayControl> display_controll;

INT_PTR CALLBACK window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_INITDIALOG:
		{
			CoInitialize(NULL);

			gb.CoCreateInstance(CLSID_FilterGraph);
			CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);

			CComPtr<IBaseFilter> evr;
			CComPtr<IBaseFilter> ff;
			evr.CoCreateInstance(CLSID_EnhancedVideoRenderer);
			ff.CoCreateInstance(CLSID_ffdshowDXVA);

			CComQIPtr<IMFGetService, &IID_IMFGetService> evr_get(evr);

			CComPtr<IMFVideoPresenter> presenter;
			presenter.CoCreateInstance(CLSID_CustomEVRPresenter);
			gb->AddFilter(evr, L"EVR Renderer");
			CComQIPtr<IMFVideoRenderer, &IID_IMFVideoRenderer> evr_mf(evr);
			evr_mf->InitializeRenderer(NULL, presenter);



			RECT client = {0,0,1920,1080};
 			GetClientRect(hDlg, &client);
			evr_get->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (void**)&display_controll);
			display_controll->SetVideoWindow(hDlg);
			display_controll->SetVideoPosition(NULL, &client);



 			gb->AddFilter(ff, L"ff DXVA");


			gb->RenderFile(L"Z:\\mkvass.mkv", NULL);
			mc->Run();
		}
		break;



	case WM_CLOSE:
		EndDialog(hDlg, -1);
		break;

	case WM_SIZE:
		{
// 			RECT client = {0,0,1920,1080};
// 			GetClientRect(hDlg, &client);
// 			display_controll->SetVideoPosition(NULL, &client);
		}
		break;


	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

int _tmain(int argc, _TCHAR* argv[])
{
	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, window_proc);


	return 0;
}

