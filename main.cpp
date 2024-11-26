#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")


#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "ServerFunctions.h"

#define DEFAULT_PORT "12345"

int main()
{
	std::vector<SUser> vecUsers;
	std::ifstream in("accounts.txt");
	
	if (!in) // якщо файлу не ≥снуЇ створюЇмо пустий файл
	{
		std::ofstream out("accounts.txt");
		out.close();
		in.open("accounts.txt");
	}

	while (!in.eof())
	{
		vecUsers.push_back(SUser());
		in >> vecUsers.back().m_strName >> vecUsers.back().m_strPassword;
	}
	vecUsers.pop_back();
	in.close();

	for (int i = 0; i < vecUsers.size(); i++)
	{
		std::ifstream inChats(CHATS_PATH + vecUsers[i].m_strName + ".txt");
		while (!inChats.eof())
		{
			std::string strChatName;
			inChats >> strChatName;
			vecUsers[i].m_vecUserChats.push_back(strChatName);
		}
		vecUsers[i].m_vecUserChats.pop_back();
	}

	WSADATA wsaData;

	// Initialize Winsock
	int	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;

	// Create a SOCKET for the server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	do
	{
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) 
		{
			printf("accept failed: %d\n", WSAGetLastError());
			break;
		}

		SHandleParam* pParam = new SHandleParam(ClientSocket, vecUsers);
		CreateThread(NULL, 0, handleClient, pParam, 0, NULL);
	} while (true);

	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}