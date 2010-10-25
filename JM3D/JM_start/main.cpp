#include <stdio.h>
#include <windows.h>

#include "detours.h"
#pragma comment(lib, "detours.lib")
#pragma comment(lib, "detoured.lib")

#define arrayof(x)      (sizeof(x)/sizeof(x[0]))

static BOOL CALLBACK ExportCallback(PVOID pContext,
                                    ULONG nOrdinal,
                                    PCHAR pszSymbol,
                                    PVOID pbTarget)
{
    (void)pContext;
    (void)pbTarget;
    (void)pszSymbol;

    if (nOrdinal == 1) {
        *((BOOL *)pContext) = TRUE;
    }
    return TRUE;
}

BOOL DoesDllExportOrdinal1(PCHAR pszDllPath)
{
    HMODULE hDll = LoadLibraryEx(pszDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hDll == NULL) {
        printf("withdll.exe: LoadLibraryEx(%s) failed with error %d.\n",
               pszDllPath,
               GetLastError());
        return FALSE;
    }

    BOOL validFlag = FALSE;
    DetourEnumerateExports(hDll, &validFlag, ExportCallback);
    FreeLibrary(hDll);
    return validFlag;
}
int CDECL main(int argc, char **argv)
{
    CHAR szExe[MAX_PATH] = "ldecod.exe";
	CHAR szCommand[1024]= "";
	CHAR szDllPath[MAX_PATH]= "JM_hook.dll";
	CHAR szDetouredDllPath[MAX_PATH] = "detoured.dll";

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    printf("Starting: `%s %s'\n", szExe, szCommand);
    printf("with hook `%s'\n", szDllPath);
    fflush(stdout);

    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    if (!DetourCreateProcessWithDll(szExe, szCommand,
                                    NULL, NULL, TRUE, dwFlags, NULL, NULL,
                                    &si, &pi, szDetouredDllPath, szDllPath, NULL)) {
        printf("failed: %d\n", GetLastError());
		return 9007;
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwResult = 0;
    if (!GetExitCodeProcess(pi.hProcess, &dwResult)) {
        return 9008;
    }

    return dwResult;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main(0,NULL);
}

