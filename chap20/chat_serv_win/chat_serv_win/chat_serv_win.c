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
#define MAX_SIZE 1024
#define MAX_CLNT 256

#define PORT 8888
//WINAPI Windows API默认的函数调用协议
unsigned WINAPI HandleClnt(void* arg);   //The calling convention for system functions
void SendMsg(char* msg, int len);      //发送消息给全部socket client连接
void ErrorHandler(LPTSTR lpszFunction);  //错误处理，微软MSDN官方例程复制过来的

int clientCnt = 0; //客户端计数,访问这个变量的代码将构成临界区
SOCKET clientSocks[MAX_CLNT];  //全局变量，存储接收到的client的socket句柄，访问这个变量的代码将构成临界区
HANDLE hMutex;  //HANDLE windows下表示对象



int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET ListenSocket, ClientSocket;
	static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;//等于SOCKADDR_IN
	int nclientaddrlen;       //client addr长度，accept时候使用
	HANDLE hThread;           //HANDLE类型
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
	//创建互斥量 Mutual Exclusion
	/*
	参数1：NULL,创建安全相关的配置信息，默认安全设置，NULL
	参数2：TRUE，创建的互斥量对象属于调用该函数的线程，同时进入non-signaled状态
		   FALSE，创建的互斥对象不属于任何线程，此时状态为signaled
		   设为non-signaled状态，这时其它的线程已经不能访问文件了，只有等待，而且是不耗资源的等待。
	参数3：明明互斥量对象，NULL，创建无名的互斥量对象
	*/
	// Create a mutex with no initial owner
	/*使用CreateMutex创建一个对象，对象名称可以自己取*/
	/*If the function succeeds, the return value is a handle to the newly created mutex object.*/	
	hMutex = CreateMutex(
		NULL,   // default security attributes
		FALSE,  // initially not owned
		NULL);   // unnamed mutex
	if (hMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
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


	while (1)
	{

		nclientaddrlen = sizeof(clientaddr);
		ClientSocket = accept(ListenSocket, (struct sockaddr*) & clientaddr, &nclientaddrlen);

		WaitForSingleObject(hMutex, INFINITE);
		/*hMutex是一个事件的句柄HANDLE，INFINITE参数等待的时间0xFFFFFFFF  // Infinite timeout*/
		/*WaitForSingleObject可以等待以下几种Event，Mutex，Semaphore，Process，Thread类型的对象*/
		/*临界区的开始，进入阻塞状态*/
		/*clientCnt和clientSocks必须在临界区*/
		clientSocks[clientCnt++] = ClientSocket;
		/*临界区的结束*/
		ReleaseMutex(hMutex);

		/*_beginthreadex线程安全，比CreateThread函数更安全稳定*/
		/*_beginthreadex()函数在创建新线程时会分配并初始化一个_tiddata块。
		这个_tiddata块自然是用来存放一些需要线程独享的数据。
		事实上新线程运行时会首先将_tiddata块与自己进一步关联起来。
		然后新线程调用标准C运行库函数如strtok()时就会先取得_tiddata块的地址再将需要保护的数据存入_tiddata块中。
		这样每个线程就只会访问和修改自己的数据而不会去篡改其它线程的数据了。
		因此，如果在代码中有使用标准C运行库中的函数时，
		尽量使用_beginthreadex()来代替CreateThread()*/
		/*If the function succeeds, the return value is a handle to the new thread.*/
		hThread =(HANDLE)_beginthreadex(
				NULL,                     // default security attributes
				0,                        // use default stack size 
				HandleClnt,                // thread function name
				(void*)&ClientSocket,        // argument to thread function
				0,                            // use default creation flags   
				NULL);							// returns the thread identifier
		if (hThread == NULL)
		{
			ErrorHandler(TEXT("CreateThread"));
			ExitProcess(3);
		}

		printf("Connected client IP :%s \n", inet_ntoa(clientaddr.sin_addr));
		printf("connected client: %d \n", ClientSocket);
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;

}

unsigned WINAPI HandleClnt(void* arg)
{
	//输入参数转为SOCKET类型
	SOCKET hClientSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char msg[MAX_SIZE];

	while ((strLen = recv(hClientSock, msg, sizeof(msg), 0)) != 0)
		SendMsg(msg, strLen);

	WaitForSingleObject(hMutex, INFINITE);
	/*临界区的开始，进入阻塞状态*/
	/*clientCnt和clientSocks必须在临界区*/
	for (i = 0; i < clientCnt; i++)
	{
		if (hClientSock == clientSocks[i])
		{
			while (i++ < clientCnt - 1)
				clientSocks[i] = clientSocks[i + 1];
			break;
		}
	}
	clientCnt--;
	/*临界区的结束*/
	ReleaseMutex(hMutex);
	closesocket(hClientSock);
	return 0;
}

//Send to ALL
/*How to send to the one?如何指定发送给某一个*/
void SendMsg(char* msg, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clientCnt; i++)   //clientCnt全局变量
	{
		send(clientSocks[i], msg, len, 0);   //clientSocks全局变量
	}
	ReleaseMutex(hMutex);


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
