#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma warning(disable : 4996)
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#define MAX_SIZE 10240
#define PORT 8888

void clientprocessthread(void* s);//client�ϴ����߳�
void reportconnectinfo(SOCKET s);

int main(int argc, char* argv[])
{
	WSADATA wsaData; 
	SOCKET ListenSocket, ClientSocket;
	static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int nclientaddrlen;
	int iResult;
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

	iResult=bind(ListenSocket, (struct sockaddr*) & serveraddr, sizeof(serveraddr));
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	iResult=listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else if(iResult == 0) //û�д��󣬷���0
	{
		printf("Server is run now ,listening on PORT %d!\n", PORT);
	}

	while (1)
	{
		nclientaddrlen = sizeof(clientaddr);
		memset((void*)&clientaddr, 0, nclientaddrlen);
		// Accept a client socket
		ClientSocket = accept(ListenSocket, (struct sockaddr*) & clientaddr, &nclientaddrlen);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		presentclientsocket = &ClientSocket;
		//����һ���̴߳���һ���ͻ���
		_beginthread(clientprocessthread, 0, (void*)presentclientsocket);
		Sleep(1000);
		//���˫����IP��ַ�Ͷ˿���Ϣ
		reportconnectinfo(ClientSocket);
	}
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}
//����ͻ����ļ�����(�ϴ�����)���߳�
void clientprocessthread(void* s)
{
	printf("\n\n�����ͻ����ϴ��̳߳ɹ�!\n\n\n");
	printf("Waiting for cmd from client\n");
	char filename[MAX_PATH];  //�ļ����MAX_PATH
	char data[MAX_SIZE];
	int i;
	char ch;
	char cmdreceive;
	FILE* fp;
	//����ָ��,����Լ��ָ��ֻ��һ���ַ������Զ�ȡһ������
	//recv(*(SOCKET*)s, &cmdreceive, 1, 0);
	if (recv(*(SOCKET*)s, &cmdreceive, 1, 0) != 1)   //��ȡһ���ַ����ж϶�ȡ�������Ƿ�Ϊ1�����������1�����൱��û���ӻ��ȡʧ��
	{
		printf("receive cmd falied\n");
		closesocket(*(SOCKET*)s);
		return;
	}
	//��ȡ����s,g�ֱ���
	switch (cmdreceive)
	{
	case 's'://send from client to server
		printf("Case :s Receive cmd is cmdreceive =%c\n", cmdreceive);
		/*��ȡ�ͻ��˷������ļ������ļ������Ȳ�ȷ������Ҫ���������ַ��Ķ�ȡ����ȡ��client���͵�'0'��Ϊ�ļ�����ȡ���*/
		memset((void*)filename, 0, sizeof(filename));  //ֱ�ӳ�ʼ��Ϊ'0'
		for (i = 0; i < sizeof(filename); i++)
		{
			if (recv(*(SOCKET*)s, &ch, 1, 0) != 1)   //��ȡһ���ַ����ж϶�ȡ�������Ƿ�Ϊ1�����������1�����൱��û���ӻ��ȡʧ��
			{
				printf("filename receive failed or client close connection\n");
				closesocket(*(SOCKET*)s);
				return;
			}

			//if (ch == 0)  //�����ļ�����������Ϊ��ȷ���ļ���������ɣ�client�����'\0'��Ϊ���������յ�֮������
			if (ch == '\0')
			{
				break;
			}

			filename[i] = ch;
		}

		if (i == sizeof(filename))
		{
			printf("Filename too long\n");
			closesocket(*(SOCKET*)s);
			return;
		}
		printf(" Receive Filename is =%s\n", filename);
		/*��ֻ��������ģʽ�����ļ����������ļ�������ͬ���ļ����򸲸ǳɿ��ļ�*/
		fp = fopen(filename, "wb");
		if (fp == NULL)
		{
			printf("Can't open the file with WRITEMODE\n");
			closesocket(*(SOCKET*)s);
			return;
		}
		printf("-----The content from client------\n");
		/*��ȡ�ͻ��˷��������ļ����ݣ����浽data������֮�󱣴浽�ļ�*/
		memset((void*)data, 0, sizeof(data));
		while (1)
		{
			i = recv(*(SOCKET*)s, data, sizeof(data), 0);
			//putchar('.');
			if (i == SOCKET_ERROR)
			{
				printf("Receive failed,file may be incomplete\n");
				break;
			}
			else if (i == 0)
			{
				printf("--RECEIVE SUCCESS---SHOWN BELOW-\n");
				break;
			}
			else
			{
				wprintf(L"Bytes received: %d\n", i);
				//���յ������ݱ��浽�ļ�
				if (fwrite((void*)data, 1, i, fp) == i)
				{
					wprintf(L"Write to file successfully\n");
				}
				else {
					wprintf(L"write to filefailed ,file is not equal the receive data\n");
				}
				//fwrite((void*)data, 1, i, fp);
			}
		}
		printf("%s\n", data);
		printf("-----RECEIVE FINISH--------------\n");
		closesocket(*(SOCKET*)s);  //��Ӳ����Ƿ���Ч
		//WSACleanup();
		reportconnectinfo(*(SOCKET*)s);
		fclose(fp);
		break;
	case 'g'://client download from server
		printf("g Receive cmd is cmdreceive =%c\n", cmdreceive);
		//����client�������ļ���
		memset((void*)filename, 0, sizeof(filename));
		for (i = 0; i < sizeof(filename); i++)
		{
			if (recv(*(SOCKET*)s, &ch, 1, 0) != 1)   //��ȡһ���ַ����ж϶�ȡ�������Ƿ�Ϊ1�����������1�����൱��û���ӻ��ȡʧ��
			{
				printf("filename receive failed or client close connection\n");
				closesocket(*(SOCKET*)s);
				return;
			}
			//if (ch == 0)//"\0"��һ����0
			if (ch == '\0')
			{
				break;
			}
			filename[i] = ch;
		}

		if (i == sizeof(filename))
		{
			printf("Filename too long\n");
			closesocket(*(SOCKET*)s);
			return;
		}
		printf(" Receive Filename is =%s\n", filename);
		printf(" Prepare to open the the Filename is =%s\n", filename);
		fp = fopen(filename, "rb");
		if (fp == NULL)
		{
			printf("Can't open the file with READMODE\n");
			closesocket(*(SOCKET*)s);
			return;
		}

		while (1)
		{
			memset((void*)data, 0, sizeof(data));
			i = fread(data, 1, sizeof(data), fp);
			if (i == 0)
			{
				printf("\n send successly\n");
				break;
			}
			//ret = send(fileclientsocket, data, 1, 0);//1,ֻ����BUGֻ����һ��
			putchar('.');
			if (send(*(SOCKET*)s, data, i, 0) == SOCKET_ERROR)
			{
				printf("\n Send failed,file maybe not complete\n");
				break;
			}
		}
		fclose(fp);
		closesocket(*(SOCKET*)s);  //��Ӳ����Ƿ���Ч
		//WSACleanup();
		reportconnectinfo(*(SOCKET*)s);
		break;
	default:
		printf("default Receive cmd is cmdreceive =%c\n", cmdreceive);
		break;
	}
	printf("thread end\n");
	closesocket(*(SOCKET*)s);  //��Ӳ����Ƿ���Ч
	//WSACleanup();
	_endthread();
	//closesocket(*(SOCKET*)s);
}


//�������˫����IP��ַ�Ͷ˿���Ϣ
void reportconnectinfo(SOCKET s)
{
	struct sockaddr_in addr;
	int naddrlen;

	naddrlen = sizeof(addr);
	memset(&addr, 0, naddrlen);
	getsockname(s, (struct sockaddr*) & addr, &naddrlen);
	printf("--Server address (%s:%d)!--\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	naddrlen = sizeof(addr);
	memset(&addr, 0, naddrlen);
	getpeername(s, (struct sockaddr*) & addr, &naddrlen);
	printf("--One Client (%s:%d)has connected to this server!--\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}




