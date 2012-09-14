#include <time.h>
#include "global_funcs.h"
#include <Shlobj.h>
#include <streams.h>
#include <intrin.h>
#include "detours/detours.h"
#include "..\AESFile\E3DReader.h"
#include <initguid.h>
#include "LAVAudioSettings.h"


#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "detours/detours.lib")

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

#if 0
char *g_server_address = "http://127.0.0.1/";
#else
char *g_server_address = "http://bo3d.net:80/";
#endif

// public variables
AutoSetting<localization_language> g_active_language(L"Language", get_system_default_lang(), REG_DWORD);
AutoSettingString g_bar_server(L"BarServer", L"");
char g_passkey[32];
char g_passkey_big[128];
DWORD g_last_bar_time;
char g_ad_address[512] = {0};


int g_logic_monitor_count;
RECT g_logic_monitor_rects[16];
int n_monitor_found = 0;
RECT monitor_rect[MAX_MONITORS];
BOOL CALLBACK monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	memcpy(monitor_rect+n_monitor_found, lprcMonitor, sizeof(RECT));
	n_monitor_found ++;

	if (n_monitor_found >= MAX_MONITORS)
		return FALSE;
	else
		return TRUE;
}

HRESULT split_span_rect(const RECT in, RECT *out1, RECT *out2)
{
	int width = in.right - in.left;
	int height = in.bottom - in.top;
	double aspect = (double)width / height;
	if (aspect > 2.425)
	{
		// horizontal span
		*out1 = in;
		*out2 = in;
		out1->right -= width/2;
		out2->left += width/2;

		return S_OK;
	}
	else if (0 < aspect && aspect < 1.2125)
	{
		// vertical span
		*out1 = in;
		*out2 = in;
		out1->bottom -= height/2;
		out2->top += height/2;

		return S_OK;
	}
	else
	{
		// normal aspect
		return E_FAIL;
	}
}

int find_phy_monitors()
{
	CoInitialize(NULL);
	CComPtr<IDirect3D9> d3d;
	d3d = Direct3DCreate9( D3D_SDK_VERSION );
	if (!d3d)
		return 0;

	unsigned int i = 0;
	for(i=0; i<min(16, d3d->GetAdapterCount()); i++)
	{
		d3d->GetAdapterIdentifier(i, NULL, g_phy_ids+i);
		g_phy_monitors[i] = d3d->GetAdapterMonitor(i);
	}
	d3d = NULL;
	CoUninitialize();
	return i;
}

int g_phy_monitor_count = find_phy_monitors();
HMONITOR g_phy_monitors[16];
HMONITOR g_logic_monitors[16];
D3DADAPTER_IDENTIFIER9 g_phy_ids[16];
D3DADAPTER_IDENTIFIER9 g_logic_ids[16];


HRESULT get_monitors_rect(RECT *screen1, RECT *screen2)
{
	if (screen1 == NULL || screen2 == NULL)
		return E_POINTER;

	for(int i=0; i<n_monitor_found; i++)
		if (SUCCEEDED(split_span_rect(monitor_rect[i], screen1, screen2)))
			return S_OK;

	*screen1 = monitor_rect[0];
	*screen2 = monitor_rect[n_monitor_found-1];

	return S_OK;
}

HRESULT detect_monitors()
{
	g_phy_monitor_count = find_phy_monitors();
	g_logic_monitor_count = 0;

	for(int i=0; i<g_phy_monitor_count; i++)
	{
		MONITORINFOEX info;
		info.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(g_phy_monitors[i], &info);

		RECT &rect = info.rcMonitor;
		RECT rect1, rect2;
		if (FAILED(split_span_rect(rect, &rect1, &rect2)))
		{
			g_logic_monitor_rects[g_logic_monitor_count] = rect;
			g_logic_monitors[g_logic_monitor_count] = g_phy_monitors[i];
			g_logic_ids[g_logic_monitor_count++] = g_phy_ids[i];
		}
		else
		{
			g_logic_monitor_rects[g_logic_monitor_count] = rect1;
			g_logic_monitors[g_logic_monitor_count] = g_phy_monitors[i];
			g_logic_ids[g_logic_monitor_count++] = g_phy_ids[i];

			g_logic_monitor_rects[g_logic_monitor_count] = rect2;
			g_logic_monitors[g_logic_monitor_count] = g_phy_monitors[i];
			g_logic_ids[g_logic_monitor_count++] = g_phy_ids[i];
		}
	}

	return S_OK;
}

int get_mixed_monitor_count(bool horizontal /* = false */, bool vertical /* = false */)
{
	for(int i=0; i<32; i++)
		if (get_mixed_monitor_by_id(i, NULL, NULL, horizontal, vertical) != 0)
			return i;
	return 0;
}

int get_mixed_monitor_by_id(int id, RECT *rect, wchar_t *descriptor, bool horizontal /* = false */, bool vertical /* = false */)
{
	int counter = 0;

	if (id < 0)
		return -1;

	// return on direct logical match
	if (id < g_logic_monitor_count)
	{
		if (rect)
			*rect = g_logic_monitor_rects[id];

		if (descriptor)
		{
			RECT rect = g_logic_monitor_rects[id];
			wsprintfW(descriptor, L"%s %d (%dx%d)", C(L"Monitor"), id+1, rect.right - rect.left, rect.bottom - rect.top);
		}

		return 0;
	}

	// find neighbor monitors
	for(int i=0; i<g_logic_monitor_count; i++)
		for(int j=i+1; j<g_logic_monitor_count; j++)
		{
			bool neighbor = false;
			RECT &r1 = g_logic_monitor_rects[i];
			RECT &r2 = g_logic_monitor_rects[j];
			if (horizontal)
			{
				if ((r1.right == r2.left || r2.right == r1.left) &&			// neighbor
					 (r1.bottom - r1.top == r2.bottom - r2.top) &&			// same height
					 (r1.right - r1.left == r2.right - r2.left))			// same width
					 neighbor = true;
			}

			if (vertical)
			{
				if ((r1.top == r2.bottom || r2.top == r1.bottom) &&			// neighbor
					(r1.bottom - r1.top == r2.bottom - r2.top) &&			// same height
					(r1.right - r1.left == r2.right - r2.left))				// same width
					neighbor = true;
			}

			if (neighbor)
			{
				if (counter == id - g_logic_monitor_count)
				{
					if (rect)
					{
						rect->left = min(r1.left, r2.left);
						rect->right = max(r1.right, r2.right);
						rect->top = min(r1.top, r2.top);
						rect->bottom = max(r1.bottom, r2.bottom);
					}

					if (descriptor)
					{
						RECT rect = g_logic_monitor_rects[id];
						wsprintfW(descriptor, L"%s %d+%d (%dx%dx2)", C(L"Monitor"), i+1, j+1, r1.right - r1.left, r2.bottom - r2.top);
					}

					return 0;
				}

				counter++;
			}
		}

	return -1;
}

