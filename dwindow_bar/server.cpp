#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>
#include <conio.h>
#include <atlbase.h>

#include "..\AESFile\rijndael.h"
#include "global_funcs_lite.h"
#include "CCritSec.h"

#include "resource.h"

#pragma  comment(lib, "ws2_32.lib")

#define MAX_CONNECTION 10
#define NETWORK_TIMEOUT 5000
#define HEARTBEEP_TIMEOUT 120000	// 2 minute
#define MAX_USER_SLOT 2048
#define NETWORK_DONE (WM_USER+1)
#define UM_ICONNOTIFY (WM_USER+2)
const UINT WM_TASKBARCREATED = 	RegisterWindowMessage(_T("TaskbarCreated"));//这个消息是系统开始菜单，任务栏创建时发出的

typedef struct _active_user_info
{
	DWORD tick;
	DWORD ip;
} user_info;


#ifndef DEBUG
//#define  printf
#endif


user_info active_users[MAX_USER_SLOT];
DWORD banned_users[MAX_USER_SLOT];
CCritSec cs;
AutoSettingString username(L"UserName", L"ztester");
AutoSettingString saved_password(L"Password", L"FWANRW");
AutoSetting<int> server_port(L"ServerPort", 80);
char password[512] = "";
AutoSetting<bool> remember_password(L"RememberPassword", false);
AutoSetting<bool> auto_login(L"AutoLogin", false);
bool server_stopping = false;
int max_user = 0;
int server_socket = -1;
bool login_reopen = false;		// true -> no auto login
HANDLE server_thread_handle = INVALID_HANDLE_VALUE;
HWND g_hwnd_login = NULL;
HWND g_hwnd_main = NULL;

HRESULT download_passkey(char *out = NULL);
int my_handle_req(char* request, DWORD ip, int client_sock);
DWORD WINAPI handler_thread(LPVOID param);
int make_server_socket();
int main(int argc, char * argv[]);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
INT_PTR CALLBACK login_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK main_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
HRESULT init_winsock();
DWORD WINAPI server_thread(LPVOID param);
HRESULT start_server();
HRESULT stop_server();
HRESULT login_ip(DWORD ip);
HRESULT logout_ip(DWORD ip);
HRESULT clear_timeout_ip();
HRESULT ban_ip(DWORD ip);
HRESULT unban_ip(DWORD ip);
bool is_ip_banned(DWORD ip);
HRESULT add_to_list(int control_id, DWORD ip);
HRESULT del_from_list(int control_id, DWORD ip);


HRESULT login_ip(DWORD ip)
{
	if (is_ip_banned(ip))
		return E_FAIL;

	int tick = GetTickCount();
	DWORD timeout_ip = 0;
	HRESULT hr = ERROR_NO_MORE_USER_HANDLES;

	{
		CAutoLock lck(&cs);
		for(int i=0; i<max_user; i++)
		{
			if (active_users[i].ip == ip)
			{
				active_users[i].tick = tick;
				return S_OK;
			}
		}

		for(int i=0; i<max_user; i++)
		{
			if (tick - active_users[i].tick > HEARTBEEP_TIMEOUT)
			{
				if (active_users[i].ip)
					timeout_ip = active_users[i].ip;

				active_users[i].tick = tick;
				active_users[i].ip = ip;

				hr = S_OK;
				break;
			}
		}
	}

	if (hr == S_OK)
	{
		add_to_list(IDC_ACTIVE_LIST, ip);
		del_from_list(IDC_ACTIVE_LIST, timeout_ip);
	}

	return hr;
}

HRESULT logout_ip(DWORD ip)
{
	{
		CAutoLock lck(&cs);
		for(int i=0; i< max_user; i++)
		{
			if (active_users[i].ip == ip)
			{
				active_users[i].ip = 0;
				active_users[i].tick = 0;
			}
		}

		int free_pos = 0;
		for(int i=0; i< max_user; i++)
		{
			if (active_users[i].ip == 0 || GetTickCount() - active_users[i].tick > HEARTBEEP_TIMEOUT)
			{
				active_users[i].ip = 0;
				active_users[i].tick = 0;
				free_pos ++;
			}
		}

		printf("Free Slot Left: %d\n", free_pos);
	}

	del_from_list(IDC_ACTIVE_LIST, ip);

	return S_OK;
}

