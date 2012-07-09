#include "dx_player.h"

int wcscmp_nocase(const wchar_t*in1, const wchar_t *in2)
{
	wchar_t *tmp = new wchar_t[wcslen(in1)+1];
	wchar_t *tmp2 = new wchar_t[wcslen(in2)+1];
	wcscpy(tmp, in1);
	wcscpy(tmp2, in2);
	_wcslwr(tmp);
	_wcslwr(tmp2);

	int out = wcscmp(tmp, tmp2);

	delete [] tmp;
	delete [] tmp2;
	return out;
}

HRESULT dx_player::execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count)
{
	#define CASE(x) if (wcscmp_nocase(command, x) == 0)

	CASE(L"hello")
		MessageBoxW(m_hwnd1, L"HELLO!", L"HELLOW", MB_OK);

	return E_NOTIMPL;
}
