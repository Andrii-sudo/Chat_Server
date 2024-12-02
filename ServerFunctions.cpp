#include "ServerFunctions.h"

HANDLE hMutex;

DWORD WINAPI handleClient(LPVOID lpParam)
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