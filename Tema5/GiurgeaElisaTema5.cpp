#include <Windows.h>
#include <iostream>
#include <aclapi.h>

using namespace std;

#define SIZE 68
#define _CRT_SECURE_NO_WARNINGS

HKEY hKey;
const char* registryPath = "SOFTWARE\\CSSO\\Tema5_try";

DWORD _RegCreateKeyEx(SECURITY_ATTRIBUTES lpSecurityAttributes) 
{
    DWORD dispositionKey = 0;

    return RegCreateKeyEx(
        HKEY_CURRENT_USER,//HKEY_LOCAL_MACHINE
        registryPath,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_READ,
        &lpSecurityAttributes,
        &hKey,
        &dispositionKey);
}

int main()
{
    BYTE sidSize[SIZE];

    PSID pAdminSid;
    PSID pEveryoneSid = NULL;

    SID_IDENTIFIER_AUTHORITY creatorSAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY worldSAuthority = SECURITY_WORLD_SID_AUTHORITY;

    if (AllocateAndInitializeSid(
        &creatorSAuthority, 
        1,
        SECURITY_CREATOR_GROUP_RID,
        0, 
        0, 
        0, 
        0, 
        0, 
        0, 
        0,
        &pAdminSid))
    {

    }
    else
    {
        cout << "AllocateAndInitializeSid failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    if (AllocateAndInitializeSid(
        &worldSAuthority,
        1,
        SECURITY_WORLD_RID,
        0, 
        0, 
        0, 
        0, 
        0, 
        0, 
        0,
        &pEveryoneSid))
    {

    }
    else
    {
        cout << "AllocateAndInitializeSid failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    EXPLICIT_ACCESS accessParams[2];

    ZeroMemory(&accessParams, 2 * sizeof(EXPLICIT_ACCESS));

    accessParams[0].grfAccessPermissions = KEY_ALL_ACCESS;
    accessParams[0].grfAccessMode = SET_ACCESS;
    accessParams[0].grfInheritance = NO_INHERITANCE;

    accessParams[0].Trustee.ptstrName = (LPTSTR)pAdminSid;
    accessParams[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    accessParams[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;

    accessParams[1].grfAccessPermissions = KEY_READ;
    accessParams[1].grfAccessMode = SET_ACCESS;
    accessParams[1].grfInheritance = NO_INHERITANCE;

    accessParams[1].Trustee.ptstrName = (LPTSTR)pEveryoneSid;
    accessParams[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    accessParams[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;

    PACL pACL = NULL;

    if (SetEntriesInAcl(
        2, 
        accessParams,
        NULL, 
        &pACL) == ERROR_SUCCESS)
    {

    }
    else
    {
        cout << "SetEntriesInAcl failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    PSECURITY_DESCRIPTOR pSDescriptor = NULL;

    pSDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(
        LPTR, 
        SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (InitializeSecurityDescriptor(
        pSDescriptor,
        SECURITY_DESCRIPTOR_REVISION))
    {

    }
    else
    {
        cout << "InitializeSecurityDescriptor failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    if (SetSecurityDescriptorGroup(
        pSDescriptor,
        pAdminSid,
        FALSE))
    {

    }
    else
    {
        cout << "SetSecurityDescriptorGroup failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    if (SetSecurityDescriptorDacl(
        pSDescriptor,
        TRUE,
        pACL,
        FALSE))
    {

    }
    else
    {
        cout << "SetSecurityDescriptorDacl failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    SECURITY_ATTRIBUTES sAttributes;

    sAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    sAttributes.lpSecurityDescriptor = pSDescriptor;
    sAttributes.bInheritHandle = FALSE;

    if (_RegCreateKeyEx(sAttributes) == ERROR_SUCCESS)
    {

    }
    else
    {
        cout << "_RegCreateKeyEx failed. Last error = " << GetLastError() << endl;
        return 0;
    }

    FreeSid(pAdminSid);
    FreeSid(pEveryoneSid);
    LocalFree(pACL);
    LocalFree(pSDescriptor);
    RegCloseKey(hKey);

    return 0;
}