bool browse_folder(wchar_t *out, HWND owner/* = NULL*/)
{
	BROWSEINFOW b;
	b.hwndOwner = owner;
	b.pidlRoot = NULL;
	b.pszDisplayName = out;
	b.lpszTitle = C(L"Select Folder..");
	b.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN ;
	b.lpfn = NULL;
	b.lParam = 0;
	b.iImage = 0;

	PIDLIST_ABSOLUTE pid = SHBrowseForFolderW(&b);
	if (pid != NULL)
	{
		return SHGetPathFromIDListW(pid, out);
	}
	return false;
}

bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter/* = NULL*/)
{
	static wchar_t *default_filter = L"Video files\0"
		L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d\0"
		L"Audio files\0"
		L"*.wav;*.ac3;*.dts;*.mp3;*.mp2;*.mpa;*.mp4;*.wma;*.flac;*.ape;*.avs\0"
		L"Subtitles\0*.srt;*.sup\0"
		L"All Files\0*.*\0"
		L"\0";
	if (filter == NULL) filter = default_filter;
	wchar_t strFileName[MAX_PATH] = L"";
	wchar_t strPath[MAX_PATH] = L"";

	wcsncpy(strFileName, pathname, MAX_PATH);
	wcsncpy(strPath, pathname, MAX_PATH);
	for(int i=(int)wcslen(strPath)-2; i>=0; i--)
		if (strPath[i] == L'\\')
		{
			strPath[i+1] = NULL;
			break;
		}

	OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW), hDlg , NULL,
		filter, NULL,
		0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
		C(L"Open File"),
		OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING, 0, 0,
		L".mp4", 0, NULL, NULL };

	int o = GetOpenFileNameW( &ofn );
	if (o)
	{
		wcsncpy(pathname, strFileName, MAX_PATH);
		return true;
	}

	return false;
}

bool select_color(DWORD *color, HWND parent)
{
	static COLORREF customColor[16];
	customColor[0] = *color;
	CHOOSECOLOR cc = {sizeof(CHOOSECOLOR), parent, NULL, *color, customColor, CC_RGBINIT, 0, NULL, NULL};
	bool rtn = ChooseColor(&cc);

	if (rtn)
		*color = cc.rgbResult;

	return rtn;
}

HRESULT RemoveDownstream(CComPtr<IPin> &input_pin)
{
	// check pin
	PIN_DIRECTION pd = PINDIR_OUTPUT;
	input_pin->QueryDirection(&pd);
	if (pd != PINDIR_INPUT)
		return E_FAIL;

	// run RemoveDownstream on each pin
	PIN_INFO pi;
	input_pin->QueryPinInfo(&pi);
	CComPtr<IBaseFilter> filter;
	filter.Attach(pi.pFilter);


	CComPtr<IEnumPins> ep;
	filter->EnumPins(&ep);
	CComPtr<IPin> pin;
	while (S_OK == ep->Next(1, &pin, NULL))
	{
		pin->QueryDirection(&pd);
		if (pd == PINDIR_OUTPUT)
		{
			CComPtr<IPin> connected;
			pin->ConnectedTo(&connected);
			if (connected)
				RemoveDownstream(connected);
		}

		pin = NULL;
	}

	// remove the filter
	FILTER_INFO fi;
	filter->QueryFilterInfo(&fi);
	CComPtr<IFilterGraph> graph;
	graph.Attach(fi.pGraph);
	graph->RemoveFilter(filter);

	return S_OK;
}

HRESULT ReadPlaylist(const wchar_t *strPlaylistFile, REFERENCE_TIME *rtDuration)
{
	DWORD read;
	BYTE Buff[100];
	bool bDuplicate = false;
	*rtDuration  = 0;

	HANDLE m_hFile   = CreateFileW(strPlaylistFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		ReadFile(m_hFile, Buff, 4, &read, NULL);
		if (memcmp (Buff, "MPLS", 4))
		{
			CloseHandle(m_hFile);
			return VFW_E_INVALID_FILE_FORMAT;
		}

		ReadFile(m_hFile, Buff, 4, &read, NULL);
		if ((memcmp (Buff, "0200", 4)!=0) && (memcmp (Buff, "0100", 4)!=0))
		{
			CloseHandle(m_hFile);
			return VFW_E_INVALID_FILE_FORMAT;
		}

		DWORD				dwPos;
		DWORD				dwTemp;
		WORD				nPlaylistItems;

		ReadFile(m_hFile, Buff, 4, &read, NULL);
		dwPos = (Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3];
		SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);

		SetFilePointer(m_hFile, 4, NULL, SEEK_CUR);
		SetFilePointer(m_hFile, 2, NULL, SEEK_CUR);
		ReadFile(m_hFile, Buff, 2, &read, NULL);
		nPlaylistItems = (Buff[0] << 8) + Buff[1];
		SetFilePointer(m_hFile, 2, NULL, SEEK_CUR);

		dwPos	  += 10;
		int playlistitems[100];
		int n_playlistitems = 0;
		for (DWORD i=0; i<nPlaylistItems; i++)
		{
			SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);
			ReadFile(m_hFile, Buff, 2, &read, NULL);
			dwPos += (Buff[0] << 8) + Buff[1] + 2;
			ReadFile(m_hFile, Buff, 5, &read, NULL);
			Buff[5] = NULL;

			// check duplication
			bool duplicate = false;
			int n = atoi((char*)Buff);			
			for(int j=0; j<n_playlistitems; j++)
				if (playlistitems[j] == n)
					duplicate = true;

			if(!duplicate) playlistitems[n_playlistitems++] = n;

			// TODO: save result

			SetFilePointer(m_hFile, 4, NULL, SEEK_CUR);
			SetFilePointer(m_hFile, 3, NULL, SEEK_CUR);

			ReadFile(m_hFile, Buff, 4, &read, NULL);
			dwTemp	= (Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3];

			ReadFile(m_hFile, Buff, 4, &read, NULL);
			if (!duplicate) *rtDuration += (REFERENCE_TIME)((Buff[0] << 24) + (Buff[1] << 16) + (Buff[2] << 8) + Buff[3] - dwTemp) * 20000 / 90;
		}

		CloseHandle(m_hFile);
		return bDuplicate ? S_FALSE : S_OK;
	}

	return AmHresultFromWin32(GetLastError());
}

