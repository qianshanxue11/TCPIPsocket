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
//WINAPI Windows API默认的函数调用协议
unsigned WINAPI SendMsg(void* arg);   //The calling convention for system functions
unsigned WINAPI RecvMsg(void* arg);   //The calling convention for system functions

void ErrorHandler(LPTSTR lpszFunction);  //错误处理，微软MSDN官方例程复制过来的
char name[NAME_SIZE] = "[DEFAULT]";
char msg[MAX_SIZE];

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hClientSocket;
	//static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	HANDLE hSendThread, hRecvThread;           //HANDLE类型
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

	/*创建发送Thread*/
			/*_beginthreadex线程安全，比CreateThread函数更安全稳定*/
		/*_beginthreadex()函数在创建新线程时会分配并初始化一个_tiddata块。
		这个_tiddata块自然是用来存放一些需要线程独享的数据。
		事实上新线程运行时会首先将_tiddata块与自己进一步关联起来。
		然后新线程调用标准C运行库函数如strtok()时就会先取得_tiddata块的地址再将需要保护的数据存入_tiddata块中。
		这样每个线程就只会访问和修改自己的数据而不会去篡改其它线程的数据了。
		因此，如果在代码中有使用标准C运行库中的函数时，
		尽量使用_beginthreadex()来代替CreateThread()*/
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


	/*创建接收Thread*/
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
	//临界区等待
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	closesocket(hClientSocket);
	WSACleanup();
	return 0;

}

/*How to send to the one?如何指定发送给某一个*/
unsigned WINAPI SendMsg(void* arg)
{
	printf("SendMsgThread run");
	SOCKET handSocket = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + MAX_SIZE];
	while (1)
	{
		fgets(msg, MAX_SIZE, stdin);  //从键盘输入
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