HRESULT clear_timeout_ip()
{
	int free_pos = 0;
	int timeout = 0;
	DWORD timeout_ips[MAX_USER_SLOT];
	{
		CAutoLock lck(&cs);
		for(int i=0; i< max_user; i++)
		{
			if (active_users[i].ip == 0 || GetTickCount() - active_users[i].tick > HEARTBEEP_TIMEOUT)
			{
				if (active_users[i].ip)
					timeout_ips[timeout++] = active_users[i].ip;
				active_users[i].ip = 0;
				active_users[i].tick = 0;
				free_pos ++;
			}
		}
	}

	for(int i=0; i<timeout; i++)
		del_from_list(IDC_ACTIVE_LIST, timeout_ips[i]);

	return S_OK;
}

HRESULT add_to_list(int control_id, DWORD ip)
{
	if (g_hwnd_main == NULL)
		return S_FALSE;

	HWND list = GetDlgItem(g_hwnd_main, control_id);

	char str_ip[50];
	BYTE *b_ip = (BYTE*)&ip;
	sprintf(str_ip, "%d.%d.%d.%d", b_ip[0], b_ip[1], b_ip[2], b_ip[3]);

	SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)str_ip);
	return S_OK;
}

HRESULT del_from_list(int control_id, DWORD ip)
{
	if (g_hwnd_main == NULL)
		return S_FALSE;

	HWND list = GetDlgItem(g_hwnd_main, control_id);

	char str_ip[50];
	BYTE *b_ip = (BYTE*)&ip;
	sprintf(str_ip, "%d.%d.%d.%d", b_ip[0], b_ip[1], b_ip[2], b_ip[3]);

	HRESULT hr = S_FALSE;

	while (true)
	{
		int index = SendMessageA(list, LB_FINDSTRINGEXACT, -1, (LPARAM)str_ip);
		if (index == LB_ERR)
			break;

		SendMessage(list, LB_DELETESTRING, index, 0);
		hr = S_OK;
	}
	return hr;
}

HRESULT ban_ip(DWORD ip)
{
	{
		CAutoLock lck(&cs);
		for(int i=0; i<MAX_USER_SLOT; i++)
		{
			if (banned_users[i] == 0)
			{
				banned_users[i] = ip;
				break;
			}
		}
	}

	add_to_list(IDC_BANNED_LIST, ip);

	return E_FAIL;		// list full
}

bool is_ip_banned(DWORD ip)
{
	CAutoLock lck(&cs);
	for(int i=0; i<MAX_USER_SLOT; i++)
		if (banned_users[i] == ip)
			return TRUE;

	return FALSE;
}

HRESULT unban_ip(DWORD ip)
{
	{
		CAutoLock lck(&cs);
		for(int i=0; i<MAX_USER_SLOT; i++)
			if (banned_users[i] == ip)
				banned_users[i] = 0;
	}

	del_from_list(IDC_BANNED_LIST, ip);

	return S_OK;
}

