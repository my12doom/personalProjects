#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>
#include <conio.h>
#include <atlbase.h>
#include "ICommand.h"

#pragma  comment(lib, "ws2_32.lib")

#define MAX_CONNECTION 10
#define NETWORK_TIMEOUT 5000
#define HEARTBEEP_TIMEOUT 120000	// 2 minute
#define MAX_USER_SLOT 2048
#define NETWORK_DONE (WM_USER+1)
#define UM_ICONNOTIFY (WM_USER+2)
int server_socket = -1;
bool server_stopping = false;
int server_port = 8080;
extern ICommandReciever *command_reciever;


DWORD WINAPI handler_thread(LPVOID param);
int my_handle_req(char* data, int size, DWORD ip, int client_sock);
HRESULT init_winsock();

int TCPTest()
{
	if (FAILED(init_winsock()))
		return -1;

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

extern bool auth/* = false*/;

DWORD WINAPI handler_thread(LPVOID param)
{
	int acc_socket = *(int*)param;
	DWORD ip = ((DWORD*)param)[1];

	delete param;

	int numbytes;
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	const char *welcome_string = "DWindow Network v0.0.1";
	send(acc_socket, welcome_string, strlen(welcome_string), 0);
	send(acc_socket, "\n", 1, 0);
	while ((numbytes=recv(acc_socket, buf, sizeof(buf)-1, 0)) > 0) 
	{
		my_handle_req(buf, numbytes, ip, acc_socket);
	}

	shutdown(acc_socket, SD_SEND);
	int timeout = GetTickCount();
	while (recv(acc_socket, buf, 99, 0) > 0 && GetTickCount() - timeout < NETWORK_TIMEOUT)
	{
		printf("got %d byte:%s\n", numbytes, buf);
		memset(buf, 0, sizeof(buf));
	}
	closesocket(acc_socket);

	auth = false;

	return 0;
}

char line[1024];
int p = 0;
int code_page = CP_ACP;
wchar_t *out = new wchar_t[1024000];
wchar_t *out2 = new wchar_t[1024000];
char *outA = new char[1024000];
int my_handle_req(char* data, int size, DWORD ip, int client_sock) 
{
	wchar_t line_w[1024];
	for (int i=0; i<size; i++)
	{
		if (data[i] != 0xA && data[i] != 0xD && p<1024)
			line[p++] = data[i];
		else
		{
			if(data[i] == 0xD)
			{
				line[p] = NULL;
				MultiByteToWideChar(CP_UTF8, 0, line, 1024, line_w, 1024);

				wprintf(L"%s\n", line_w);
				out[0] = NULL;
				HRESULT hr = command_reciever->execute_command_line(line_w, out);
				if (hr == S_JPG)
				{
					int s = send(client_sock, ((char*)out), *((int*)out)+4, 0);
					if (s<0)
					{
						int e =  WSAGetLastError();
						break;
					}
				}

				else
				{
					wprintf(L"%08x,%s\n", hr, out);
					swprintf(out2, L"%08x,", hr);
					wcscat(out2, out);
					int o = WideCharToMultiByte(CP_UTF8, 0, out2, -1, outA, 1024000, NULL, NULL);
					send(client_sock, outA, strlen(outA), 0);
					send(client_sock, "\n", 1, 0);
				}


			}
			else
				p = 0;
		}
	}

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