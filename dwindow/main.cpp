#include <time.h>
#include "global_funcs.h"
#include "dx_player.h"
#include "..\AESFile\E3DReader.h"
#include "..\AESFile\rijndael.h"
#include "resource.h"


// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
int on_command(HWND hWnd, int uid);
int init_dialog(HWND hWnd);

UINT FindAwardBios( BYTE** ppBiosAddr )
{
	BYTE* pBiosAddr = * ppBiosAddr + 0xEC71;

	BYTE szBiosData[128];
	CopyMemory( szBiosData, pBiosAddr, 127 );
	szBiosData[127] = 0;

	int iLen = strlen( ( char* )szBiosData );
	if( iLen > 0 && iLen < 128 )
	{
		//AWard:         07/08/2002-i845G-ITE8712-JF69VD0CC-00 
		//Phoenix-Award: 03/12/2002-sis645-p4s333
		if( szBiosData[2] == '/' && szBiosData[5] == '/' )
		{
			BYTE* p = szBiosData;
			while( * p )
			{
				if( * p < ' ' || * p >= 127 )
				{
					break;
				}
				++ p;
			}
			if( * p == 0 )
			{
				* ppBiosAddr = pBiosAddr;
				return ( UINT )iLen;
			}
		}
	}
	return 0;
}

UINT FindAmiBios( BYTE** ppBiosAddr )
{
	BYTE* pBiosAddr = * ppBiosAddr + 0xF478;

	BYTE szBiosData[128];
	CopyMemory( szBiosData, pBiosAddr, 127 );
	szBiosData[127] = 0;

	int iLen = strlen( ( char* )szBiosData );
	if( iLen > 0 && iLen < 128 )
	{
		// Example: "AMI: 51-2300-000000-00101111-030199-"
		if( szBiosData[2] == '-' && szBiosData[7] == '-' )
		{
			BYTE* p = szBiosData;
			while( * p )
			{
				if( * p < ' ' || * p >= 127 )
				{
					break;
				}
				++ p;
			}
			if( * p == 0 )
			{
				* ppBiosAddr = pBiosAddr;
				return ( UINT )iLen;
			}
		}
	}
	return 0;
}

UINT FindPhoenixBios( BYTE** ppBiosAddr )
{
	UINT uOffset[3] = { 0x6577, 0x7196, 0x7550 };
	for( UINT i = 0; i < 3; ++ i )
	{
		BYTE* pBiosAddr = * ppBiosAddr + uOffset[i];

		BYTE szBiosData[128];
		CopyMemory( szBiosData, pBiosAddr, 127 );
		szBiosData[127] = 0;

		int iLen = strlen( ( char* )szBiosData );
		if( iLen > 0 && iLen < 128 )
		{
			// Example: Phoenix "NITELT0.86B.0044.P11.9910111055"
			if( szBiosData[7] == '.' && szBiosData[11] == '.' )
			{
				BYTE* p = szBiosData;
				while( * p )
				{
					if( * p < ' ' || * p >= 127 )
					{
						break;
					}
					++ p;
				}
				if( * p == 0 )
				{
					* ppBiosAddr = pBiosAddr;
					return ( UINT )iLen;
				}
			}
		}
	}
	return 0;
}

INT_PTR CALLBACK MainDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
		init_dialog(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	CoInitialize(NULL);

	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, L"System initialization failed, the program will exit now.", L"Error", MB_OK);
		return -1;
	}

	load_passkey();
	if (FAILED(check_passkey()))
	{
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, MainDlgProc );
		return 0;
	}

	g_bomb_function;
	save_passkey();


	dx_player test(screen1, screen2, hinstance);
	while (!test.is_closed())
		Sleep(100);

	return 0;
}

int test(int in)
{
	printf("%d", in);
	return 0;
}

