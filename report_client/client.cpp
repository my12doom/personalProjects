#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>
#include <conio.h>
#pragma  comment(lib, "ws2_32.lib")


int tcp_connect(char *hostname,int port)
{
	struct hostent *host;
	if((host=gethostbyname(hostname))==NULL)
		return -1;

	int sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sockfd==-1)
		return -1;

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family=AF_INET; 
	server_addr.sin_port=htons(port); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);         

	if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
	{
		closesocket(sockfd);
		return -1;
	}

	return sockfd;
}

int main()
{
	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc<2)
		return -1;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
	{
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}

	FILE * f = _wfopen(argv[1], L"rb");
	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	int fd = tcp_connect("bo3d.net", 82);
	//int fd = tcp_connect("127.0.0.1", 82);
	if (fd<0)
	{
		printf("error connectting to server\n");
		getch();
	}

	send(fd, (char*)&filesize, 4, 0);
	char tmp[4096];
	int block_size;
	int sent = 0;
	while(block_size = fread(tmp, 1, 4096, f))
	{
		send(fd, tmp, block_size, 0);

		sent += block_size;

		printf("\r%d/%d sent, %d%%", sent, filesize, (__int64)sent*100/filesize);
	}

	closesocket(fd);

	return 0;
}