HRESULT find_main_movie(const wchar_t *folder, wchar_t *out)
{
	HRESULT	hr	= E_FAIL;

	wchar_t tmp_folder[MAX_PATH];
	wcscpy(tmp_folder, folder);
	if (tmp_folder[wcslen(tmp_folder)-1] == L'\\')
		tmp_folder[wcslen(tmp_folder)-1] = NULL;

	wchar_t	strFilter[MAX_PATH];
	wsprintfW(strFilter, L"%s\\BDMV\\PLAYLIST\\*.mpls", tmp_folder);

	WIN32_FIND_DATAW fd = {0};
	HANDLE hFind = FindFirstFileW(strFilter, &fd);
	if(hFind != INVALID_HANDLE_VALUE)
	{
		REFERENCE_TIME		rtMax	= 0;
		REFERENCE_TIME		rtCurrent;
		wchar_t	strCurrentPlaylist[MAX_PATH];
		do
		{
			wsprintfW(strCurrentPlaylist, L"%s\\BDMV\\PLAYLIST\\%s", tmp_folder, fd.cFileName);

			// Main movie shouldn't have duplicate files...
			if (ReadPlaylist(strCurrentPlaylist, &rtCurrent) == S_OK && rtCurrent > rtMax)
			{
				rtMax			= rtCurrent;

				wcscpy(out, strCurrentPlaylist);
				hr				= S_OK;
			}
		}
		while(FindNextFileW(hFind, &fd));

		FindClose(hFind);
	}

	return hr;
}

HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr))  // Already connected, not the pin we want.
			{
				pTmp->Release();
			}
			else  // Unconnected, this is the pin we want.
			{
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr))
	{
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK)
	{
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir)
		{
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr)) // Connected, this is the pin we want. 
			{
				pTmp->Release();
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
			else  // Unconnected, not the pin we want.
			{
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	// Did not find a matching pin.
	return E_FAIL;
}

// CoreMVC
HRESULT write_property(IPropertyBag *bag, const wchar_t *property_to_write)
{
	VARIANT var;
	var.vt = VT_BSTR;
	var.bstrVal = NULL;
	if (property_to_write)
		var.bstrVal =  SysAllocString(property_to_write);

	HRESULT hr = bag->Write(L"Settings", &var);
	if (property_to_write)
		SysFreeString(var.bstrVal);

	return hr;
}
// hook
// static bool coremvc_hooked = false;
// static DWORD (WINAPI *TrueGetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize) = GetModuleFileNameA;
// DWORD WINAPI MineGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
// {
// 	DWORD rtn = TrueGetModuleFileNameA(hModule, lpFilename, nSize);
// 	if (hModule == NULL)
// 		strcat(lpFilename, ".StereoPlayer.exe.MVCToAVI.exe");
// 	return rtn;
//}

HRESULT beforeCreateCoreMVC()
{
// 	if (!coremvc_hooked)
// 	{
// 		// apihook
// 		DetourRestoreAfterWith();
// 		DetourTransactionBegin();
// 
// 		int id = GetCurrentThreadId();
// 		DetourUpdateThread(GetCurrentThread());
// 		DetourAttach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);
// 		LONG error = DetourTransactionCommit();
// 		coremvc_hooked = true;
// 	}

	return S_OK;
}

HRESULT afterCreateCoreMVC()
{
// 	if (coremvc_hooked)
// 	{
// 		// apihook
// 		DetourRestoreAfterWith();
// 		DetourTransactionBegin();
// 
// 		int id = GetCurrentThreadId();
// 		DetourUpdateThread(GetCurrentThread());
// 		DetourDetach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);
// 		LONG error = DetourTransactionCommit();
// 		coremvc_hooked = false;
// 	}

	return S_OK;
}

wchar_t g_apppath[MAX_PATH];
wchar_t *get_apppath()
{
	GetModuleFileNameW(NULL, g_apppath, MAX_PATH);
	for(int i=wcslen(g_apppath); i>0; i--)
		if (g_apppath[i] == L'\\')
		{
			g_apppath[i+1] = NULL;
			break;
		}
	return g_apppath;
}
wchar_t *apppath = get_apppath();

coremvc_hooker::coremvc_hooker()
{
	beforeCreateCoreMVC();
}

coremvc_hooker::~coremvc_hooker()
{
	afterCreateCoreMVC();
}

AutoSetting<bool> g_CUDA(L"CUDA", false);
HRESULT ActiveCoreMVC(IBaseFilter *decoder)
{
	CComQIPtr<IPropertyBag, &IID_IPropertyBag> pbag(decoder);
	if (pbag)
	{
		HRESULT hr = write_property(pbag, L"use_tray=0");
		hr = write_property(pbag, L"low_latency=0");
		hr = write_property(pbag, L"di=6");
		hr = write_property(pbag, g_CUDA ? L"use_cuda=1" : L"use_cuda=0");
		return write_property(pbag, L"app_mode=1");
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT set_lav_audio_bitstreaming(IBaseFilter *filter, bool active)
{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<ILAVAudioSettings, &IID_ILAVAudioSettings> setting(filter);
	if (setting == NULL)
		return E_NOINTERFACE;

	HRESULT hr = S_OK;
	hr = setting->SetBitstreamConfig(Bitstream_AC3, active);
	hr = setting->SetBitstreamConfig(Bitstream_EAC3, active);
	hr = setting->SetBitstreamConfig(Bitstream_TRUEHD, active);
	hr = setting->SetBitstreamConfig(Bitstream_DTS, active);
	hr = setting->SetBitstreamConfig(Bitstream_DTSHD, active);

	return hr;
}

HRESULT set_ff_audio_bitstreaming(IBaseFilter *filter, bool active)
{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<IffdshowBaseA, &IID_IffdshowBaseA> cfg(filter);
	if (NULL == cfg)
		return E_NOINTERFACE;

	HRESULT hr = S_OK;
	int enable = active ? 1 : 0;
	//hr = cfg->putParam(IDFF_aoutAC3EncodeMode, enable);
	hr = cfg->putParam(IDFF_aoutpassthroughAC3, enable);
	hr = cfg->putParam(IDFF_aoutpassthroughDTS, enable);
	hr = cfg->putParam(IDFF_aoutpassthroughTRUEHD, enable);
	hr = cfg->putParam(IDFF_aoutpassthroughDTSHD, enable);
	hr = cfg->putParam(IDFF_aoutpassthroughEAC3, enable);
	hr = cfg->putParam(IDFF_aoutAC3EncodeMode, enable);

	return hr;
}

HRESULT set_ff_audio_normalizing(IBaseFilter *filter, double max_ratio)	// setting a ratio less than 1.0 cause normalizing off, more than 1.0 cause normalizing on
{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<IffdshowBaseW, &IID_IffdshowBaseW> cfg(filter);

	if (NULL == cfg)
		return E_NOINTERFACE;

	HRESULT hr = S_OK;
	if (max_ratio < 1.0 )
		return hr = cfg->putParam(IDFF_volumeNormalize, 0);

	hr = cfg->putParam(IDFF_volumeNormalize, 1);
	hr = cfg->putParam(IDFF_maxNormalization, max_ratio*100);

	return S_OK;
}

HRESULT set_ff_output_channel(IBaseFilter *filter, int channel)	// channel=0: no downmixing, channel=9 : headphone, channel=10 : HRTF, channel=-1: bitstreaming

{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<IffdshowBaseW, &IID_IffdshowBaseW> cfg(filter);

	if (NULL == cfg)
		return E_NOINTERFACE;

	HRESULT hr = S_OK;
	if (channel == 0 || channel > 10)
	{
		hr = set_ff_audio_bitstreaming(filter, false);
		return hr = cfg->putParam(IDFF_isMixer, 0);
	}
	if (channel == -1)
	{
		hr = set_ff_audio_bitstreaming(filter, true);
		hr = cfg->putParam(IDFF_isMixer, 0);
		return hr;
	}

	/*
	0:mono		:0
	1:headphone	:17
	2:stereo	:1
	3:3.0		:2
	4:4.1		:12
	5:5.0		:6
	6:5,1		:13
	7:7.1		:24
	8:off		:
	*/

	hr = cfg->putParam(IDFF_isMixer, 1);
	switch (channel)
	{
	case 1:
		return hr = cfg->putParam(IDFF_mixerOut, 0);
	case 2:
		return hr = cfg->putParam(IDFF_mixerOut, 1);
	case 3:
	case 4:
		return hr = cfg->putParam(IDFF_mixerOut, 2);
	case 5:
		return hr = cfg->putParam(IDFF_mixerOut, 6);
	case 6:
		return hr = cfg->putParam(IDFF_mixerOut, 13);
	case 7:
	case 8:
		return hr = cfg->putParam(IDFF_mixerOut, 24);
	case 9:
		return hr = cfg->putParam(IDFF_mixerOut, 17);
	}

	return hr;
}

HRESULT set_ff_audio_formats(IBaseFilter *filter)
{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<IffdshowBaseW, &IID_IffdshowBaseW> cfg(filter);

	if (NULL == cfg)
		return E_NOINTERFACE;


	HRESULT hr = S_OK;

	hr = cfg->putParam(IDFF_trayIcon, 0);

	hr = cfg->putParam(IDFF_mp3, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mp2, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_ac3, IDFF_MOVIE_LIBA52);
	hr = cfg->putParam(IDFF_eac3, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_truehd, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mlp, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_dts, IDFF_MOVIE_LIBDTS);
	hr = cfg->putParam(IDFF_aac, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vorbis, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_amr, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_flac, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_tta, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_lpcm, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_ra, IDFF_MOVIE_NONE);

	return hr;
}

HRESULT set_ff_video_formats(IBaseFilter *filter)
{
	if (filter == NULL)
		return E_POINTER;

	CComQIPtr<IffdshowBaseW, &IID_IffdshowBaseW> cfg(filter);

	if (NULL == cfg)
		return E_NOINTERFACE;

	HRESULT hr = S_OK;

	hr = cfg->putParam(IDFF_trayIcon, 0);

	hr = cfg->putParam(IDFF_h264, IDFF_MOVIE_LAVC);		// this is mainly for non-private files, use whatever I can
	hr = cfg->putParam(IDFF_xvid, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_dx50, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mp41, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mp42, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mp43, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mp4v, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_div3, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_div3, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_h263, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_flv1, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_theo, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vp3, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vp5, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vp6, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vp6f, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_vp8, IDFF_MOVIE_LAVC);

	hr = cfg->putParam(IDFF_mpg1, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mpg2, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_mpegAVI, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_em2v, IDFF_MOVIE_LAVC);
	//hr = cfg->putParam(IDFF_vcr1, IDFF_MOVIE_LAVC);
	hr = cfg->putParam(IDFF_h261, IDFF_MOVIE_LAVC);


	return hr;
}


bool isUselessFilter(IBaseFilter *filter)
{
	bool has_output = false;
	bool connected = false;
	bool has_connected_input = false;
	bool has_connected_output = false;

	CComPtr<IEnumPins> ep;
	CComPtr<IPin> pin;
	filter->EnumPins(&ep);
	while (ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_DIRECTION pd;
		pin->QueryDirection(&pd);
		CComPtr<IPin> tmp;
		pin->ConnectedTo(&tmp);

		if (tmp != NULL)
			connected = true;
		if (pd == PINDIR_OUTPUT)
			has_output = true;
		if (pd == PINDIR_OUTPUT && tmp != NULL)
			has_connected_output = true;
		if (pd == PINDIR_INPUT && tmp != NULL)
			has_connected_input = true;

		pin = NULL;
	}

	if (!connected)
		return true;
	else if (has_output && !has_connected_output)
		return true;
	else if (has_connected_input && !has_output)
		return false;
	else if (has_connected_output)
		return false;
	else
		return true;
}

HRESULT RemoveUselessFilters(IGraphBuilder *gb)
{
	CComPtr<IEnumFilters> ef;
	gb->EnumFilters(&ef);

	CComPtr<IBaseFilter> filter;
	while (ef->Next(1, &filter, NULL) == S_OK)
	{
		if (isUselessFilter(filter))
		{
			FILTER_INFO fi;
			filter->QueryFilterInfo(&fi);
			if (fi.pGraph) fi.pGraph->Release();

			wprintf(L"Removed %s...\n", fi.achName);
			gb->RemoveFilter(filter);
			ef->Reset();
		}

		filter = NULL;
	}

	return S_OK;
}

HRESULT GetPinByName(IBaseFilter *pFilter, PIN_DIRECTION PinDir, const wchar_t *name, IPin **ppPin)
{
	*ppPin = NULL;
	CComPtr<IEnumPins> ep;
	pFilter->EnumPins(&ep);
	CComPtr<IPin> pin;
	while (ep->Next(1, &pin, NULL) == S_OK)
	{
		PIN_INFO pi;
		pin->QueryPinInfo(&pi);

		if (pi.pFilter)
			pi.pFilter->Release();

		PIN_DIRECTION ThisPinDir;
		pin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir && wcsstr(pi.achName, name))
		{
			*ppPin = pin;
			(*ppPin)->AddRef();
			return S_OK;
		}

		pin = NULL;
	}

	return E_FAIL;
}

__time64_t mytime(bool reset)
{
	const char* key_word = "DWindow's Kernel Corrupted.";
	__time64_t current_time =  _time64(NULL);

	char key[256];
	memset(key, 0, 256);
	strcpy(key, key_word);
	AESCryptor codec;
	codec.set_key((const unsigned char*)key, 256);
	DWORD e[32];
	int got = load_D3D_setting(L"Flags", e, 128);
	for(int i=0; i<128; i+=16)
		codec.decrypt(((unsigned char*)e) + i, ((unsigned char*)e) + i);
	if (got == 128 && *(__time64_t*)e > current_time)
		current_time = 0xffffffff;

	if (reset)
		current_time = _time64(NULL);

	srand(time(NULL));
	for(int i=0; i<128; i++)
		((BYTE*)e)[i] = rand() & 0xff;
	memcpy(e, &current_time, 8);
	for(int i=0; i<128; i+=16)
		codec.encrypt( ((unsigned char*)e) + i, ((unsigned char*)e) + i);
	save_D3D_setting(L"Flags", e, 128);

	return current_time;
}

HRESULT load_passkey()
{
	int tmp[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(tmp, 0x80000002);
	memcpy(CPUBrandString, tmp, 16);
	__cpuid(tmp, 0x80000003);
	memcpy(CPUBrandString+16, tmp, 16);
	__cpuid(tmp, 0x80000004);
	memcpy(CPUBrandString+32, tmp, 16);
	memset(CPUBrandString + strlen((char*)CPUBrandString), 0, 48-strlen((char*)CPUBrandString));
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];

	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn;


	if (g_bar_server[0])
	{
		USES_CONVERSION;

		// bar mode init
		DWORD tick = GetTickCount();
		for(int i=0; i<8; i++)
			((DWORD*)CPUBrandString)[i] ^= tick;

		char url[1024] = "http://";
		char tmp[10];
		strcat_s(url, W2A(g_bar_server));
		strcat_s(url, "/");
		for(int i=0; i<32; i++)
		{
			sprintf(tmp, "%02X", CPUBrandString[i]);
			strcat_s(url, tmp);
		}
	
		char downloaded[1024];
		memset(downloaded, 0, sizeof(downloaded));
		download_url(url, downloaded, 1024, 3000);
		unsigned char encoded_passkey_big[128+10];

		HRESULT hr = E_FAIL;
		if (strlen(downloaded) == 256)
		{
			// binary it
			for(int i=0; i<128; i++)
				sscanf(downloaded+i*2, "%02X", encoded_passkey_big+i);

			// decrypt it
			for(unsigned int i=0; i<256; i++)
			{
				unsigned char key[32];
				memcpy(key, CPUBrandString, 32);
				for(int j=0; j<32; j++)
					key[j] ^= i&0xff;

				AESCryptor aes;
				aes.set_key(key, 256);
				for(int j=0; j<128; j+=16)
					aes.decrypt(encoded_passkey_big+j, (unsigned char*)g_passkey_big+j);

				hr = check_passkey();
				if (SUCCEEDED(hr))
				{
					g_last_bar_time = GetTickCount();
					return hr;
				}
			}
		}

		return hr;
	}
	else
	{
		// normal init

		AESCryptor aes;
		aes.set_key(CPUBrandString, 256);

		memset(g_passkey_big, 0x38, 128);
		load_setting(L"passkey", g_passkey_big, 128);
		for(int i=0; i<128; i+=16)
			aes.decrypt((unsigned char*)g_passkey_big+i, (unsigned char*)g_passkey_big+i);
	}

	return check_passkey();
}

HRESULT save_passkey()
{
	int tmp[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(tmp, 0x80000002);
	memcpy(CPUBrandString, tmp, 16);
	__cpuid(tmp, 0x80000003);
	memcpy(CPUBrandString+16, tmp, 16);
	__cpuid(tmp, 0x80000004);
	memcpy(CPUBrandString+32, tmp, 16);
	memset(CPUBrandString + strlen((char*)CPUBrandString), 0, 48-strlen((char*)CPUBrandString));
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];

	mytime();

	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn ^ g_bar_server[0];

	AESCryptor aes;
	aes.set_key(CPUBrandString, 256);
	for(int i=0; i<128; i+=16)
		aes.encrypt((unsigned char*)g_passkey_big+i, (unsigned char*)g_passkey_big+i);

	save_setting(L"passkey", g_passkey_big, 128);



	for(int i=0; i<128; i+=16)
		aes.decrypt((unsigned char*)g_passkey_big+i, (unsigned char*)g_passkey_big+i);
	return S_OK;
}

HRESULT load_e3d_key(const unsigned char *file_hash, unsigned char *file_key)
{
	unsigned char key_tmp[32+16];
	wchar_t tmp[3] = L"";
	wchar_t reg_key[41] = L"";
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", file_hash[i]);
		wcscat(reg_key, tmp);
	}

	load_setting(reg_key, key_tmp, 32+16);

	// AES it
	AESCryptor codec;
	codec.set_key((unsigned char*)g_passkey, 256);
	codec.decrypt((unsigned char*)key_tmp, (unsigned char*)key_tmp);
	codec.decrypt((unsigned char*)key_tmp+16, (unsigned char*)key_tmp+16);
	codec.decrypt((unsigned char*)key_tmp+32, (unsigned char*)key_tmp+32);

	// time
	__time64_t time = mytime();
	__time64_t key_start_time;
	__time64_t key_end_time;
	memcpy(&key_start_time, key_tmp+32, 8);
	memcpy(&key_end_time, key_tmp+40, 8);

	if (key_start_time > time || time > key_end_time || (key_start_time >> 32) || (key_end_time >> 32))
	{
		del_setting(reg_key);
		return E_FAIL;
	}

	memcpy(file_key, key_tmp, 32);
	return S_OK;
}

HRESULT save_e3d_key(const unsigned char *file_hash, const unsigned char *file_key)
{
	wchar_t tmp[3] = L"";
	wchar_t reg_key[41] = L"";
	for(int i=0; i<20; i++)
	{
		wsprintfW(tmp, L"%02X", file_hash[i]);
		wcscat(reg_key, tmp);
	}

	unsigned char encrypted_key[32+16];
	// time
	__time64_t time = mytime();
	memcpy(encrypted_key+32, &time, 8);
	time += 7*24*3600;		// 7 days
	memcpy(encrypted_key+40, &time, 8);

	// AES it
	AESCryptor codec;
	codec.set_key((const unsigned char*)g_passkey, 256);
	codec.encrypt((const unsigned char*)file_key, (unsigned char*)encrypted_key);
	codec.encrypt((const unsigned char*)file_key+16, (unsigned char*)encrypted_key+16);
	codec.encrypt((const unsigned char*)encrypted_key+32, (unsigned char*)encrypted_key+32);

	save_setting(reg_key, encrypted_key, 32+16);

	return S_OK;
}

HRESULT make_xvid_support_mp4v()
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\GNU\\XviD", 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	DWORD value = 4, size=4;
	ret = RegQueryValueExW(hkey, L"Supported_4CC", 0, NULL, (LPBYTE)&value, (LPDWORD)&size);
	value |= 0xf;
	ret = RegSetValueExW(hkey, L"Supported_4CC", 0, REG_DWORD, (const byte*)&value, size );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	RegCloseKey(hkey);
	return S_OK;
}

