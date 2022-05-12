#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib,"ws2_32.lib");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <Ws2tcpip.h>

#define BUF_SIZE 2048
#define BUF_SMALL 100

unsigned WINAPI RequestHandler(void* arg);
char* ContentType(char* file);
void SendData(SOCKET sock, char* ct, char* fileName);
void SendErrorMSG(SOCKET sock);
void ErrorHandling(char* message);

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	HANDLE hThread;
	DWORD dwThreadID;
	int clntAdrSize;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	//������Ӧ
	while (1)
	{
		clntAdrSize = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSize);
		char sendBuf[20] = { '\0' };
		printf("Connection Request : %s:%d\n",
			inet_ntop(AF_INET, (void*)&clntAdr.sin_addr, sendBuf, 16),ntohs(clntAdr.sin_port));
		hThread = (HANDLE)_beginthreadex(
			NULL, 0, RequestHandler, (void*)hClntSock, 0, (unsigned*)&dwThreadID);
	}
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned __stdcall RequestHandler(void* arg)
{
	SOCKET hClntSock = (SOCKET)arg;
	char buf[BUF_SIZE];
	char method[BUF_SMALL];
	char ct[BUF_SMALL];
	char fileName[BUF_SMALL];

	recv(hClntSock, buf, BUF_SIZE, 0);
	if (strstr(buf, "HTTP/") == NULL)		//�鿴�Ƿ�ΪHTTP���������
	{
		SendErrorMSG(hClntSock);
		closesocket(hClntSock);
		return 1;
	}

	strcpy(method, strtok(buf, " /"));
	if (strcmp(method, "GET"))				//�鿴�Ƿ�ΪGET��ʽ������
		SendErrorMSG(hClntSock);

	strcpy(fileName, strtok(NULL," /"));	//�鿴�����ļ���
	strcpy(ct, ContentType(fileName));		//�鿴Content-type
	SendData(hClntSock, ct, fileName);		//��Ӧ
	return 0;
}

char* ContentType(char* file)				//����Content-type
{
	char extension[BUF_SMALL];
	char fileName[BUF_SMALL];
	strcpy(fileName, file);
	strtok(fileName, ".");
	strcpy(extension, strtok(NULL, "."));
	if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
		return "text/html";
	else
		return "text/plain";
}

void SendData(SOCKET sock, char* ct, char* fileName)
{
	char protocol[] = "HTTP/1.0 200 OK\r\n";
	char servName[] = "Server:simple web server\r\n";
	char cntLen[]   = "Content-length:2048\r\n";
	char cntType[BUF_SMALL];
	char buf[BUF_SIZE];
	FILE* sendFile;

	sprintf(cntType, "Content-type:%s\r\n\r\n", ct);
	if ((sendFile = fopen(fileName, "r")) == NULL)
	{
		SendErrorMSG(sock);
		return;
	}

	//����ͷ��Ϣ
	send(sock, protocol, strlen(protocol), 0);
	send(sock, servName, strlen(servName), 0);
	send(sock, cntLen, strlen(cntLen), 0);
	send(sock, cntType, strlen(cntType), 0);

	//������������ 
	while (fgets(buf, BUF_SIZE, sendFile) != NULL)
		send(sock, buf, strlen(buf), 0);

	closesocket(sock);							//��HTTPЭ����Ӧ��Ͽ�
}

void SendErrorMSG(SOCKET sock)
{
	char protocol[] = "HTTP/1.0 400 Bad Request\r\n";
	char servName[] = "Server:simple web server\r\n";
	char cntLen[] = "Content-length:2048\r\n";
	char cntType[] = "Content-type:text/html\r\n\r\n";
	char content[] = "<html><head><title>NETWORK</title></head>"
		"<body><font size=+5><br> �������󣡲鿴�����ļ���������!"
		"</font></body></html>";

	send(sock, protocol, strlen(protocol), 0);
	send(sock, servName, strlen(servName), 0);
	send(sock, cntLen, strlen(cntLen), 0);
	send(sock, cntType, strlen(cntType), 0);
	send(sock, content, strlen(content), 0);
	closesocket(sock);
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
