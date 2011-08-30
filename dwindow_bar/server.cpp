#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

#include "..\AESFile\rijndael.h"
#include "global_funcs_lite.h"

#pragma  comment(lib, "ws2_32.lib")

#define SERVER_PORT 80
#define BACKLOG 10
#define MAX_TIMEOUT 5000

#ifndef DEBUG
#define  printf
#endif

HRESULT init_key()
{
	char username[512] = "my12doom";

	char password[512] = "tester88";

	// SHA1 it!
	unsigned char sha1[20];
	SHA1Hash(sha1, (unsigned char*)password, strlen(password));

	// generate message
	dwindow_message_uncrypt message;
	memcpy(message.passkey, username, 32);
	memcpy(message.requested_hash, sha1, 20);
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
	download_url(url, downloaded, 400);

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


		return hr;
	}
	else
	{
		printf("Activation Failed, Server Response:%s\n.", downloaded);

		return E_FAIL;
	}
}

int my_handle_req(char* request, int client_sock) 
{
	printf("%s\n", request);
	char command[BUFSIZ];
	char arguments[BUFSIZ];

	if (sscanf(request, "%s%s", command, arguments) != 2) 
		return -1;

	if (arguments[0] == '/')
		strcpy(arguments, arguments+1);

	char tmp[666];
	char reply[666];

	strcpy_s(reply, "Unkown Argument");

	if (strlen(arguments) == 64)
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


	return 0;
}


DWORD WINAPI ThreadServer(LPVOID param)
{
	int acc_socket = (int) param;

	int numbytes;
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	if ((numbytes=recv(acc_socket, buf, sizeof(buf)-1, 0)) == -1) 
	{
		closesocket(acc_socket);
		return -1;
	}

	my_handle_req(buf, acc_socket);
	shutdown(acc_socket, SD_SEND);
	int timeout = GetTickCount();
	while (recv(acc_socket, buf, 99, 0) > 0 && GetTickCount() - timeout < MAX_TIMEOUT)
	{
		printf("got %d byte:%s\n", numbytes, buf);
		memset(buf, 0, sizeof(buf));
	}
	closesocket(acc_socket);

	return 0;
}

int make_server_socket() 
{
	struct sockaddr_in server_addr;
	int tempSockId;
	tempSockId = socket(PF_INET, SOCK_STREAM, 0);
 
	if (tempSockId == -1)
		return -1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	memset(&(server_addr.sin_zero), '\0', 8);
	if (bind(tempSockId, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		return -1;
	if (listen(tempSockId, BACKLOG) == -1 ) 
		return -1;
	return tempSockId;
 }

int main(int argc, char * argv[]) 
{
	init_key();

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
	{
		fprintf(stderr, "WSAStartup failed.\n");
		return -1;
	}
	printf("server starting...");
	int server_socket;
	int acc_socket;
	int sock_size = sizeof(struct sockaddr_in);
	struct sockaddr_in user_socket;
	server_socket = make_server_socket();
	if (server_socket == -1) 
	{
		printf("FAIL!\n");
		return -1;
	}
	printf("OK\n");


	while(true) 
	{
		acc_socket = accept(server_socket, (struct sockaddr *)&user_socket, &sock_size);

		DWORD ip = user_socket.sin_addr.S_un.S_addr;
		if (ip != inet_addr("127.0.0.1") &&
			(ip & inet_addr("192.168.0.0")) != inet_addr("192.168.0.0") &&
			(ip & inet_addr("172.16.0.0")) != inet_addr("172.16.0.0"))
		{
			closesocket(acc_socket);
			continue;
		}

		printf("IP: %s.\n", inet_ntoa(user_socket.sin_addr));
		CreateThread(NULL, NULL, ThreadServer, (LPVOID)acc_socket, NULL, NULL);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	main(0,NULL);
}