HRESULT make_av_splitter_support_my_formats()
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\AVS\\AV Splitter\\Settings", 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	DWORD value = 0, size=4;
	ret = RegSetValueExW(hkey, L"TrayIcon", 0, REG_DWORD, (const byte*)&value, size );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	RegCloseKey(hkey);

	ret = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\AVS\\AV Splitter\\Settings\\Formats", 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	value = 1;
	wchar_t formats[10][200] = {L"asf,wma,wmv", L"avi", L"flv", L"matroska", L"matroska,webm", L"mov,mp4", L"mpeg", L"mpegts", L"ogg,ogm", L"rm,rmvb"};
	for (int i=0; i<10; i++)
	{
		ret = RegSetValueExW(hkey, formats[i], 0, REG_DWORD, (const byte*)&value, 4 );
		if (ret != ERROR_SUCCESS)
			return E_FAIL;
	}
	RegCloseKey(hkey);


	return S_OK;
}

HRESULT DeterminPin(IPin *pin, wchar_t *name, CLSID majortype, CLSID subtype)
{
	if (NULL == pin)
		return E_POINTER;

	if (!name && majortype == CLSID_NULL && subtype == CLSID_NULL)		
		return E_INVALIDARG;

	PIN_INFO pi;
	pin->QueryPinInfo(&pi);
	if (pi.pFilter) pi.pFilter->Release();
	bool name_ok = true;
	bool major_ok = true;
	bool sub_ok = true;
	if (name)
	{
		if (!wcsstr(pi.achName, name))
			name_ok = false;
	}

	if (majortype != CLSID_NULL || subtype != CLSID_NULL)
	{
		if (majortype != CLSID_NULL)
			major_ok = false;
		if (subtype != CLSID_NULL)
			sub_ok = false;

		CComPtr<IEnumMediaTypes> em;
		pin->EnumMediaTypes(&em);
		AM_MEDIA_TYPE *mt = NULL;
		while (em->Next(1, &mt, NULL) == S_OK)
		{
			if (mt->majortype == majortype && subtype == CLSID_NULL)
				major_ok = true;
			if (mt->subtype == subtype && majortype == CLSID_NULL)
				sub_ok = true;
			if (mt->majortype == majortype && mt->subtype == subtype)
				major_ok = sub_ok = true;
			DeleteMediaType(mt);
		}
	}

	return (name_ok && major_ok && sub_ok) ? S_OK : S_FALSE;

}

