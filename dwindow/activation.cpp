#include <Windows.h>
#include <time.h>
#include "global_funcs.h"
#include "..\AESFile\rijndael.h"
#include "activation.h"
#include "resource.h"
#include "Hyperlink.h"

static int on_command(HWND hWnd, int uid);
HWND g_activation_window;

HRESULT test_base64()
{

	luaState L;
	lua_getglobal(L, "bittorrent");
	lua_getfield(L, -1, "hex2string");
	lua_pushlstring(L, g_passkey_big, 128);

	lua_mypcall(L, 1, 1, 0);
	int raw_len = lua_rawlen(L, -1);
	if (lua_rawlen(L, -1) < 128 || lua_type(L, -1) != LUA_TSTRING)
		return E_INVALIDARG;

	SetDlgItemTextW(g_activation_window, IDC_ACTIVATIONCODE_BOX, UTF82W(lua_tostring(L, -1)));

	return S_OK;
}


INT_PTR CALLBACK activation_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
		g_activation_window = hDlg;
		localize_window(hDlg);
// 		test_base64();
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

int check_activation()
{
	load_passkey();
#ifdef OEM2
	wchar_t key[] = L"feYHOBQbnwYQMWtyHAgdwhm2lEJXqlUXa95NmO0F1106samn3uLFiHwVIQN+0LhhRfThQRcR/Ravda0gr3pa2waKUy8ypcga/fCiwpPXeJa+7OS77REMLnJ5kBNDpM2Q3IlRW0ZRQNcpoTokk2raoHlZPlqbq/fJDEA67P0RBwI=";
	active_base64(key);

#endif
	if (FAILED(check_passkey()))
	{
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_ACTIVATION), NULL, activation_proc );
	}

	save_passkey();

	BasicRsaCheck();

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK)
	{
		// TODO
		wchar_t key[1024];
		GetDlgItemTextW(hWnd, IDC_ACTIVATIONCODE_BOX, key, 1024);
		HRESULT hr = active_base64(key);
		if (SUCCEEDED(hr))
			EndDialog(hWnd, 0);
		else if (hr == E_INVALIDARG)
			MessageBoxW(hWnd, C(L"too short activation code, it should be 170 characters or more."), C(L"Error"), MB_OK);
		else 
			MessageBoxW(hWnd, C(L"invalid activation code."), C(L"Error"), MB_OK);

	}
	else if (uid == IDCANCEL)
	{
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}