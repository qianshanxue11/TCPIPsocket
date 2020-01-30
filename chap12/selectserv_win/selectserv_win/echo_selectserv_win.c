#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma warning(disable : 4996)
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#define MAX_SIZE 1024
#define PORT 8888
//void ErrorHandling(char* message);



int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET ListenSocket, ClientSocket;
	static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int nclientaddrlen;
	int iResult;
	char buf[MAX_SIZE] = {0};
	/*select define*/
	fd_set reads_fdset, cpyReads_fdset;
	TIMEVAL timeout;
	int fdNum;
	/*select end*/
	//WSAStartup(0x202, &wsaData);
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	// Setup the TCP listening socket
	memset((void*)&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	serveraddr.sin_port = htons(PORT);

	iResult = bind(ListenSocket, (struct sockaddr*) & serveraddr, sizeof(serveraddr));
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else if (iResult == 0) //没有错误，返回0
	{
		printf("Server is running now ,listening on PORT %d!\n", PORT);
	}

	FD_ZERO(&reads_fdset);
	//向要传到select函数第二个参数的fd_set变量reads_fdset注册服务端套接字
	//接收数据的情况的监视对象就包含了服务端
	FD_SET(ListenSocket, &reads_fdset);



}