HRESULT localize_menu(HMENU menu)
{
	HRESULT hr = S_OK;
	const int max_item = 999;
	for(int i=0; i<max_item; i++)
	{
		HMENU sub = GetSubMenu(menu, i);
		if (sub)
		{
			hr = localize_menu(sub);
		}
		//else
		{
			int flag = GetMenuState(menu, i, MF_BYPOSITION);
			if (flag == -1)
				return hr;
			flag &= 0xffff;
			int uid = GetMenuItemID(menu, i);
			wchar_t text[1024];
			GetMenuStringW(menu, i, text, 1024, MF_BYPOSITION);

			const wchar_t *local = C(text);
			if (sub)
				ModifyMenuW(menu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)sub, local);
			else
				ModifyMenuW(menu, i, MF_BYPOSITION | flag, uid, local);
		}
	}

	return hr;
}

BOOL CALLBACK localize_window_proc(HWND hwnd, LPARAM lParam)
{
	wchar_t tmp[1024];
	GetWindowTextW(hwnd, tmp, 1024);
	SetWindowTextW(hwnd, C(tmp));

	return TRUE;
}

HRESULT localize_window(HWND hwnd)
{
	wchar_t tmp[1024];
	GetWindowTextW(hwnd, tmp, 1024);
	SetWindowTextW(hwnd, C(tmp));
	EnumChildWindows(hwnd, localize_window_proc, NULL);
	return S_OK;
}

