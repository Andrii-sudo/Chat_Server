#include "ServerFunctions.h"

DWORD WINAPI handleClient(LPVOID lpParam)
{
	SHandleParam* pParam = static_cast<SHandleParam*>(lpParam);

	SOCKET ClientSocket = pParam->m_ClientSocket;
	std::vector<SUser>& vecUsers = pParam->m_vecUsers;

	std::vector<char> vecRecvBuf(1024);
	int iResult, iSendResult;

	iResult = recv(ClientSocket, vecRecvBuf.data(), vecRecvBuf.size(), 0);
	if (iResult > 0)
	{
		std::string strTypeOfRequest;
		std::string strOtherInfo;
		for (int i = 0, isTypeOfRequest = 1; vecRecvBuf[i] != '\0'; i++)
		{
			if (isTypeOfRequest == 1 && vecRecvBuf[i] == ' ')
			{
				isTypeOfRequest = 0;
			}
			else if (isTypeOfRequest)
			{
				strTypeOfRequest.push_back(vecRecvBuf[i]);
			}
			else
			{
				strOtherInfo.push_back(vecRecvBuf[i]);
			}
		}

		std::vector<char> vecSendBuf;
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
	for (int i = 0, isName = 1; strInfo[i] != '\0'; i++)
	{
		if (strInfo[i] == ' ')
		{
			isName = 0;
		}
		else if (isName)
		{
			strName.push_back(strInfo[i]);
		}
		else
		{
			strPassword.push_back(strInfo[i]);
		}
	}
}

bool createAccount(std::string strInfo, std::vector<SUser>& vecUsers)
{
	std::string strName;
	std::string strPassword;
	getNameAndPassword(strInfo, strName, strPassword);

	SUser userNew;
	userNew.m_strName = strName;
	userNew.m_strPassword = strPassword;

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

bool loginAccount(std::string strInfo, const std::vector<SUser>& vecUsers)
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
		if (bIsFound)
		{
			for (int j = 0; j < strName.size(); j++)
			{
				vecSendBuf.push_back(strName[j]);
			}
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
		else if (infoPartNum == 1)
		{
			strSender.push_back(strInfo[i]);
		}
		else if (infoPartNum == 2)
		{
			strReceiver.push_back(strInfo[i]);
		}
		else if (infoPartNum == 3)
		{
			strMessage.push_back(strInfo[i]);
		}
	}

	std::ofstream outSender(CHATS_PATH + strSender + "-" + strReceiver + ".txt", std::ios::app);
	outSender << strSender << ":" << strMessage << std::endl;
	outSender.close();
	
	std::ofstream outReceiver(CHATS_PATH + strReceiver + "-" + strSender + ".txt", std::ios::app);
	outReceiver << strSender << ":" << strMessage << std::endl;
	outReceiver.close();

	bool bSenderFound = false;
	bool bReceiverFound = false;
	for (int i = 0; i < vecUsers.size(); i++)
	{
		if (vecUsers[i].m_strName == strSender)
		{
			std::vector<std::string>& vecSenderChats(vecUsers[i].m_vecUserChats);
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
			std::vector<std::string>& vecReceiverChats(vecUsers[i].m_vecUserChats);
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