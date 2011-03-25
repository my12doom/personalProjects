#include <winsock.h>
#include <wininet.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;
#define SERVER_PORT 80
#define MAX_CONNECTION 100           //同时等待的连接个数

int make_server_socket() 
{
	struct sockaddr_in server_addr;					//服务器地址结构体
	int tempSockId;									//临时存储socket描述符
	tempSockId = socket(PF_INET, SOCK_STREAM, 0);
 
	if (tempSockId == -1)							//如果返回值为－1 则出错
		return -1;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(&(server_addr.sin_zero), '\0', 8);
	if (bind(tempSockId, (struct sockaddr *)&server_addr,
		sizeof(server_addr)) == -1)
	{
		//绑定服务如果出错则返回－1
		return -1;
	}
	if (listen(tempSockId, MAX_CONNECTION) == -1 ) 
	{
		return -1;
	}
	return tempSockId;
 }

/*
void main(int argc, char * argv[]) 
{
	int server_socket = make_server_socket();
	while(true)
	{
		acc_socket = accept(server_socket, (struct sockaddr *)&user_socket, &sock_size); //接收连接

		MSG_WAITALL
	}

}
*/