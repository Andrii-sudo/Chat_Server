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

		iSendResult = send(ClientSocket, vecSendBuf.data(), vecRecvBuf.size(), 0);
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