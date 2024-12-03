#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>

#define CHATS_PATH ".\\User chats\\"
#define DEFAULT_PORT "12345"
#define BUF_SIZE 2048

struct SUser
{
	std::string m_strName;
	std::string m_strPassword;
	std::vector<std::string> m_vecUserChats;

	bool m_isUpToDate;
};

struct SHandleParam
{
	SOCKET m_ClientSocket;
	HANDLE m_hPipe;
	std::vector<SUser>& m_vecUsers;

	SHandleParam(SOCKET ClientSocket, HANDLE hPipe, std::vector<SUser>& vecUsers)
        : m_ClientSocket(ClientSocket), m_hPipe(hPipe), m_vecUsers(vecUsers)
    { }

};

DWORD WINAPI handleClientSocket(LPVOID lpParam);

void getNameAndPassword(std::string strInfo, std::string& strName, std::string& strPassword);

bool createAccount(std::string strInfo, std::vector<SUser>& vecUsers);

bool loginAccount(std::string strInfo, std::vector<SUser>& vecUsers);

std::vector<char> searchUsers(std::string strInfo, const std::vector<SUser>& vecUsers);

void sendMessage(std::string strInfo, std::vector<SUser>& vecUsers);

std::vector<char> updateChats(std::string strName, std::vector<SUser>& vecUsers);
;
DWORD WINAPI hahdleClientPipe(LPVOID lpParam);

DWORD WINAPI mainPipe(LPVOID lpParam);