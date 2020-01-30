#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

#define MAX_SIZE 10240
#define PORT 8888
//void serveoneclient(void* s);
//void reportconnectinfo(SOCKET s);

int main(int argc, char* argv[])
{
	char filename[81];
	char ip[81];
	if (argc != 3)
	{
		printf("USAGE: upload filename serverip\n");
		exit(1);
	}
	strcpy(filename, argv[1]);
	strcpy(ip, argv[2]);

	WSADATA wsaData;
	SOCKET fileclientsocket;
	FILE *fp;
	struct sockaddr_in serveraddr;
	char data[MAX_SIZE];
	int i;
	int ret;

	if ((fp = fopen(filename, "rb")) == NULL)
	{
		printf("can't open file%s\n", filename);
	}

	WSAStartup(0x202, &wsaData);

	fileclientsocket = socket(AF_INET, SOCK_STREAM, 0);
	memset((void*)&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(ip);
	serveraddr.sin_port = htons(PORT);

	if (connect(fileclientsocket, (struct sockaddr*)&serveraddr,sizeof(struct sockaddr_in)) ==SOCKET_ERROR)
	{
		printf("connect fileserver error\n");
		fclose(fp);
		closesocket(fileclientsocket);
		WSACleanup();
		exit(1);

	};

	printf("send the name of sendfile to server...\n");
	send(fileclientsocket, filename, strlen(filename), 0);
	send(fileclientsocket, "\0", 1, 0);
	printf("send file stream...\n");

	while (1)
	{
		memset((void*)data, 0, sizeof(data));
		i = fread(data, 1, sizeof(data), fp);
		if (i == 0)
		{
			printf("\n send success\n");
			break;
		}
		ret = send(fileclientsocket, data, 1, 0);
		putchar('.');
		if (ret == SOCKET_ERROR)
		{
			printf("\n send failed,file maybe not complete\n");
			break;		
		}
	}

	fclose(fp);
	closesocket(fileclientsocket);
	WSACleanup();
}