const WCHAR* soft_key= L"Software\\DWindow";
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, REG_TYPE!=REG_SZ?len:wcslen((wchar_t*)data)*2+2);
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}

int load_setting(const WCHAR *key, void *data, int len)
{
	HKEY hkey = NULL;
	int ret = RegOpenKeyExW(HKEY_CURRENT_USER, soft_key,0,STANDARD_RIGHTS_REQUIRED |KEY_READ  , &hkey);
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return -1;
	ret = RegQueryValueExW(hkey, key, 0, NULL, (LPBYTE)data, (LPDWORD)&len);
	if (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA)
		return len;

	RegCloseKey(hkey);
	return 0;
}

bool del_setting(const WCHAR *key)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return false;

	ret = RegDeleteValueW(hkey, key);
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}

const WCHAR* soft_key_private= L"Software\\Microsoft\\Direct3D\\MostRecentApplication";
bool save_D3D_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE/*=REG_BINARY*/)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key_private, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, REG_TYPE!=REG_SZ?len:wcslen((wchar_t*)data)*2+2);
	if (ret != ERROR_SUCCESS)
		return false;

	RegCloseKey(hkey);
	return true;
}
int load_D3D_setting(const WCHAR *key, void *data, int len)
{
	HKEY hkey = NULL;
	int ret = RegOpenKeyExW(HKEY_CURRENT_USER, soft_key_private,0,STANDARD_RIGHTS_REQUIRED |KEY_READ  , &hkey);
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return -1;
	ret = RegQueryValueExW(hkey, key, 0, NULL, (LPBYTE)data, (LPDWORD)&len);
	if (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA)
		return len;

	RegCloseKey(hkey);
	return 0;
}

typedef struct _download_para
{
	char *url_to_download;
	char *out;
	int outlen;
	bool *cancel;
	HRESULT *hr;
} download_para;

