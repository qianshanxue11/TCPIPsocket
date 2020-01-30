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
	struct sockaddr_in clientaddr;//����SOCKADDR_IN
	int nclientaddrlen;       //client addr���ȣ�acceptʱ��ʹ��
	int iResult;

	/*select define*/
	fd_set reads_fdset, cpyReads_fdset;
	TIMEVAL timeout;
	int fd_Num;
	int fd_max, receivestrLen, i;
	char buf[MAX_SIZE] = { 0 };
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
	else if (iResult == 0) //û�д��󣬷���0
	{
		printf("Server is running now ,listening on PORT %d!\n", PORT);
	}

	FD_ZERO(&reads_fdset);
	//��Ҫ����select�����ڶ���������fd_set����reads_fdsetע�������׽���
	//�������ݵ�����ļ��Ӷ���Ͱ����˷�����׽��֡��ͻ��˵���������ͬ��ͨ������������ɣ�
	//��˷������׽������н��յ����ݣ�����ζ�����µ���������
	FD_SET(ListenSocket, &reads_fdset);

	while (1)
	{
		//����������Ϣ
		cpyReads_fdset = reads_fdset;
		timeout.tv_sec = 5;  //seconds
		timeout.tv_usec = 5000; //microseconds

		//���ӷ�Χ�ǵ�һ�������趨
		/*The select function returns the total number of socket handles that are ready 
		and contained in the fd_set structures, zero if the time limit expired,
		or SOCKET_ERROR if an error occurred.*/
		if ((fd_Num = select(0, &cpyReads_fdset, 0, 0, &timeout)) == SOCKET_ERROR)
			break;
		if (fd_Num == 0)
			continue;

		for (i = 0; i < reads_fdset.fd_count; i++)
		{
			if (FD_ISSET(reads_fdset.fd_array[i], &cpyReads_fdset))
			{
				if (reads_fdset.fd_array[i] == ListenSocket)   //connection request!
				{
					nclientaddrlen = sizeof(clientaddr);
					ClientSocket = accept(ListenSocket, (struct sockaddr*) & clientaddr, &nclientaddrlen);
					FD_SET(ClientSocket, &reads_fdset);
					printf("connected client: %d \n", ClientSocket);
				}
				else //read message
				{
					receivestrLen = recv(reads_fdset.fd_array[i],buf,MAX_SIZE-1,0);
					if (receivestrLen == 0)  //close request
					{
						FD_CLR(reads_fdset.fd_array[i], &reads_fdset);
						closesocket(cpyReads_fdset.fd_array[i]);
						printf("closed client: %d \n", cpyReads_fdset.fd_array[i]);
					}
					else
					{
						send(reads_fdset.fd_array[i], buf, receivestrLen, 0); //echo back to client
					}
				}
			}
		}

	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;

}
