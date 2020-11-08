#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <sstream>

using namespace std;

int main() 
{
    constexpr auto ln = 1024 * 1024;
    HANDLE fileMapping = CreateFileMapping(
                            INVALID_HANDLE_VALUE,
                            NULL, 
                            PAGE_READWRITE,
                            0, 
                            ln, 
                            L"ProcessTree");

    if (fileMapping == NULL) 
    {
        cout << "Unable to create file mapping." << "\n";
        return 0;
    }

    wchar_t* mappedFile = (wchar_t*) MapViewOfFile(
                                        fileMapping, 
                                        FILE_MAP_ALL_ACCESS, 
                                        0, 
                                        0, 
                                        0);

    if (mappedFile == NULL) 
    {
        CloseHandle(fileMapping);
        cout << "Unable to get map view of file." << "\n";

        return 0;
    }

    HANDLE snapshotProcesses = CreateToolhelp32Snapshot(
                                TH32CS_SNAPPROCESS,
                                0);

    if (snapshotProcesses == INVALID_HANDLE_VALUE) 
    {
        UnmapViewOfFile(mappedFile);
        CloseHandle(fileMapping);
        cout << "Unable to get map view of file." << "\n";

        return 0;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(
        snapshotProcesses, 
        &processEntry))
	{
        cout << "Process32First failed." << "\n";
		CloseHandle(snapshotProcesses);
		CloseHandle(fileMapping);
		UnmapViewOfFile(mappedFile);

		return 0;
	}

    int index = 0;

	do
	{
        wstringstream line;
        line << index++ << " " << 
            processEntry.th32ProcessID << " " << 
            processEntry.th32ParentProcessID << " " << 
            processEntry.szExeFile << "\n";
        
        wcscat(mappedFile, line.str().c_str());

	} while (Process32Next(
            snapshotProcesses, 
            &processEntry));

    system("PAUSE");

    CloseHandle(snapshotProcesses);
    UnmapViewOfFile(mappedFile);
    CloseHandle(fileMapping);

    return 0;
}

