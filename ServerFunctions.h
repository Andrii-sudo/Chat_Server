#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>

struct SUser
{
	std::string m_strName;
	std::string m_strPassword;
};

struct SHandleParam
{
	SOCKET m_ClientSocket;
	std::vector<SUser>& m_vecUsers;

	SHandleParam(SOCKET ClientSocket, std::vector<SUser>& vecUsers)
		: m_ClientSocket(ClientSocket), m_vecUsers(vecUsers)
	{ }
};

DWORD WINAPI handleClient(LPVOID lpParam);

void getNameAndPassword(std::string strInfo, std::string& strName, std::string& strPassword);

bool createAccount(std::string strInfo, std::vector<SUser>& vecUsers);

bool loginAccount(std::string strInfo, const std::vector<SUser>& vecUsers);

std::vector<char> searchUsers(std::string strInfo, const std::vector<SUser>& vecUsers);