#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <Windows.h>
#include <system_error>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define MAX_CONTENT_SIZE 2 * MAX_VALUE_NAME

using namespace std;

//iterate the registry keys
void RegistryKey(HKEY initialKey, string regKey, string basePath = "D:\\CSSORegistry\\") 
{
    HKEY hKey;

    //if the initial key can t be read => return
    if (RegOpenKeyExA(
        initialKey, 
        regKey.c_str(), 
        0, 
        KEY_READ, 
        &hKey) != ERROR_SUCCESS) 
    {
        return;
    }

    DWORD numberOfSubKeys, numberOfValues;

    //if the info can t be taken from the key (more specifically the number of subkeys and values)
    if (RegQueryInfoKeyA(
        hKey, 
        nullptr, 
        nullptr, 
        nullptr,
        &numberOfSubKeys,
        nullptr, 
        nullptr,
        &numberOfValues,
        nullptr, 
        nullptr,
        nullptr, 
        nullptr) != ERROR_SUCCESS) 
    {
        return;
    }

    //if there are subkeys for the given key
    if (numberOfSubKeys > 0)
    {
        cout << "Found " << numberOfSubKeys << " subkeys" << endl;

        for (DWORD i = 0; i < numberOfSubKeys; ++i)
        {
            char subkeyName[MAX_KEY_LENGTH];

            //create a new directory for each subkey
            if (RegEnumKeyA(
                hKey, 
                i, 
                subkeyName, 
                MAX_KEY_LENGTH) == ERROR_SUCCESS) 
            {
                string newDirectoryName = basePath + "\\" + subkeyName;
                CreateDirectoryA(newDirectoryName.c_str(), NULL);
                RegistryKey(hKey, subkeyName, newDirectoryName);
            }
        }
    }

    //if there are values for the given key
    if (numberOfValues > 0)
    {
        cout << "Found " << numberOfValues << " values" << endl;

        for (DWORD i = 0; i < numberOfValues; ++i)
        {
            char valueName[MAX_VALUE_NAME];
            DWORD valueNameSize = MAX_VALUE_NAME;

            //create a file with its content
            if (RegEnumValueA(
                hKey, 
                i, 
                valueName, 
                &valueNameSize, 
                NULL, 
                NULL, 
                NULL, 
                NULL) == ERROR_SUCCESS) 
            {
                vector<unsigned char> content;
                content.resize(MAX_CONTENT_SIZE);
                DWORD maxContentSize = MAX_CONTENT_SIZE;

                if (RegQueryValueExA(
                    hKey, 
                    valueName, 
                    NULL, 
                    NULL, 
                    content.data(), 
                    &maxContentSize) == ERROR_SUCCESS) 
                {

                    const string& path = basePath + "\\" + valueName;

                    HANDLE hFile = CreateFileA(
                        path.c_str(),
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

                    //invalid file
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        return;
                    }

                    DWORD numberOfBytesWritten = 0;

                    //write in file
                    if (WriteFile(
                        hFile,
                        content.data(),
                        content.size(),
                        &numberOfBytesWritten,
                        NULL) != ERROR_SUCCESS)
                    {
                        return;
                    }

                    //close file
                    CloseHandle(hFile);
                }
            }
        }
    }

    //close key
    RegCloseKey(hKey);
}

int main(unsigned int argc, char* argv[]) 
{
    RegistryKey(HKEY_CURRENT_USER, "Console");
           
    return 0;
}