HRESULT download_passkey(char *out /*= NULL*/)
{
	USES_CONVERSION;

	// SHA1 it!
	unsigned char sha1[20];
	SHA1Hash(sha1, (unsigned char*)password, strlen(password));

	// generate message
	dwindow_message_uncrypt message;
	memcpy(message.passkey, W2A(username), 32);
	memcpy(message.requested_hash, sha1, 20);
	memcpy(message.password_uncrypted, password, min(19, strlen(password)));
	message.password_uncrypted[min(19, strlen(password))] = 0;
	message.client_time = _time64(NULL);
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() &0xff;
	message.zero = 0;
	unsigned char encrypted_message[128];
	RSA_dwindow_network_public(&message, encrypted_message);

	char url[512];
	strcpy(url, g_server_address);
	strcat(url, g_server_gen_key);
	strcat(url, "?");
	char tmp[3];
	for(int i=0; i<128; i++)
	{
		wsprintfA(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char downloaded[400] = "";
	memset(downloaded, 0, 400);
	download_url(url, downloaded, 399);

	OutputDebugStringA(url);
	OutputDebugStringA("\n");
	OutputDebugStringA(downloaded);

	if (strlen(downloaded) == 256)
	{
		unsigned char new_key[256];
		for(int i=0; i<128; i++)
			sscanf(downloaded+i*2, "%02X", new_key+i);
		AESCryptor aes;
		aes.set_key(message.random_AES_key, 256);
		for(int i=0; i<128; i+=16)
			aes.decrypt(new_key+i, new_key+i);

		memcpy(&g_passkey_big, new_key, 128);

		HRESULT hr = check_passkey();
		if (FAILED(hr))
			printf("Activation Failed, Server Response:%s\n.", downloaded);
		else
			printf("Activation OK\n");

		DWORD e[32];
		dwindow_passkey_big p;
		BigNumberSetEqualdw(e, 65537, 32);
		RSA((DWORD*)&p, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

		max_user = p.max_bar_user;
		printf("max user: %d.\n", max_user);

		return hr;
	}
	else
	{
		if (out)
			strcpy(out, downloaded);

		return E_FAIL;
	}
}

int my_handle_req(char* request, DWORD ip, int client_sock) 
{
	if (ip != inet_addr("127.0.0.1") &&
		(ip & inet_addr("192.168.0.0")) != inet_addr("192.168.0.0") &&
		(ip & inet_addr("172.16.0.0")) != inet_addr("172.16.0.0"))
	{
		closesocket(client_sock);
		return -1;
	}

	//printf("IP: %s.\n", inet_ntoa(ip));

	printf("%s\n", request);
	char command[1024];
	char arguments[1024];

	if (sscanf(request, "%s%s", command, arguments) != 2) 
		return -1;

	if (arguments[0] == '/')
		strcpy(arguments, arguments+1);

	char tmp[666];
	char reply[666];

	strcpy_s(reply, "Unkown Argument");

	if (strlen(arguments) == 64)
	{
		if (is_ip_banned(ip))
		{
			strcpy_s(reply, "IP denied");
		}
		else if (SUCCEEDED(login_ip(ip)))
		{
			srand(time(NULL));
			unsigned char random = rand()&0xff;
			unsigned char key_got[40];
			for(int i=0; i<32; i++)
			{
				sscanf(arguments+i*2, "%02X", key_got+i);
				key_got[i] ^= random;
			}

			AESCryptor aes;
			aes.set_key(key_got, 256);

			unsigned char crypted_passkey[128];
			for(int i=0; i<128; i+=16)
				aes.encrypt((unsigned char*)g_passkey_big+i, (unsigned char*)crypted_passkey+i);

			reply[0] = NULL;
			for(int i=0; i<128; i++)
			{
				sprintf(tmp, "%02X", crypted_passkey[i]);
				strcat_s(reply, tmp);
			}
		}
		else
		{
			strcpy_s(reply, "SERVER FULL");
		}
	}
	else if (strlen(arguments) == 6 && strcmp(arguments, "LOGOUT") == 0)
	{
		logout_ip(ip);
		strcpy_s(reply, "OK");
	}

	// send reply info
	int content_length = strlen(reply);

	//header
	strcpy(tmp, "HTTP/1.1 200 OK\r\n");
	send(client_sock, tmp, strlen(tmp), 0);

	sprintf(tmp, "Content-Length: %d\r\n", content_length);
	send(client_sock, tmp, strlen(tmp), 0);

	strcpy(tmp, "Content-Type: text/html;charset=GBK\r\n");
	//send(client_sock, line, strlen(line), 0);

	strcpy(tmp, "Server: DWindow-NetBar/1.1\r\n");
	//send(client_sock, line, strlen(line), 0);

	strcpy(tmp, "\r\n");
	send(client_sock, tmp, strlen(tmp), 0);

	//content
	send(client_sock, reply, strlen(reply), 0);

	printf("Reply: %s\n<END OF REPLAY>\n\n", reply);

	return 0;
}


DWORD WINAPI handler_thread(LPVOID param)
{
	int acc_socket = *(int*)param;
	DWORD ip = ((DWORD*)param)[1];

	delete param;

	int numbytes;
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	if ((numbytes=recv(acc_socket, buf, sizeof(buf)-1, 0)) == -1) 
	{
		closesocket(acc_socket);
		return -1;
	}

	my_handle_req(buf, ip, acc_socket);
	shutdown(acc_socket, SD_SEND);
	int timeout = GetTickCount();
	while (recv(acc_socket, buf, 99, 0) > 0 && GetTickCount() - timeout < NETWORK_TIMEOUT)
	{
		printf("got %d byte:%s\n", numbytes, buf);
		memset(buf, 0, sizeof(buf));
	}
	closesocket(acc_socket);

	return 0;
}

HRESULT init_winsock()
{
	static bool inited = false;
	if (inited)
		return S_FALSE;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
		return E_FAIL;

	inited = true;

	return S_OK;
}

DWORD WINAPI server_thread(LPVOID param)
{
	// init and listen
	SOCKADDR_IN server_addr;
	int tmp_socket = socket(PF_INET, SOCK_STREAM, 0);

	if (tmp_socket == -1)
		return -1;

	printf("Server Starting @ %d....", (int)server_port);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((int)server_port);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	memset(&(server_addr.sin_zero), 0, 8);
	if (bind(tmp_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == -1)
	{
		printf("FAIL!\n");
		server_socket = -1;
		closesocket(tmp_socket);
		return -1;
	}
	if (listen(tmp_socket, MAX_CONNECTION) == -1 ) 
	{
		server_socket = -1;
		printf("FAIL!\n");
		closesocket(tmp_socket);
		return -1;
	}
	printf("OK\n");

	int sock_size = sizeof(SOCKADDR_IN);
	SOCKADDR_IN user_socket;

	// wait for connection
	server_socket = tmp_socket;

	while(!server_stopping)
	{
		int acc_socket = accept(tmp_socket, (SOCKADDR*)&user_socket, &sock_size);

		if (server_stopping)
			break;

		if (acc_socket == -1)
			continue;

		DWORD ip = user_socket.sin_addr.S_un.S_addr;
		DWORD *para = new DWORD[2];
		para[0] = acc_socket;
		para[1] = ip;
		CreateThread(NULL, NULL, handler_thread, para, NULL, NULL);
	}

	server_socket = -1;
	return 0;
}

HRESULT start_server()
{
	HRESULT hr = stop_server();
	if (FAILED(hr))
		return hr;

	server_socket = 0;
	server_stopping = false;
	server_thread_handle = CreateThread(NULL, NULL, server_thread, NULL, NULL, NULL);
	while (!server_socket)
		Sleep(1);

	return server_socket == -1 ? E_FAIL : S_OK;
}

HRESULT stop_server()
{
	if (server_thread_handle == INVALID_HANDLE_VALUE)
		return S_FALSE;
	if (server_socket == -1)
		return S_FALSE;

	server_stopping = true;
	closesocket(server_socket);
	WaitForSingleObject(server_thread_handle, INFINITE);

	CloseHandle(server_thread_handle);
	server_thread_handle = INVALID_HANDLE_VALUE;
	memset(active_users, 0, sizeof(active_users));
	return S_OK;
}

INT_PTR CALLBACK main_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int code = HIWORD(wParam);
			int id = LOWORD(wParam);
			HWND ctl = (HWND)lParam;

			if (id == IDC_EXIT || id == IDC_LOGOUT)
			{
				// delete icon
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = hDlg;
				nid.uID = IDI_ICON1;
				Shell_NotifyIcon(NIM_DELETE, &nid);
				SendMessage(hDlg, WM_SYSCOMMAND,SC_RESTORE,0);
				PostMessage(hDlg, WM_USER,0 ,0);

				EndDialog(hDlg, id);
				g_hwnd_main = NULL;
			}

			if (id == IDC_BAN)
			{
				HWND list = GetDlgItem(hDlg, IDC_ACTIVE_LIST);
				int index = SendMessage(list, LB_GETCURSEL, 0, 0);
				if (index != LB_ERR)
				{
					char toban[20] = "0.0.0.0";
					SendMessageA(list, LB_GETTEXT, index, (LPARAM)toban);
					DWORD ip = inet_addr(toban);
					if (ip && ip != 0xffffffff)
					{
						logout_ip(ip);
						ban_ip(ip);
					}
				}
			}

			if (id == IDC_UNBAN)
			{
				HWND list = GetDlgItem(hDlg, IDC_BANNED_LIST);
				int index = SendMessage(list, LB_GETCURSEL, 0, 0);
				if (index != LB_ERR)
				{
					char tounban[20] = "0.0.0.0";
					SendMessageA(list, LB_GETTEXT, index, (LPARAM)tounban);
					DWORD ip = inet_addr(tounban);
					if (ip && ip != 0xffffffff)
						unban_ip(ip);
				}
			}
			if (id == IDC_HIDE)
				ShowWindow(hDlg, SW_HIDE);
			
			if (code == EN_CHANGE)
			{
				char port[50];
				GetWindowTextA(ctl, port, 50);
				stop_server();
				server_port = atoi(port);

				HRESULT hr = server_port >= 0xffff ? E_FAIL : start_server();

				ShowWindow(GetDlgItem(hDlg, IDC_WARNING), FAILED(hr)? SW_SHOW : SW_HIDE);

				printf("EN_CHANGE\n");
			}

		}
		break;
	case WM_TIMER:
		clear_timeout_ip();
		break;

	case UM_ICONNOTIFY:
		if (WM_LBUTTONDOWN == lParam || WM_RBUTTONDOWN == lParam)
		{
			ShowWindow(hDlg, SW_SHOW);
			SendMessage(hDlg, WM_SYSCOMMAND,SC_RESTORE,0);
			SetForegroundWindow(hDlg);
		}

		break;

	case WM_INITDIALOG:
		{
			g_hwnd_main = hDlg;

			// window icon
			HICON icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hDlg, WM_SETICON, TRUE, (LPARAM)icon);
			SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)icon);

			// window caption
			char caption[200];
			sprintf(caption, "DWindow网吧版服务器(%d用户授权)", max_user);
			SetWindowTextA(hDlg, caption);

			// banned ips
			for(int i=0; i<MAX_USER_SLOT; i++)
				if (banned_users[i])
					add_to_list(IDC_BANNED_LIST, banned_users[i]);

			// timer
			SetTimer(hDlg, 0, 5000, NULL);

			// port
			char port[50];
			sprintf(port, "%d", (int)server_port);
			SetWindowTextA(GetDlgItem(hDlg, IDC_PORT), port);

			// tray icon
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hDlg;
			nid.uID = IDI_ICON1;
			nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
			nid.uCallbackMessage = UM_ICONNOTIFY;
			nid.hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
			strcpy(nid.szTip, "DWindow网吧版服务器端");

			Shell_NotifyIcon(NIM_ADD, &nid);

		}
		break;

	case WM_CLOSE:
		SendMessage(hDlg, WM_COMMAND, IDC_HIDE, 0);
		return FALSE;
		break;

	default:
		if (msg == WM_TASKBARCREATED)
			return main_proc(hDlg, WM_INITDIALOG, 0, 0);
		return FALSE;
	}

	return TRUE; // Handled message
}