DWORD WINAPI download_thread(LPVOID para)
{
	download_para * p = (download_para*)para;

	char url_to_download[1024];
	strcpy_s(url_to_download, p->url_to_download);
	void *out = malloc(p->outlen);
	int outlen = p->outlen;
	bool *cancel = p->cancel;
	HRESULT *hr = p->hr;

	HINTERNET HI;
	HI=InternetOpenA("dwindow",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (HI==NULL)
		return E_FAIL;

	HINTERNET HURL;
	HURL=InternetOpenUrlA(HI, url_to_download,NULL,0,INTERNET_FLAG_RELOAD | NULL,0);
	if (HURL==NULL)
		return E_FAIL;

	DWORD byteread = 0;
	BOOL internetreadfile = InternetReadFile(HURL,out, outlen, &byteread);

	if (!internetreadfile && !*cancel)
		*hr = S_FALSE;

	if (!*cancel)
	{
		*hr = S_OK;
		memcpy(p->out, out, byteread);
	}

	free(out);
	delete cancel;
	return 0;
}

HRESULT download_url(char *url_to_download, char *out, int outlen /*= 64*/, int timeout/*=INFINITE*/)
{
	download_para thread_para = {url_to_download, out, outlen, new bool(false), new HRESULT(E_FAIL)};

	HANDLE thread = CreateThread(NULL, NULL, download_thread, &thread_para, NULL, NULL);

	WaitForSingleObject(thread, timeout);

	if (*thread_para.hr == E_FAIL)
		*thread_para.cancel = true;

	HRESULT hr = *thread_para.hr == S_OK ? S_OK: E_FAIL;
	delete thread_para.hr;
	return hr;
}

HRESULT download_e3d_key(const wchar_t *filename)
{
	HRESULT hr;
	file_reader reader;
	HANDLE h_file = CreateFileW (filename, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	reader.SetFile(h_file);
	if (!reader.m_is_encrypted)
	{
		hr = E_FAIL;
		goto err_ret;
	}

	load_passkey();
	dwindow_message_uncrypt message;
	memset(&message, 0, sizeof(message));
	message.zero = 0;
	message.client_rev = my12doom_rev;
	memcpy(message.passkey, g_passkey, 32);
	memcpy(message.requested_hash, reader.m_hash, 20);
	srand(time(NULL));
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() & 0xff;

	unsigned char encrypted_message[128];
	RSA_dwindow_network_public(&message, encrypted_message);

	char url[300] = "";
	char tmp[3];
	strcpy(url, g_server_address);
	strcat(url, g_server_E3D);
	strcat(url, "?");
	for(int i=0; i<128; i++)
	{
		sprintf(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char str_e3d_key[65] = "D3821F7B81206903280461E52DE2B29901B9B458836B3795DD40F50C2583EF7A";
	hr = download_url(url, str_e3d_key);
	if (FAILED(hr))
		goto err_ret;

	unsigned char e3d_key[36];
	for(int i=0; i<32; i++)
		sscanf(str_e3d_key+i*2, "%02X", e3d_key+i);

	AESCryptor aes;
	aes.set_key(message.random_AES_key, 256);
	aes.decrypt(e3d_key, e3d_key);
	aes.decrypt(e3d_key+16, e3d_key+16);
	reader.set_key(e3d_key);
	if (!reader.m_key_ok)
	{
		hr = E_FAIL;
		goto err_ret;
	}

	CloseHandle(h_file);

	return e3d_set_process_key(e3d_key);


err_ret:
	CloseHandle(h_file);
	return hr;
}

DWORD WINAPI killer_thread(LPVOID time)
{
	Sleep(*(DWORD*)time);
	if (GetTickCount() - g_last_bar_time > HEARTBEAT_TIMEOUT)
		TerminateProcess(GetCurrentProcess(), -1);
	return 0;
}

DWORD WINAPI ad_thread(LPVOID lpParame)
{
	char url[512+1];

	strcpy(url, g_server_address);
	strcat(url, g_server_ad);
	while (FAILED(download_url(url, g_ad_address, 512)))
		Sleep(30*1000);


	return 0;
}

DWORD WINAPI killer_thread2(LPVOID time)
{
	int sleep = 0;
#ifdef no_dual_projector
	char url[512+1];
	if (strstr(g_ad_address, "http") == g_ad_address)
	{
		sleep = *(DWORD*)time;
		sprintf(url, "explorer.exe \"%s\"", g_ad_address);

		if (!is_payed_version())
			WinExec(url, SW_HIDE);
	}
	else
	{
		// no ad / invalid ad
	}
#endif
	Sleep(sleep);
	TerminateProcess(GetCurrentProcess(), 0);
	return 0;
}

HRESULT bar_logout()
{
	if (!g_bar_server[0])
		return S_OK;

	USES_CONVERSION;
	char url[1024] = "http://";
	char tmp[10];
	strcat_s(url, W2A(g_bar_server));
	strcat_s(url, "/LOGOUT");

	download_url(url, tmp, 1024, 3000);
	return S_OK;
}

int wcsexplode(const wchar_t *string_to_explode, const wchar_t *delimeter, wchar_t **out, int max_part /* = 0xfffffff */)
{
	int strleng = wcslen(string_to_explode);
	int delleng = wcslen(delimeter);

	if (delleng == 0 || delleng >= strleng)
	{
		wcscpy(out[0], string_to_explode);
		return 0;
	}

	wchar_t *str = (wchar_t*)malloc(sizeof(wchar_t)*(wcslen(string_to_explode)+1));
	wchar_t *tmp = str;
	wcscpy(str, string_to_explode);

	int n_found = 0;
	bool del_tail = false;
	while ((n_found < max_part-1) &&(tmp = wcsstr(str, delimeter)))
	{
		out[n_found] = (wchar_t *) malloc(sizeof(wchar_t) * (wcslen(str)+1));
		wcscpy(out[n_found], str);
		out[n_found++][tmp-str] = NULL;

		if (tmp+delleng >= str + wcslen(str))
		{
			del_tail = true;
			break;
		}
		else if (!wcsstr(tmp+delleng, delimeter))
		{
			memmove(str, tmp+delleng, (1+wcslen(tmp+delleng)) * sizeof(wchar_t));
			break;
		}
		else
			memmove(str, tmp+delleng, (1+wcslen(tmp+delleng)) * sizeof(wchar_t));
	}

	if (!del_tail)
	{
		out[n_found] = (wchar_t *) malloc(sizeof(wchar_t) * (wcslen(str)+1));
		wcscpy(out[n_found++], str);
	}

	free(str);

	return n_found;
}

int wcstrim(wchar_t *str, wchar_t char_ )
{
	int len = (int)wcslen(str);
	//LEADING:
	int lead = 0;
	for(int i=0; i<len; i++)
		if (str[i] != char_)
		{
			lead = i;
			break;
		}

	//ENDING:
	int end = 0;
	for (int i=len-1; i>=0; i--)
		if (str[i] != char_)
		{
			end = len - 1 - i;
			break;
		}
	//TRIMMING:
	memmove(str, str+lead, (len-lead-end)*sizeof(wchar_t));
	str[len-lead-end] = NULL;

	return len - lead - end;
}

localization_language get_system_default_lang()
{
	LCID lcid = GetSystemDefaultLCID();
	if (lcid == 2052)
		return CHINESE;
	else if (lcid == 3076)
		return CHINESE;
	else
		return ENGLISH;
}

HMODULE hDXVA2 = LoadLibrary( L"dxva2.dll" );

typedef HRESULT (WINAPI *lpDXVA2CreateDirect3DDeviceManager9)(UINT * token,IDirect3DDeviceManager9** manager);
typedef HRESULT (WINAPI *lpDXVA2CreateVideoService)(IDirect3DDevice9* pDD, REFIID riid, void** ppService);

lpDXVA2CreateDirect3DDeviceManager9 mineDXVA2CreateDirect3DDeviceManager9;
lpDXVA2CreateVideoService mineDXVA2CreateVideoService;
HRESULT loadDXVA2()
{
	if (!hDXVA2)
		return E_NOINTERFACE;

	mineDXVA2CreateDirect3DDeviceManager9 = (lpDXVA2CreateDirect3DDeviceManager9)GetProcAddress(hDXVA2, "DXVA2CreateDirect3DDeviceManager9");

	mineDXVA2CreateVideoService = (lpDXVA2CreateVideoService)GetProcAddress(hDXVA2, "DXVA2CreateVideoService");

	return S_OK;
}

HRESULT myDXVA2CreateDirect3DDeviceManager9(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager)
{
	loadDXVA2();

	if (!mineDXVA2CreateDirect3DDeviceManager9)
		return E_NOINTERFACE;

	return mineDXVA2CreateDirect3DDeviceManager9(pResetToken, ppDeviceManager);
}

HRESULT myDXVA2CreateVideoService(IDirect3DDevice9* pDD, REFIID riid, void** ppService)
{
	loadDXVA2();

	if (!mineDXVA2CreateVideoService)
		return E_NOINTERFACE;

	return mineDXVA2CreateVideoService(pDD, riid, ppService);
}

extern "C" HRESULT WINAPI DXVA2CreateVideoService(IDirect3DDevice9* pDD, REFIID riid, void** ppService)
{
	return myDXVA2CreateVideoService(pDD, riid, ppService);
}