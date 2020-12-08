#include "pch.h"
#include <iostream>
#include <Windows.h>
#include <wininet.h>
#include <sstream>
#include <vector>
#include <string>

#pragma comment(lib, "wininet.lib")

#define LINK L"https://raw.githubusercontent.com/egg-nation/CSSO/main/info.txt"
#define FTP_SERVER "127.0.0.1"

using namespace std;

string clear_string(string str)
{
	while (str.size() && (str.back() == '\r' || str.back() == '\t' || str.back() == '\n'))
	{
		str.pop_back();
	}
	return str;
}

//https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
vector<string> parse(string str, char delimitators = ' ')
{
	string token;
	vector<string> parsedTokens;
	stringstream *current = new stringstream(str);

	while (getline(*current, token, delimitators))
	{
		parsedTokens.push_back(token);
	}

	return parsedTokens;
}

int commands(vector<string> contentLines, string link, string username, string password)
{
	HINTERNET internetHandle;
	HINTERNET ftpSession;

	internetHandle = InternetOpen(
		NULL,
		INTERNET_OPEN_TYPE_DIRECT, 
		NULL,
		NULL, 
		0);

	if (internetHandle)
	{
		ftpSession = InternetConnectA(
			internetHandle,
			FTP_SERVER, 
			INTERNET_DEFAULT_FTP_PORT, 
			username.c_str(),
			password.c_str(), 
			INTERNET_SERVICE_FTP,
			0, 
			0);

		if (ftpSession)
		{

		}
		else
		{
			cout << "InternetConnectA failed. Last error = " << GetLastError() << endl;
			return 0;
		}
	}
	else
	{
		cout << "InternetOpen failed. Last error = " << GetLastError() << endl;
		return 0;
	}

	for (auto line : contentLines)
	{
		auto tokens = parse(line);
		string command = tokens[0];
		string parameter = tokens[1];

		if (command == "PUT")
		{
			auto vec = parse(parameter, '\\');
			string ftpPath = vec[vec.size() - 1];

			if (!FtpPutFileA(
				ftpSession,
				parameter.c_str(),
				ftpPath.c_str(),
				FTP_TRANSFER_TYPE_BINARY,
				0))
			{
				cout << "FtpPutFileA failed. Last error = " << GetLastError() << endl;
				return 0;
			}
		}
		else
		{
			if (!FtpGetFileA(
				ftpSession,
				parameter.c_str(),
				parameter.c_str(),
				false,
				0,
				0,
				0))
			{
				cout << "FtpGetFileA failed. Last error = " << GetLastError() << endl;
				return 0;
			}

			PROCESS_INFORMATION newProcess;
			STARTUPINFOA startupInfo = { 0 };
			startupInfo.cb = sizeof(startupInfo);

			if (!CreateProcessA(
				parameter.c_str(),
				NULL,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&startupInfo,
				&newProcess))
			{
				cout << "Unable to create a new process. Last error = " << GetLastError() << endl;
				return 0;
			}

			CloseHandle(newProcess.hProcess);
			CloseHandle(newProcess.hThread);
		}
	}

	InternetCloseHandle(ftpSession);
	InternetCloseHandle(internetHandle);
	return 1;
}

string download_file_from_http(LPCWSTR link)
{
	HINTERNET internetHandle;
	HINTERNET fileHandle;

	char buffer[1024];
	DWORD bytesRead;
	unsigned int flag = 0, size;

	internetHandle = InternetOpen(
		L"",
		INTERNET_OPEN_TYPE_DIRECT,
		NULL,
		NULL,
		0);

	if (internetHandle)
	{

	}
	else
	{
		cout << "InternetOpen failed. Last error = " << GetLastError() << endl;
		return 0;
	}

	fileHandle = InternetOpenUrl(
		internetHandle,
		link,
		NULL,
		0L,
		0,
		0);

	if (fileHandle)
	{

	}
	else
	{
		cout << "InternetOpenURL failed. Last error = " << GetLastError() << endl;
		return 0;
	}

	while (!flag)
	{
		if (InternetReadFile(
			fileHandle,
			buffer,
			sizeof(buffer),
			&bytesRead))
		{
			if (!bytesRead)
			{
				flag = 1;
				buffer[size] = '\0';
			}
			size = bytesRead;
		}
		else
		{
			cout << "InternetReadFile failed. Last error = " << GetLastError() << endl;
			flag = 1;
		}
	}

	InternetCloseHandle(internetHandle);
	InternetCloseHandle(fileHandle);

	return string(buffer);
}

int main()
{
	unsigned int numberCommands;

	string username;
	string password;

	string link;

	string content = download_file_from_http(LINK);

	auto contentLines = parse(content, '\n');

	for (auto& line : contentLines) 
	{
		line = clear_string(line);
	}

	numberCommands = stoi(contentLines.at(0));

	link = contentLines.at(1);

	username = contentLines.at(2);
	password = contentLines.at(3);

	contentLines.erase(contentLines.begin(), contentLines.begin() + 4);

	commands(contentLines, link, username, password);

	return 0;
}