INT_PTR CALLBACK login_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDC_LOGIN)
			{

				GetWindowTextW(GetDlgItem(hDlg, IDC_USERNAME), username, 512);
				GetWindowTextA(GetDlgItem(hDlg, IDC_PASSWORD), password, 512);
				username.save();

				USES_CONVERSION;
				saved_password = remember_password ? A2W(password) : L"";

				SetWindowTextW(GetDlgItem(hDlg, IDC_LOGIN), L"登录中.....");

				char out[512];
				HRESULT hr = download_passkey(out);

				SetWindowTextW(GetDlgItem(hDlg, IDC_LOGIN), L"登录");

				if (SUCCEEDED(hr) && max_user == 0)
				{
					hr = E_FAIL;
					strcpy(out, "非网吧用户");
				}
				if (FAILED(hr))
				{
					char tmp[666];
					sprintf(tmp, "登录失败，服务器响应：%s", out);
					MessageBoxA(hDlg, tmp, "Error", MB_ICONERROR);
				}

				else
				{
					g_hwnd_login = NULL;
					EndDialog(hDlg, id);
				}
			}

			else if (id == IDC_AUTOLOGIN || id == IDC_REMEMBER)
			{
				HWND h_auto_login = GetDlgItem(hDlg, IDC_AUTOLOGIN);
				HWND h_remember = GetDlgItem(hDlg, IDC_REMEMBER);

				auto_login = SendMessage(h_auto_login, BM_GETCHECK, 0, 0);
				remember_password = SendMessage(h_remember, BM_GETCHECK, 0, 0);
			}
		}
		break;

	case WM_TIMER:
		{
			KillTimer(hDlg, 0);
			ShowWindow(hDlg, SW_SHOW);
			if (auto_login && remember_password && !login_reopen)
				SendMessage(hDlg, WM_COMMAND, IDC_LOGIN, 0);
		}
		break;

	case WM_SHOWWINDOW:
		SetTimer(hDlg, 0, 500, NULL);
		break;

	case WM_INITDIALOG:
		{
			g_hwnd_login = hDlg;
			HWND h_auto_login = GetDlgItem(hDlg, IDC_AUTOLOGIN);
			HWND h_remember = GetDlgItem(hDlg, IDC_REMEMBER);

			SendMessage(h_auto_login, BM_SETCHECK, auto_login, 0);
			SendMessage(h_remember, BM_SETCHECK, remember_password, 0);

			SetWindowTextW(GetDlgItem(hDlg, IDC_USERNAME), username);
			if (remember_password)
				SetWindowTextW(GetDlgItem(hDlg, IDC_PASSWORD), saved_password);
		}
		break;

	case WM_CLOSE:
		g_hwnd_login = NULL;
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}
int main(int argc, char * argv[]) 
{
	USES_CONVERSION;
	memset(active_users, 0, sizeof(active_users));
	memset(banned_users, 0, sizeof(banned_users));
	if (FAILED(init_winsock()))
	{
		MessageBoxA(NULL, "网络初始化失败", "....", MB_ICONERROR);
		return -1;
	}

logout:
	int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, login_proc );
	login_reopen = true;
	if (o == 0)
		return 0;

	printf("Username: %s\nPassword: %s\n", W2A(username), password);

	o = DialogBox( NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, main_proc);

	stop_server();

	if (o == IDC_LOGOUT)
		goto logout;

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main(0,NULL);
}
