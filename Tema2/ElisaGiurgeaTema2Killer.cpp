#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <memory>
#include <vector>

using namespace std;

struct Tree;

struct Process 
{
	int PID;
	int PPID;
	wstring processName;
};

map<int, Process> pidToProcess;
map<int, vector<int>> pidToChildren;
vector<int> roots;

void set_token_privileges() 
{
    HANDLE tokenHandle;
	TOKEN_PRIVILEGES tokenPriviledges;

	if (OpenProcessToken(
		GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES,
		&tokenHandle))
	{
		LUID lpLuid;

		if (LookupPrivilegeValue(
			NULL,
			SE_DEBUG_NAME,
			&lpLuid))
		{
			tokenPriviledges.PrivilegeCount = 1;
			tokenPriviledges.Privileges[0].Luid = lpLuid;
			tokenPriviledges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if (!AdjustTokenPrivileges(
				tokenHandle,
				FALSE,
				&tokenPriviledges,
				sizeof(TOKEN_PRIVILEGES),
				NULL,
				NULL))
			{
				cout << "AdjustTokenPrivileges failed." << "\n";
				CloseHandle(tokenHandle);
				return;
			}
		}
		else
		{
			cout << "LookupPrivilegeValue failed." << "\n";
			CloseHandle(tokenHandle);
			return;
		}
	}
	else
	{
		cout << "OpenProcessToken failed." << "\n";
		return;
	}
}

void read_processes() 
{
    constexpr auto ln = 1024 * 1024;
    HANDLE handleMapFile = OpenFileMapping(
							FILE_MAP_ALL_ACCESS,
							FALSE, 
							L"ProcessTree");

	if (handleMapFile)
	{

	}
	else
	{
        cout << "OpenFileMapping failed." << "\n";
		return;
	}

	wchar_t* buffer = (wchar_t*)MapViewOfFile(handleMapFile, 
											FILE_MAP_ALL_ACCESS, 
											0, 
											0, 
											ln);

	if (buffer == NULL)
	{
		cout << "MapViewOfFile failed." << "\n";
		CloseHandle(handleMapFile);
		return;
	}

	wistringstream inputStream(buffer);
    wstring line;

	while (getline(inputStream, line)) 
	{
        wistringstream lineStream(line);

		Process currentProcess{};
		int index;
		lineStream >> index >> currentProcess.PID >> currentProcess.PPID;
		getline(lineStream, currentProcess.processName);

		wcout << L"Read process: Name = " << currentProcess.processName << 
			L"; PID = " << currentProcess.PID << 
			L"; PPID = " << currentProcess.PPID << "\n";

		//insert
		pidToProcess[currentProcess.PID] = currentProcess;

		if (currentProcess.PID == currentProcess.PPID && currentProcess.PID == 0)
		{

		}
		else
		{
			pidToChildren[currentProcess.PPID].push_back(currentProcess.PID);
		}
	}

    CloseHandle(handleMapFile);
	UnmapViewOfFile(buffer);
}

void print_process_tree(int PID, int depth = 0) 
{

	for (int i = 0; i < depth; ++i) 
	{
		cout << "----";
	}

	if (pidToProcess.find(PID) != pidToProcess.end()) 
	{
		Process currentParent = pidToProcess[PID];
		wcout << "Process name = " << currentParent.processName << 
			"; Process ID = " << currentParent.PID << 
			"; Parent Process ID = " << currentParent.PPID << "\n";
	}
	else 
	{
        wcout << L"Unnamed process with PID = " << PID << "\n";
	}

	auto currentChildren = pidToChildren[PID];

	for (auto childPid : currentChildren)
	{
		print_process_tree(childPid, depth + 1);
	}
}

void kill_subtree(int PID, int depth = 0) 
{
    for (int i = 0; i < depth; ++i) 
	{
		cout << "----";
	}

	if (pidToProcess.find(PID) != pidToProcess.end()) 
	{
		Process currentParent = pidToProcess[PID];
		wcout << L"Killing process: " << currentParent.processName << 
			"; Process ID = " << currentParent.PID << 
			"; Parent Process ID = " << currentParent.PPID << "\n";
	} 
	else 
	{
        wcout << L"Killing unnamed process with PID = " << PID << "\n";
	}

	auto currentChildren = pidToChildren[PID];

	for (auto childPid : currentChildren) 
	{
		kill_subtree(childPid, depth + 1);
	}

    HANDLE processToKill = OpenProcess(PROCESS_ALL_ACCESS, 
										false, 
										PID);

	if (processToKill == NULL) 
	{
		wcout << L"Unable to open process with PID = " << PID << "." << "\n";
	} 
	else 
	{
		if (TerminateProcess(processToKill, 0) == 0) 
		{
            wcout << L"Unable to kill process with PID = " << PID << "." << "\n";
		}
	}
}

void initialize_roots() 
{

	bool visited[100000] = {};
	memset(visited, 0, sizeof(visited));

	for (auto it : pidToProcess) 
	{
		if (visited[it.first])
		{
			continue;
		}

		bool suitableRoot = true;
		int currentPid = it.second.PID;
		int parentPid = it.second.PPID;

		while (true) 
		{
			currentPid = parentPid;
			visited[currentPid] = true;

			if (currentPid == 0)
			{
				break;
			}
			
			if (pidToProcess.find(currentPid) != pidToProcess.end()) 
			{
				parentPid = pidToProcess[currentPid].PPID;
			}
			else 
			{
				break;
			}

			if (visited[parentPid]) 
			{
				suitableRoot = false;
				break;
			}

		}

		if (suitableRoot) 
		{
			roots.push_back(currentPid);
			wcout << L"Found new root " << currentPid << L"\n";
		}
	}
}

int main() 
{
	system("PAUSE");
	set_token_privileges();
	read_processes();
	initialize_roots();

	//print process trees
	int treeId = 0;

	for (auto root : roots)
	{
		cout << "Tree ID: " << treeId++ << "\n";
		print_process_tree(root);
		cout << "\n";
	}

	int killTree;
	wcout << L"What tree do you want to kill?" << "\n";
	cin >> killTree;

	treeId = 0;

	for (auto root : roots)
	{
		if (treeId++ == killTree)
		{
			cout << "Killing tree with root PID = " << root << "\n";
			kill_subtree(root);
		}
	}

}