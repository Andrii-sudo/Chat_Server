#include "ServerFunctions.h"

HANDLE hMutex;

DWORD WINAPI handleClientSocket(LPVOID lpParam)
{
	SHandleParam* pParam = static_cast<SHandleParam*>(lpParam);

	SOCKET ClientSocket = pParam->m_ClientSocket;
	std::vector<SUser>& vecUsers = pParam->m_vecUsers;

	// Прийняття запиту від клієнта
	std::vector<char> vecRecvBuf(1024);
	int iResult, iSendResult;

	iResult = recv(ClientSocket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
	if (iResult > 0)
	{
		// Визначення типу запиту (перші три літери)
		auto itEnd = std::find(vecRecvBuf.begin(), vecRecvBuf.end(), '\0');		  // \0 є кінцем буферу
		std::string strTypeOfRequest(vecRecvBuf.begin(), vecRecvBuf.begin() + 3); // Тип запиту  [0, 3)
		std::string strOtherInfo(vecRecvBuf.begin() + 4, itEnd);				  // Решта даних [4, itEnd)

		// Оброблення запиту
		std::vector<char> vecSendBuf;
		
		WaitForSingleObject(hMutex, INFINITE);  
		if (strTypeOfRequest == "reg")
		{
			if (createAccount(strOtherInfo, vecUsers))
			{
				vecSendBuf = { 'Y' };
			}
			else
			{
				vecSendBuf = { 'N' };
			}
		}
		else if (strTypeOfRequest == "log")
		{
			if (loginAccount(strOtherInfo, vecUsers))
			{
				vecSendBuf = { 'Y' };
			}
			else
			{
				vecSendBuf = { 'N' };
			}
		}
		else if (strTypeOfRequest == "ser")
		{
			vecSendBuf = searchUsers(strOtherInfo, vecUsers);
		}
		else if (strTypeOfRequest == "mes")
		{
			sendMessage(strOtherInfo, vecUsers);
			vecSendBuf = { 'Y' };
		}
		else if (strTypeOfRequest == "upd")
		{
			vecSendBuf = updateChats(strOtherInfo, vecUsers);
		}
		ReleaseMutex(hMutex);

		// Відправлення відповіді
		iSendResult = send(ClientSocket, vecSendBuf.data(), vecSendBuf.size(), 0);
		if (iSendResult == SOCKET_ERROR)
		{
			printf("send failed: %d\n", WSAGetLastError());
		}
	}
	else if (iResult == 0)
	{
		printf("Connection closing...\n");
	}
	else
	{
		printf("recv failed: %d\n", WSAGetLastError());
	}

	delete pParam;
	closesocket(ClientSocket);
	return 0;
}

void getNameAndPassword(std::string strInfo, std::string& strName, std::string& strPassword)
{
	auto itSpace = std::find(strInfo.begin(), strInfo.end(), ' '); 
	strName.assign(strInfo.begin(), itSpace);		// [0, itSpace)
	strPassword.assign(itSpace + 1, strInfo.end()); // [itSpace, end)
}

bool createAccount(std::string strInfo, std::vector<SUser>& vecUsers)
{
	std::string strName;
	std::string strPassword;
	getNameAndPassword(strInfo, strName, strPassword);

	SUser userNew;
	userNew.m_strName = strName;
	userNew.m_strPassword = strPassword;

	// Перевірка чи існує акаунт з таким іменем
	for (int i = 0; i < vecUsers.size(); i++)
	{
		if (vecUsers[i].m_strName == userNew.m_strName)
		{
			return false;
		}
	}

	vecUsers.push_back(userNew);

	std::ofstream out("accounts.txt", std::ios::app);

	out << strName << " " << strPassword << std::endl;

	out.close();

	out.open(CHATS_PATH + strName + ".txt");
	out.close();
	
	return true;
}

bool loginAccount(std::string strInfo, std::vector<SUser>& vecUsers)
{
	std::string strName;
	std::string strPassword;
	getNameAndPassword(strInfo, strName, strPassword);

	bool bResult = false;
	for (int i = 0; i < vecUsers.size(); i++)
	{
		if (strName == vecUsers[i].m_strName)
		{
			if (strPassword == vecUsers[i].m_strPassword)
			{
				bResult = true;

				vecUsers[i].m_isUpToDate = false;
			}
			break;
		}
	}

	return bResult;
}

std::vector<char> searchUsers(std::string strInfo, const std::vector<SUser>& vecUsers)
{
	std::vector<char> vecSendBuf;
	for (int i = 0; i < vecUsers.size(); i++)
	{
		bool bIsFound = true;
		std::string strName(vecUsers[i].m_strName);
		for (int j = 0; j < strInfo.length() && j < strName.length(); j++)
		{
			if(strInfo[j] != strName[j])
			{
				bIsFound = false;
				break;
			}
		}
		if (bIsFound) // Знайдені імена користувачів записуємо через пробіл
		{
			vecSendBuf.insert(vecSendBuf.end(), strName.begin(), strName.end());
			vecSendBuf.push_back(' ');
		}
	}

	if (vecSendBuf.empty()) // Якщо нічого не знайдено
	{
		vecSendBuf.push_back(' ');
	}

	return vecSendBuf;
}

void sendMessage(std::string strInfo, std::vector<SUser>& vecUsers)
{
	std::string strSender;
	std::string strReceiver;
	std::string strMessage;
	for (int i = 0, infoPartNum = 1; strInfo[i] != '\0'; i++)
	{
		if (strInfo[i] == ' ' && infoPartNum != 3)
		{
			infoPartNum++;
		}
		else if (infoPartNum == 1) // Весь текст до першого пробілу
		{
			strSender.push_back(strInfo[i]);
		}
		else if (infoPartNum == 2) // Текст від першого пробулу до другого
		{
			strReceiver.push_back(strInfo[i]);
		}
		else if (infoPartNum == 3) // Текст від другого пробілу до кінця
		{
			strMessage.push_back(strInfo[i]);
		}
	}

	// У файл відправника додаємо нове повідомлення
	std::ofstream outSender(CHATS_PATH + strSender + "-" + strReceiver + ".txt", std::ios::app);
	outSender << strSender << ":" << strMessage << std::endl;
	outSender.close();
	
	// У файл одержувача додаємо нове повідомлення
	std::ofstream outReceiver(CHATS_PATH + strReceiver + "-" + strSender + ".txt", std::ios::app);
	outReceiver << strSender << ":" << strMessage << std::endl;
	outReceiver.close();


	bool bSenderFound = false;
	bool bReceiverFound = false;
	for (int i = 0; i < vecUsers.size(); i++)
	{
		if (vecUsers[i].m_strName == strSender)
		{
			vecUsers[i].m_isUpToDate = false;

			std::vector<std::string>& vecSenderChats(vecUsers[i].m_vecUserChats);

			// Якщо одержувач ще не в списку чатів відправника, додаємо його
			if (std::find(vecSenderChats.cbegin(), vecSenderChats.cend(), strReceiver) == vecSenderChats.cend())
			{
				vecSenderChats.push_back(strReceiver);

				outSender.open(CHATS_PATH + strSender + ".txt", std::ios::app);
				outSender << strReceiver << std::endl;
				outSender.close();
			}

			bSenderFound = true;
		}
		else if (vecUsers[i].m_strName == strReceiver)
		{
			vecUsers[i].m_isUpToDate = false;

			std::vector<std::string>& vecReceiverChats(vecUsers[i].m_vecUserChats);

			// Якщо відправник ще не в списку чатів одержувача, додаємо його
			if (std::find(vecReceiverChats.cbegin(), vecReceiverChats.cend(), strSender) == vecReceiverChats.cend())
			{
				vecReceiverChats.push_back(strSender);

				outReceiver.open(CHATS_PATH + strReceiver + ".txt", std::ios::app);
				outReceiver << strSender << std::endl;
				outReceiver.close();
			}

			bReceiverFound = true;
		}
		
		if (bSenderFound && bReceiverFound)
		{
			break;
		}
	}
}

std::vector<char> updateChats(std::string strName, std::vector<SUser>& vecUsers) 
{
	// Перевірка чи потрібно користувачу обновляти дані
	for (int i = 0; i < vecUsers.size(); i++)
	{
		if (vecUsers[i].m_strName == strName)
		{
			if (vecUsers[i].m_isUpToDate)
			{
				return { ' ' };
			}
			else
			{
				vecUsers[i].m_isUpToDate = true;
			}
			break;
		}
	}


	/*
	* --------------------------------------
	* Формат даних в буфері відпралення
	* <Ім'я співбесідника 1>
	* <Повідомлення 1>s
	* ...
	* <Повідомлення n>
	* 
	* <Ім'я співбесідника 2>
	* ...
	* <В кінці символ ' '>
	* --------------------------------------
	*/

	std::vector<char> vecSendBuf;

	std::ifstream in(CHATS_PATH + strName + ".txt");
	std::string strUserName;
	while (std::getline(in, strUserName))
	{
		// Вставляємо ім'я співбесідника в буфер відправлення
		vecSendBuf.insert(vecSendBuf.end(), strUserName.begin(), strUserName.end());
		vecSendBuf.push_back('\n');

		// Відкриття файлу з чатом
		std::ifstream inChat(CHATS_PATH + strName + "-" + strUserName + ".txt");

		std::string strChat;
		while (std::getline(inChat, strChat))
		{
			// Вставляємо одне повідомлення в буфер відправлення
			vecSendBuf.insert(vecSendBuf.end(), strChat.begin(), strChat.end());
			vecSendBuf.push_back('\n');
		}
		vecSendBuf.push_back('\n');

		inChat.close();
	}
	vecSendBuf.push_back(' ');
	
	in.close();

	return vecSendBuf;
}

DWORD WINAPI hahdleClientPipe(LPVOID lpParam)
{
	SHandleParam* pParam = reinterpret_cast<SHandleParam*>(lpParam);
	HANDLE hPipe = pParam->m_hPipe;
	std::vector<SUser>& vecUsers = pParam->m_vecUsers;

	std::vector<char> vecRecvBuf(BUF_SIZE);
	DWORD dwBytesRead, dwBytesWritten;

	while (true)
	{
		// Читання з пайпу
		BOOL bRead = ReadFile(hPipe, vecRecvBuf.data(), vecRecvBuf.size(), &dwBytesRead, NULL);
		if (bRead && dwBytesRead > 0)
		{
			// Визначення типу запиту (перші три літери)
			auto itEnd = std::find(vecRecvBuf.begin(), vecRecvBuf.end(), '\0'); // \0 є кінцем буфера
			std::string strTypeOfRequest(vecRecvBuf.begin(), vecRecvBuf.begin() + 3); // Тип запиту [0, 3)
			std::string strOtherInfo(vecRecvBuf.begin() + 4, itEnd); // Решта даних [4, itEnd)

			// Оброблення запиту
			std::vector<char> vecSendBuf;

			WaitForSingleObject(hMutex, INFINITE); // Блокування ресурсу
			if (strTypeOfRequest == "reg")
			{
				if (createAccount(strOtherInfo, vecUsers))
				{
					vecSendBuf = { 'Y' };
				}
				else
				{
					vecSendBuf = { 'N' };
				}
			}
			else if (strTypeOfRequest == "log")
			{
				if (loginAccount(strOtherInfo, vecUsers))
				{
					vecSendBuf = { 'Y' };
				}
				else
				{
					vecSendBuf = { 'N' };
				}
			}
			else if (strTypeOfRequest == "ser")
			{
				vecSendBuf = searchUsers(strOtherInfo, vecUsers);
			}
			else if (strTypeOfRequest == "mes")
			{
				sendMessage(strOtherInfo, vecUsers);
				vecSendBuf = { 'Y' };
			}
			else if (strTypeOfRequest == "upd")
			{
				vecSendBuf = updateChats(strOtherInfo, vecUsers);
			}
			ReleaseMutex(hMutex);  // Звільняємо ресурс

			// Відправлення відповіді через пайп
			BOOL bWrite = WriteFile(hPipe, vecSendBuf.data(), vecSendBuf.size(), &dwBytesWritten, NULL);
			if (!bWrite)
			{
				printf("WriteFile failed: %d\n", GetLastError());
			}
		}
		else if (dwBytesRead == 0)
		{
			printf("Client disconnected...\n");
			break;
		}
		else
		{
			printf("ReadFile failed: %d\n", GetLastError());
			break;
		}
	}

	// Закриття пайпу після завершення обробки
	CloseHandle(hPipe);
	delete pParam; // Видаляємо структуру параметрів
	return 0;
}

DWORD WINAPI mainPipe(LPVOID lpParam)
{
	// Отримуємо параметри з переданої структури
	SHandleParam* pParam = reinterpret_cast<SHandleParam*>(lpParam);
	std::vector<SUser>& vecUsers = pParam->m_vecUsers;

	// Ім'я іменованого пайпа
	const std::string pipeName = "\\\\.\\pipe\\ServerPipe";

	while (true)
	{
		// Створюємо пайп
		HANDLE hPipe = CreateNamedPipeA(
			pipeName.c_str(),
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			BUF_SIZE,
			BUF_SIZE,
			NMPWAIT_WAIT_FOREVER,
			NULL);

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			printf("CreateNamedPipe failed: %d\n", GetLastError());
			continue;
		}

		printf("Waiting for client to connect...\n");

		// Чекаємо на підключення клієнта
		BOOL bConnect = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (bConnect)
		{
			// Клієнт підключився, створюємо потік для обробки клієнта через пайп
			SHandleParam* pParam = new SHandleParam(0, hPipe, vecUsers);
			CreateThread(NULL, 0, hahdleClientPipe, pParam, 0, NULL);
		}
		else
		{
			CloseHandle(hPipe);  // Закриваємо пайп, якщо підключення не вдалося
			printf("ConnectNamedPipe failed: %d\n", GetLastError());
		}
	}

	return 0;
}
