#include <time.h>
#include "global_funcs.h"
#include <Shlobj.h>
#include <streams.h>
#include <intrin.h>
#include "detours/detours.h"
#include "..\AESFile\E3DReader.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "detours/detours.lib")

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

char *g_server_address = "http://59.51.45.21:80/";

//char server_url[300] = "http://127.0.0.1:8080/w32.php?";
//char url[512] = "http://127.0.0.1:8080/gen_key.php?";
char url[512] = "http://59.51.45.21/gen_key.php?";

// public variables
AutoSetting<localization_language> g_active_language(L"Language", CHINESE);
char g_passkey[32];
char g_passkey_big[128];

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

int find_monitors()
{
	CoInitialize(NULL);
	CComPtr<IDirect3D9> d3d;
	d3d = Direct3DCreate9( D3D_SDK_VERSION );
	if (!d3d)
		return 0;

	int i = 0;
	for(i=0; i<min(16, d3d->GetAdapterCount()); i++)
	{
		d3d->GetAdapterIdentifier(i, NULL, g_ids+i);
		g_monitors[i] = d3d->GetAdapterMonitor(i);
	}
	d3d = NULL;
	CoUninitialize();
	return i;
}

int g_monitor_count = find_monitors();
HMONITOR g_monitors[16];
D3DADAPTER_IDENTIFIER9 g_ids[16];


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
static bool coremvc_hooked = false;
static DWORD (WINAPI *TrueGetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize) = GetModuleFileNameA;
DWORD WINAPI MineGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
	DWORD rtn = TrueGetModuleFileNameA(hModule, lpFilename, nSize);
	if (hModule == NULL)
		strcat(lpFilename, ".StereoPlayer.exe.MVCToAVI.exe");
	return rtn;
}

HRESULT beforeCreateCoreMVC()
{
	if (!coremvc_hooked)
	{
		// apihook
		DetourRestoreAfterWith();
		DetourTransactionBegin();

		int id = GetCurrentThreadId();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);
		LONG error = DetourTransactionCommit();
		coremvc_hooked = true;
	}

	return S_OK;
}

HRESULT afterCreateCoreMVC()
{
	if (coremvc_hooked)
	{
		// apihook
		DetourRestoreAfterWith();
		DetourTransactionBegin();

		int id = GetCurrentThreadId();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);
		LONG error = DetourTransactionCommit();
		coremvc_hooked = false;
	}

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

AutoSetting<bool> g_CUDA(L"CUDA", true);
HRESULT ActiveCoreMVC(IBaseFilter *decoder)
{

	CComQIPtr<IPropertyBag, &IID_IPropertyBag> pbag(decoder);
	if (pbag)
	{
		write_property(pbag, L"use_tray=0");
		write_property(pbag, L"low_latency=0");
		write_property(pbag, g_CUDA ? L"use_cuda=1" : L"use_cuda=0");
		return write_property(pbag, L"app_mode=1");
	}
	else
	{
		return E_FAIL;
	}
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


HRESULT check_passkey()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);
	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return E_FAIL;

	__time64_t t = _time64(NULL);

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > _time64(NULL) || _time64(NULL) > m1.time_end)
	{
		memset(&m1, 0, 128);
		return E_FAIL;
	}

	memcpy(g_passkey, &m1, 32);
	memset(&m1, 0, 128);
	return S_OK;
}

HRESULT load_passkey()
{
	int CPUInfo[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(CPUInfo, 0x80000002);
	memcpy(CPUBrandString, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000003);
	memcpy(CPUBrandString+16, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000004);
	memcpy(CPUBrandString+32, CPUInfo, 16);
	memset(CPUBrandString + strlen((char*)CPUBrandString), 0, 48-strlen((char*)CPUBrandString));
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];

	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn;

	AESCryptor aes;
	aes.set_key(CPUBrandString, 256);

	memset(g_passkey_big, 0x38, 128);
	load_setting(L"passkey", g_passkey_big, 128);
	for(int i=0; i<128; i+=16)
		aes.decrypt((unsigned char*)g_passkey_big+i, (unsigned char*)g_passkey_big+i);
	return check_passkey();
}

HRESULT save_passkey()
{
	int CPUInfo[4];
	unsigned char CPUBrandString[48];
	memset(CPUBrandString, 0, 48);
	__cpuid(CPUInfo, 0x80000002);
	memcpy(CPUBrandString, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000003);
	memcpy(CPUBrandString+16, CPUInfo, 16);
	__cpuid(CPUInfo, 0x80000004);
	memcpy(CPUBrandString+32, CPUInfo, 16);
	memset(CPUBrandString + strlen((char*)CPUBrandString), 0, 48-strlen((char*)CPUBrandString));
	for(int i=0; i<16; i++)
		CPUBrandString[i] ^= CPUBrandString[i+32];


	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	((DWORD*)CPUBrandString)[4] ^= volume_c_sn;

	//char tmp [32];
	//sprintf(tmp, "%d", volume_c_sn);
	//MessageBoxA(NULL, (char*)CPUBrandString, "...", MB_OK);
	//MessageBoxA(NULL, tmp, "...", MB_OK);
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
	__time64_t time = _time64(NULL);
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
	__time64_t time = _time64(NULL);
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

			wchar_t *local = C(text);
			if (NULL == local) local = text;
			if (sub)
				ModifyMenuW(menu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)sub, local);
			else
				ModifyMenuW(menu, i, MF_BYPOSITION | flag, uid, local);
		}
	}

	return hr;
}

const WCHAR* soft_key= L"Software\\DWindow";
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE)
{
	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return false;
	ret = RegSetValueExW(hkey, key, 0, REG_TYPE, (const byte*)data, len );
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
		return false;
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

HRESULT download_url(char *url_to_download, char *out, int outlen /*= 64*/)
{
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

	if (!internetreadfile)
		return E_FAIL;

	return S_OK;
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
	message.zero = 0;
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