int main()
{
	/*
	BYTE szSystemInfo[4096]; // 在程序执行完毕后，此处存储取得的系统特征码
	UINT uSystemInfoLen = 0; // 在程序执行完毕后，此处存储取得的系统特征码的长度	//结构定义 
	typedef struct _UNICODE_STRING 
	{ 
		USHORT  Length;//长度 
		USHORT  MaximumLength;//最大长度 
		PWSTR  Buffer;//缓存指针 
	} UNICODE_STRING,*PUNICODE_STRING; 
	typedef struct _OBJECT_ATTRIBUTES 
	{ 
		ULONG Length;//长度 18h 
		HANDLE RootDirectory;//  00000000 
		PUNICODE_STRING ObjectName;//指向对象名的指针 
		ULONG Attributes;//对象属性00000040h 
		PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR，0 
		PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE，0 
	} OBJECT_ATTRIBUTES; 
	typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES; 
	//函数指针变量类型
	typedef DWORD  (__stdcall *ZWOS )( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES); 
	typedef DWORD  (__stdcall *ZWMV )( HANDLE,HANDLE,PVOID,ULONG,ULONG,PLARGE_INTEGER,PSIZE_T,DWORD,ULONG,ULONG); 
	typedef DWORD  (__stdcall *ZWUMV )( HANDLE,PVOID); 

	// BIOS 编号，支持 AMI, AWARD, PHOENIX
	{
		SIZE_T ssize; 

		LARGE_INTEGER so; 
		so.LowPart=0x000f0000;
		so.HighPart=0x00000000; 
		ssize=0xffff; 
		wchar_t strPH[30]=L"\\device\\physicalmemory"; 

		DWORD ba=0;

		UNICODE_STRING struniph; 
		struniph.Buffer=strPH; 
		struniph.Length=0x2c; 
		struniph.MaximumLength =0x2e; 

		OBJECT_ATTRIBUTES obj_ar; 
		obj_ar.Attributes =64;
		obj_ar.Length =24;
		obj_ar.ObjectName=&struniph;
		obj_ar.RootDirectory=0; 
		obj_ar.SecurityDescriptor=0; 
		obj_ar.SecurityQualityOfService =0; 

		HMODULE hinstLib = LoadLibraryA("ntdll.dll"); 
		ZWOS ZWopenS=(ZWOS)GetProcAddress(hinstLib,"ZwOpenSection"); 
		ZWMV ZWmapV=(ZWMV)GetProcAddress(hinstLib,"ZwMapViewOfSection"); 
		ZWUMV ZWunmapV=(ZWUMV)GetProcAddress(hinstLib,"ZwUnmapViewOfSection"); 

		//调用函数，对物理内存进行映射 
		HANDLE hSection; 
		if( 0 == ZWopenS(&hSection,4,&obj_ar) && 
			0 == ZWmapV( 
			( HANDLE )hSection,   //打开Section时得到的句柄 
			( HANDLE )0xFFFFFFFF, //将要映射进程的句柄， 
			&ba,                  //映射的基址 
			0,
			0xFFFF,               //分配的大小 
			&so,                  //物理内存的地址 
			&ssize,               //指向读取内存块大小的指针 
			1,                    //子进程的可继承性设定 
			0,                    //分配类型 
			2                     //保护类型 
			) )
			//执行后会在当前进程的空间开辟一段64k的空间，并把f000:0000到f000:ffff处的内容映射到这里 
			//映射的基址由ba返回,如果映射不再有用,应该用ZwUnmapViewOfSection断开映射 
		{
			BYTE* pBiosSerial = ( BYTE* )ba;
			UINT uBiosSerialLen = FindAwardBios( &pBiosSerial );
			if( uBiosSerialLen == 0U )
			{
				uBiosSerialLen = FindAmiBios( &pBiosSerial );
				if( uBiosSerialLen == 0U )
				{
					uBiosSerialLen = FindPhoenixBios( &pBiosSerial );
				}
			}
			if( uBiosSerialLen != 0U )
			{
				CopyMemory( szSystemInfo + uSystemInfoLen, pBiosSerial, uBiosSerialLen );
				uSystemInfoLen += uBiosSerialLen;
			}
			ZWunmapV( ( HANDLE )0xFFFFFFFF, ( void* )ba );
		}
	}

	// CPU ID
	{
		BOOL bException = FALSE;
		BYTE szCpu[16]  = { 0 };
		UINT uCpuID     = 0U;

		__try 
		{
			_asm 
			{
				mov eax, 0
					cpuid
					mov dword ptr szCpu[0], ebx
					mov dword ptr szCpu[4], edx
					mov dword ptr szCpu[8], ecx
					mov eax, 1
					cpuid
					mov uCpuID, edx
			}
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			bException = TRUE;
		}

		if( !bException )
		{
			CopyMemory( szSystemInfo + uSystemInfoLen, &uCpuID, sizeof( UINT ) );
			uSystemInfoLen += sizeof( UINT );

			uCpuID = strlen( ( char* )szCpu );
			CopyMemory( szSystemInfo + uSystemInfoLen, szCpu, uCpuID );
			uSystemInfoLen += uCpuID;
		}
	}


	SYSTEM_INFO sinfo;
	GetSystemInfo(&sinfo);

	printf("len=%d", uSystemInfoLen);
	*/

	WinMain(LoadLibraryW(L"dwindow.exe"), 0, "", SW_SHOW);
}


int init_dialog(HWND hWnd)
{
	SetWindowTextW(hWnd, C(L"Enter User ID"));

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK)
	{
		wchar_t str_key[512];
		GetWindowTextW(GetDlgItem(hWnd, IDC_EDIT1), str_key, 512);
 
		char new_key[256];
		for(int i=0; i<128; i++)
			swscanf(str_key+i*2, L"%02X", new_key+i);
		memcpy(g_passkey_big, new_key, 128);
		save_passkey();

		MessageBoxW(hWnd, L"This program will exit now, Restart it to use new user id.", L"Exiting", MB_ICONINFORMATION);

		EndDialog(hWnd, IDOK);
	}
	else if (uid == IDCANCEL)
	{
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}