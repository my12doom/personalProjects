#pragma once
#include <Windows.h>

class ICommandReciever
{
public:
	virtual HRESULT execute_command(wchar_t *command, wchar_t *out = NULL, const wchar_t *arg1 = NULL, const wchar_t *arg2 = NULL, const wchar_t *arg3 = NULL, const wchar_t *arg4 = NULL)
	{
		if (command == NULL)
			return E_POINTER;

		int args_count = 4;
		if (arg4== NULL) args_count = 3;
		if (arg3== NULL) args_count = 2;
		if (arg2== NULL) args_count = 1;
		if (arg1== NULL) args_count = 0;
		const wchar_t *args[4] = {arg1,arg2,arg3,arg4};

		return execute_command_adv(command, out, args, args_count);
	}
	virtual HRESULT execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count) PURE;
};