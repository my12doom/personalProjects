#include <tchar.h>
#include <stdio.h>
#include "CVideoSwitcher.h"


int main()
{
	CoInitialize(NULL);

	HRESULT hrr;
	CVideoSwitcher * s2sbs = new CVideoSwitcher(_T("S2SBS"), NULL, &hrr);
	CComQIPtr<IBaseFilter, &IID_IBaseFilter> s2sbs_base(s2sbs);

	CComPtr<IGraphBuilder> gb;
	gb.CoCreateInstance(CLSID_FilterGraph);

	gb->AddFilter(s2sbs_base, L"s2sbs");

	//gb->RenderFile(L"F:\\TDDownload\\3840.ts", NULL);
	//gb->RenderFile(L"F:\\TDDownload\\Leona.Lewis.-.[I.See.You].MV.(720P).mkv", NULL);
	gb->RenderFile(L"E:\\test_folder\\test-001.mkv", NULL);
	gb->RenderFile(L"E:\\test_folder\\test-002.mkv", NULL);

	CComPtr<IEnumPins> ep;
	s2sbs_base->EnumPins(&ep);
	CComPtr<IPin> pin;
	while(ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_INFO pi;
		pin->QueryPinInfo(&pi);

		if (pi.pFilter)
			pi.pFilter->Release();

		wprintf(pi.achName);

		CComPtr<IPin> to;
		if(pin->ConnectedTo(&to) == S_OK)
		{
			PIN_INFO pi;
			to->QueryPinInfo(&pi);

			FILTER_INFO fi;
			pi.pFilter->QueryFilterInfo(&fi);
			if (fi.pGraph)
				fi.pGraph->Release();

			if (pi.pFilter)
				pi.pFilter->Release();

			wprintf(L"(connected to %s)", fi.achName);
		}

		printf("\n");

		pin = NULL;
	}


	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();
	while(true)
		Sleep(1);

	CoUninitialize();
	return 0;
}