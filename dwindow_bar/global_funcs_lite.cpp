#include "global_funcs_lite.h"

#include <wininet.h>
#pragma  comment(lib, "WININET.lib")

char g_passkey_big[128];
char *g_server_address = "http://bo3d.net:81/";
//char *g_server_address = "http://127.0.0.1:8080/";

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

	memset(&m1, 0, 128);
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