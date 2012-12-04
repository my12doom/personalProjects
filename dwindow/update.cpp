#include "update.h"
#include "global_funcs.h"
#include "..\libass\charset.h"

AutoSetting<DWORD> latest_rev(L"LatestRev", 0, REG_DWORD);
AutoSettingString latest_url(L"LatestUrl",L"");
AutoSettingString latest_description(L"LatestDescription",L"");

DWORD WINAPI check_update_thread(LPVOID);
HANDLE g_thread = INVALID_HANDLE_VALUE;
HWND g_wnd;
DWORD g_message;

HRESULT start_check_for_update(HWND callback_window, DWORD callback_message)			// threaded, S_OK: thread started. S_FALSE: thread already started, this call is ignored, no callback will be made.
{
	if (g_thread != INVALID_HANDLE_VALUE)
		return S_FALSE;

	g_wnd = callback_window;
	g_message = callback_message;

	g_thread = CreateThread(NULL, NULL, check_update_thread, NULL, NULL, NULL);

	return S_OK;
}
HRESULT get_update_result(wchar_t *description, int *new_rev, wchar_t *url)
{
	if (g_thread == INVALID_HANDLE_VALUE)
		return E_FAIL;

	if (description)
		wcscpy(description, latest_description);
	if (new_rev)
		*new_rev = latest_rev;
	if (url)
		wcscpy(url, latest_url);

	return S_OK;
}

DWORD WINAPI check_update_thread(LPVOID)
{
	Sleep(30*1000);

	int buffersize = 1024*1024;
	char *data = (char*)malloc(buffersize);
	char *data2 = (char*)malloc(buffersize);
	wchar_t *dataw = (wchar_t*)malloc(buffersize*sizeof(wchar_t));
	char url[512] = {0};
	strcpy(url, g_server_address);
	strcat(url, g_server_update);
	strcat(url, "?lang=");
	sprintf(data, "%d", GetSystemDefaultLCID());
	strcat(url, data);
	memset(data, 0, buffersize);
	memset(data2, 0, buffersize);
	memset(dataw, 0, buffersize*sizeof(wchar_t));
	download_url(url, data, buffersize);
	strcat(data, "\r\n");
	ConvertToUTF8(data, strlen(data)+1, data2, buffersize);

	MultiByteToWideChar(CP_UTF8, 0, data2, buffersize, dataw, buffersize);

	wchar_t *exploded[3] = {0};
	wcsexplode(dataw, L"\n", exploded, 3);

	latest_rev = _wtoi(exploded[0]);
	latest_url = exploded[1];
	latest_description = exploded[2];

	for(int i=0; i<3; i++)
		if (exploded[i])
			free(exploded[i]);
	free(data);
	free(data2);
	free(dataw);

	SendMessage(g_wnd, g_message, 0, 0);

	return 0;
}