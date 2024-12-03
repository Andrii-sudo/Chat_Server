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

extern HANDLE hMutex;

int main()
{
#pragma region Зчитування даних користувачів з файлів
	std::vector<SUser> vecUsers;
	std::ifstream in("accounts.txt");
	
	// Якщо файлу не існує створюємо пустий файл
	if (!in)
	{
		std::ofstream out("accounts.txt");
		out.close();
		in.open("accounts.txt");
	}

	// Зчитування всіх користувачів, які зареєстровані в системі
	while (!in.eof()) 
	{
		vecUsers.push_back(SUser());
		in >> vecUsers.back().m_strName >> vecUsers.back().m_strPassword;
	}
	vecUsers.pop_back();
	in.close();

	// Зчитування списків усіх чатів користувачів
	for (int i = 0; i < vecUsers.size(); i++)
	{
		std::string strUserFileName(CHATS_PATH + vecUsers[i].m_strName + ".txt");
		
		std::ifstream inChats(strUserFileName);
		if (!inChats)
		{
			std::cout << "Error opening file: " << strUserFileName << std::endl;
			return 1;
		}

		// Зчитування списку всіх чатів і-го користувача 
		while (!inChats.eof())
		{
			std::string strChatName;
			inChats >> strChatName;
			vecUsers[i].m_vecUserChats.push_back(strChatName);
		}
		vecUsers[i].m_vecUserChats.pop_back();
	}
#pragma endregion

#pragma region Ініціалізація сервера
	// Створення потоку оброблення каналів
	SHandleParam* pParam = new SHandleParam(0, NULL, vecUsers);
	CreateThread(NULL, 0, mainPipe, pParam, 0, NULL);


	WSADATA wsaData;

	// Ініціаліазація Winsock
	int	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family   = AF_INET;     // Використовувати IPv4
	hints.ai_socktype = SOCK_STREAM; // Тип сокета: потік (TCP)
	hints.ai_protocol = IPPROTO_TCP; // Протокол: TCP
	hints.ai_flags    = AI_PASSIVE;  // Вказує, що сервер прийматиме з'єднання

	// Визначення локальної адреси і порту для використання сервером
	// NULL означає, що сервер буде прив'язаний до всіх доступних інтерфейсів (0.0.0.0)
	// DEFAULT_PORT — це порт, на якому сервер прийматиме з'єднання
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	// Створення сокету, який буде слухати клієнтські з'єднання
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	
	// Прив'язка сокета до локальної адреси і порту
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
	
	// Початок прослуховування на створеному сокеті (максимальна кількість вхідних з'єднань)
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex == NULL)
	{
		printf("CreateMutex error: %d\n", GetLastError());
		return 1;
	}
#pragma endregion

#pragma region Приймання клієнтських зєднань
	SOCKET ClientSocket = INVALID_SOCKET;

	do
	{
		// Приймаємо клієнтський сокет
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) 
		{
			printf("accept failed: %d\n", WSAGetLastError());
			break;
		}

		// Створюємо новий потік для обробки клієнта
		SHandleParam* pParam = new SHandleParam(ClientSocket, NULL, vecUsers);
		CreateThread(NULL, 0, handleClient, pParam, 0, NULL);
	} while (true);
#pragma endregion

	closesocket(ClientSocket);
	WSACleanup();
	CloseHandle(hMutex);

	return 0;
}