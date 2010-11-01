#include <windows.h>
#include <atlbase.h>
#include <dshow.h>
#include <initguid.h>
#include <strsafe.h>

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

HRESULT reg_ax(const TCHAR *pathname);
bool compare_file(wchar_t *file1, wchar_t *file2);


int main()
{
	bool silent = false;
	int argc = 0;
	LPWSTR *argvv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc>1)
		silent = true;

	CoInitialize(NULL);

	CComPtr<IBaseFilter> demuxer;
	CComPtr<IBaseFilter> decoder;

	demuxer.CoCreateInstance(CLSID_PD10_DEMUXER);
	decoder.CoCreateInstance(CLSID_PD10_DECODER);

	bool all_ok = true;
	if (demuxer == NULL)
	{
		HRESULT hr = reg_ax(_T("codec\\CLDemuxer2.ax"));
		if (FAILED(hr))
		{
			all_ok = false;
			if(!silent)MessageBox(0, _T("×¢²áCyberlink Demuxer 2.0Ê§°Ü£¡"), _T("ERROR"), MB_OK | MB_ICONERROR);
		}
	}

	// check decoder veriosn
	bool right_decoder = false;
	if (decoder != NULL)
	{
        HKEY hkeyFilter=0;
        DWORD dwSize=MAX_PATH;
        BYTE pbFilename[MAX_PATH];

        // Open the CLSID key that contains information about the filter
        int rc = RegOpenKey(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID\\{D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}\\InprocServer32", &hkeyFilter);
        if (rc == ERROR_SUCCESS)
        {
            rc = RegQueryValueEx(hkeyFilter, NULL,  // Read (Default) value
                                 NULL, NULL, pbFilename, &dwSize);

            if (rc == ERROR_SUCCESS)
            {
                TCHAR szFilename[MAX_PATH];
                HRESULT hr = StringCchPrintf(szFilename, NUMELMS(szFilename), TEXT("%s\0"), pbFilename);

				if(compare_file(szFilename, L"codec\\CLCvd.ax"))
					right_decoder = true;
            }

            RegCloseKey(hkeyFilter);
        }

	}

	if (decoder == NULL || !right_decoder)
	{
		HRESULT hr = reg_ax(_T("codec\\CLCvd.ax"));
		if (FAILED(hr))
		{			
			all_ok = false;
			if(!silent)MessageBox(0, _T("×¢²áCyberlink Video Decoder(PDVD10)Ê§°Ü£¡"), _T("ERROR"), MB_OK | MB_ICONERROR);
		}
	}

	// always register mySplitter
	{
		HRESULT hr = reg_ax(_T("codec\\mySplitter.ax"));
		if (FAILED(hr))
		{
			all_ok = false;
			if(!silent)MessageBox(0, _T("×¢²áDWindow filtersÊ§°Ü£¡"), _T("ERROR"), MB_OK | MB_ICONERROR);
		}
	}

	if (!silent)
	{
		if (all_ok)
			MessageBox(0, _T("×¢²á×é¼þÍê³É"), _T(""), MB_OK | MB_ICONINFORMATION);
		else
			MessageBox(0, _T("×¢²á×é¼þÊ§°Ü"), _T("ERROR"), MB_OK | MB_ICONERROR);
	}

	demuxer = NULL;
	decoder = NULL;

	CoUninitialize();

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main();
}

HRESULT reg_ax(const TCHAR *pathname)
{
	HINSTANCE hLib = LoadLibrary( pathname);
	if (hLib == (HINSTANCE)HINSTANCE_ERROR)
		return HRESULT_FROM_WIN32(GetLastError());

	FARPROC entry_point;
	entry_point = GetProcAddress (hLib,("DllRegisterServer"));

	if (entry_point == NULL)
		return E_FAIL;

	return (*entry_point)();
}

bool compare_file(wchar_t *file1, wchar_t *file2)
{
	bool rtn = false;

	FILE *f1 = NULL;
	FILE *f2 = NULL;
	void *buf1 = NULL;
	void *buf2 = NULL;
	int size1 = 0;
	int size2 = 0;

	f1 = _wfopen(file1, L"rb");
	f2 = _wfopen(file2, L"rb");

	if (!f1 || !f2)
		goto clear_up;


	fseek(f1, 0, SEEK_END);
	fseek(f2, 0, SEEK_END);

	size1 = ftell(f1);
	size2 = ftell(f2);

	if (size1 != size2)
		goto clear_up;

	buf1 = malloc(size1);
	buf2 = malloc(size2);

	fseek(f1, 0, SEEK_SET);
	fseek(f2, 0, SEEK_SET);

	fread(buf1, 1, size1, f1);
	fread(buf2, 1, size2, f2);

	if (memcmp(buf1, buf2, size1) == 0)
		rtn = true;

clear_up:
	if (f1) fclose(f1);
	if (f2) fclose(f2);
	if (buf1) free(buf1);
	if (buf2) free(buf2);

	return rtn;
}