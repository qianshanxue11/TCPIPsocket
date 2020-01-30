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
//WINAPI Windows APIĬ�ϵĺ�������Э��
unsigned WINAPI HandleClnt(void* arg);   //The calling convention for system functions
void SendMsg(char* msg, int len);      //������Ϣ��ȫ��socket client����
void ErrorHandler(LPTSTR lpszFunction);  //������΢��MSDN�ٷ����̸��ƹ�����

int clientCnt = 0; //�ͻ��˼���,������������Ĵ��뽫�����ٽ���
SOCKET clientSocks[MAX_CLNT];  //ȫ�ֱ������洢���յ���client��socket�����������������Ĵ��뽫�����ٽ���
HANDLE hMutex;  //HANDLE windows�±�ʾ����



int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET ListenSocket, ClientSocket;
	static SOCKET* presentclientsocket;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;//����SOCKADDR_IN
	int nclientaddrlen;       //client addr���ȣ�acceptʱ��ʹ��
	HANDLE hThread;           //HANDLE����
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
	//���������� Mutual Exclusion
	/*
	����1��NULL,������ȫ��ص�������Ϣ��Ĭ�ϰ�ȫ���ã�NULL
	����2��TRUE�������Ļ������������ڵ��øú������̣߳�ͬʱ����non-signaled״̬
		   FALSE�������Ļ�����������κ��̣߳���ʱ״̬Ϊsignaled
		   ��Ϊnon-signaled״̬����ʱ�������߳��Ѿ����ܷ����ļ��ˣ�ֻ�еȴ��������ǲ�����Դ�ĵȴ���
	����3����������������NULL�����������Ļ���������
	*/
	// Create a mutex with no initial owner
	/*ʹ��CreateMutex����һ�����󣬶������ƿ����Լ�ȡ*/
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
	else if (iResult == 0) //û�д��󣬷���0
	{
		printf("Server is running now ,listening on PORT %d!\n", PORT);
	}


	while (1)
	{

		nclientaddrlen = sizeof(clientaddr);
		ClientSocket = accept(ListenSocket, (struct sockaddr*) & clientaddr, &nclientaddrlen);

		WaitForSingleObject(hMutex, INFINITE);
		/*hMutex��һ���¼��ľ��HANDLE��INFINITE�����ȴ���ʱ��0xFFFFFFFF  // Infinite timeout*/
		/*WaitForSingleObject���Եȴ����¼���Event��Mutex��Semaphore��Process��Thread���͵Ķ���*/
		/*�ٽ����Ŀ�ʼ����������״̬*/
		/*clientCnt��clientSocks�������ٽ���*/
		clientSocks[clientCnt++] = ClientSocket;
		/*�ٽ����Ľ���*/
		ReleaseMutex(hMutex);

		/*_beginthreadex�̰߳�ȫ����CreateThread��������ȫ�ȶ�*/
		/*_beginthreadex()�����ڴ������߳�ʱ����䲢��ʼ��һ��_tiddata�顣
		���_tiddata����Ȼ���������һЩ��Ҫ�̶߳�������ݡ�
		��ʵ�����߳�����ʱ�����Ƚ�_tiddata�����Լ���һ������������
		Ȼ�����̵߳��ñ�׼C���п⺯����strtok()ʱ�ͻ���ȡ��_tiddata��ĵ�ַ�ٽ���Ҫ���������ݴ���_tiddata���С�
		����ÿ���߳̾�ֻ����ʺ��޸��Լ������ݶ�����ȥ�۸������̵߳������ˡ�
		��ˣ�����ڴ�������ʹ�ñ�׼C���п��еĺ���ʱ��
		����ʹ��_beginthreadex()������CreateThread()*/
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
	//�������תΪSOCKET����
	SOCKET hClientSock = *((SOCKET*)arg);
	int strLen = 0, i;
	char msg[MAX_SIZE];

	while ((strLen = recv(hClientSock, msg, sizeof(msg), 0)) != 0)
		SendMsg(msg, strLen);

	WaitForSingleObject(hMutex, INFINITE);
	/*�ٽ����Ŀ�ʼ����������״̬*/
	/*clientCnt��clientSocks�������ٽ���*/
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
	/*�ٽ����Ľ���*/
	ReleaseMutex(hMutex);
	closesocket(hClientSock);
	return 0;
}

//Send to ALL
/*How to send to the one?���ָ�����͸�ĳһ��*/
void SendMsg(char* msg, int len)
{
	int i;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clientCnt; i++)   //clientCntȫ�ֱ���
	{
		send(clientSocks[i], msg, len, 0);   //clientSocksȫ�ֱ���
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
