#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include <strsafe.h>
#pragma warning(disable : 4996)
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#define MAX_SIZE 100
#define NAME_SIZE 20

//#define PORT 8888
//WINAPI Windows APIĬ�ϵĺ�������Э��
unsigned WINAPI SendMsg(void* arg);   //The calling convention for system functions
unsigned WINAPI RecvMsg(void* arg);   //The calling convention for system functions

void ErrorHandler(LPTSTR lpszFunction);  //������΢��MSDN�ٷ����̸��ƹ�����
char name[NAME_SIZE] = "[DEFAULT]";
char msg[MAX_SIZE];

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hClientSocket;
	//static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	HANDLE hSendThread, hRecvThread;           //HANDLE����
	if (argc != 4) {
		printf("Usage : %s <IP> <port> <yourname>\n", argv[0]);
		exit(1);
	}
	sprintf(name, "[%s]", argv[3]);

	int iResult;
	//WSAStartup(0x202, &wsaData);
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		fputc('\n', stderr);
		return 1;
	}

	// Create a SOCKET for connecting to server
	hClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hClientSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		fputc('\n', stderr);
		WSACleanup();
		return 1;
	}
	// Setup the TCP listening socket
	memset((void*)&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(atoi(argv[2]));

	//connect to server
	if (connect(hClientSocket, (struct sockaddr*) & serveraddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("connect server error\n");
		fputc('\n', stderr);
		closesocket(hClientSocket);
		WSACleanup();
		exit(1);
	};

	/*��������Thread*/
			/*_beginthreadex�̰߳�ȫ����CreateThread��������ȫ�ȶ�*/
		/*_beginthreadex()�����ڴ������߳�ʱ����䲢��ʼ��һ��_tiddata�顣
		���_tiddata����Ȼ���������һЩ��Ҫ�̶߳�������ݡ�
		��ʵ�����߳�����ʱ�����Ƚ�_tiddata�����Լ���һ������������
		Ȼ�����̵߳��ñ�׼C���п⺯����strtok()ʱ�ͻ���ȡ��_tiddata��ĵ�ַ�ٽ���Ҫ���������ݴ���_tiddata���С�
		����ÿ���߳̾�ֻ����ʺ��޸��Լ������ݶ�����ȥ�۸������̵߳������ˡ�
		��ˣ�����ڴ�������ʹ�ñ�׼C���п��еĺ���ʱ��
		����ʹ��_beginthreadex()������CreateThread()*/
		/*If the function succeeds, the return value is a handle to the new thread.*/
	hSendThread = (HANDLE)_beginthreadex(
		NULL,                     // default security attributes
		0,                        // use default stack size 
		SendMsg,                // thread function name
		(void*)&hClientSocket,        // argument to thread function
		0,                            // use default creation flags   
		NULL);							// returns the thread identifier
	if (hSendThread == NULL)
	{
		ErrorHandler(TEXT("CreateSendThread"));
		ExitProcess(3);
	}


	/*��������Thread*/
	hRecvThread = (HANDLE)_beginthreadex(
		NULL,                     // default security attributes
		0,                        // use default stack size 
		RecvMsg,                // thread function name
		(void*)&hClientSocket,        // argument to thread function
		0,                            // use default creation flags   
		NULL);							// returns the thread identifier
	if (hRecvThread == NULL)
	{
		ErrorHandler(TEXT("CreateRecvThread"));
		ExitProcess(3);
	}
	//�ٽ����ȴ�
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	closesocket(hClientSocket);
	WSACleanup();
	return 0;

}

/*How to send to the one?���ָ�����͸�ĳһ��*/
unsigned WINAPI SendMsg(void* arg)
{
	printf("SendMsgThread run");
	SOCKET handSocket = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + MAX_SIZE];
	while (1)
	{
		fgets(msg, MAX_SIZE, stdin);  //�Ӽ�������
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			closesocket(handSocket);
			exit(0);
		}
		sprintf(nameMsg,"%s %s", name, msg);
		send(handSocket, nameMsg, strlen(nameMsg), 0);
	}
	return 0;
}


unsigned WINAPI RecvMsg(void* arg)
{
	printf("RecvMsgThread run");
	SOCKET handSocket = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + MAX_SIZE];
	int strLen;
	while (1)
	{
		strLen = recv(handSocket, nameMsg, NAME_SIZE + MAX_SIZE - 1, 0);
		if (strLen == -1)
			return -1;
		nameMsg[strLen] = 0;
		fputs(nameMsg, stdout);
	}
	return 0;
}


void ErrorHandler(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code.

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	// Free error-handling buffer allocations.

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}
