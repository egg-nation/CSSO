#include <iostream>
#include <Windows.h>
#include <random>
#include <cstdlib>
#include <utility>
#include <tuple>
#include <memory>
#include <optional>

using namespace std;


struct Data
{
    unsigned int value;
    unsigned int doubleValue;
};

template <typename Object, typename Deleter>
struct WindowsPtr
{
private:
    optional<Object> objectData;
    Deleter deleter;
    Object invalidObject;

    int* refCount = new int(0);

public:
    WindowsPtr() = default;
    WindowsPtr(
        const Object& obj,
        Deleter deleter,
        const Object& invalidObject) :
        objectData(obj), deleter(deleter), invalidObject(invalidObject) {
        *refCount = 1;

    }
    WindowsPtr(const WindowsPtr<Object, Deleter>& rhs) {
        this->refCount = rhs.refCount;
        (*refCount)++;
        objectData = rhs.objectData;
        deleter = rhs.deleter;
        invalidObject = rhs.invalidObject;
    };

    WindowsPtr(const WindowsPtr<Object, Deleter>&&) = delete;

    ~WindowsPtr()
    {
        if (objectData.has_value() && *refCount == 1)
        {
            deleter(objectData.value());
            objectData = invalidObject;
            delete refCount;
        }
        *refCount -= 1;
    }

    WindowsPtr<Object, Deleter>& operator = (const Object& rhs)
    {
        this->objectData = rhs;
        return *this;
    }

    Object operator*() const
    {
        return objectData.has_value() ? objectData.value() : invalidObject;
    }

};

#define HANDLE_PTR_OBJECT WindowsPtr<HANDLE, BOOL(__stdcall*)(HANDLE)>
#define HANDLE_PTR_VIEW WindowsPtr<void*, BOOL(__stdcall*)(LPCVOID)>


tuple<bool, HANDLE_PTR_OBJECT, HANDLE_PTR_VIEW>
create_file_mapping_handle_and_view()
{
    constexpr const unsigned int fileMappingSize = 1024 * 1024;

    HANDLE fileMappingHandle = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL, PAGE_READWRITE,
        0,
        fileMappingSize,
        "MultithreadedChecker");

    if (fileMappingHandle == NULL)
    {
        cout << "CreateFileMapping failed. Error code: " << GetLastError() << endl;
        return {
            false,
            {},
            {} };
    }

    void* fileMapping = MapViewOfFile(
        fileMappingHandle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0);

    if (fileMapping == NULL)
    {
        CloseHandle(fileMappingHandle);
        cout << "MapViewOfFile failed. Error code: " << GetLastError() << endl;
        return {
            false,
            {},
            {} };
    }

    return {
        true,
        {fileMappingHandle, CloseHandle, INVALID_HANDLE_VALUE},
        {fileMapping, UnmapViewOfFile, nullptr}
    };
}

int main(int argc, const char* argv[])
{
    unsigned int steps = 420;

    if (argc != 3)
    {
        cout << "Invalid number of arguments given" << endl;
        return 0;
    }

    auto fileMappingHandleView = create_file_mapping_handle_and_view();

    if (!get<0>(fileMappingHandleView))
    {
        return 0;
    }

    auto fileMappingHandle = get<1>(fileMappingHandleView);
    auto fileMappingView = get<2>(fileMappingHandleView);

    random_device dev;
    ranlux24 rng(dev());
    uniform_int_distribution<unsigned int> randomValueGenerator(1, 1000);

    if (string(argv[2]) == "mutex")
    {

        HANDLE mutexValue;
        mutexValue = CreateMutex(
            NULL,
            FALSE,
            "SynchronisedMutex");

        if (!mutexValue)
        {
            cout << "[Process 1] Create mutex failed. Last error = " << GetLastError() << endl;
            return 0;
        }
        HANDLE_PTR_OBJECT mutex(
            mutexValue,
            CloseHandle,
            INVALID_HANDLE_VALUE);

        HANDLE_PTR_OBJECT processThread, processHandle;

        PROCESS_INFORMATION newProcess;
        STARTUPINFOA startupInfo = { 0 };
        startupInfo.cb = sizeof(startupInfo);

        string commandLine = argv[1];
        commandLine.append(" mutex");

        BOOL processCreateResult = CreateProcessA(
            NULL,
            (LPSTR)commandLine.c_str(),
            nullptr,
            nullptr,
            true,
            0,
            nullptr,
            (LPSTR)"./",
            &startupInfo,
            &newProcess);

        if (!processCreateResult)
        {
            cout << "Unable to create a new process. Last error = " << GetLastError() << endl;
            return 0;
        }

        processThread = newProcess.hThread;
        processHandle = newProcess.hProcess;

        for (unsigned int i = 0; i < steps; ++i)
        {
            WaitForSingleObject(
                *mutex,
                INFINITE);

            unsigned int randomValue = randomValueGenerator(rng);

            Data sendData{ randomValue, 2 * randomValue };

            void* viewAddress = *fileMappingView;

            CopyMemory(
                viewAddress,
                &i,
                sizeof(unsigned int));
            CopyMemory(
                (unsigned char*)viewAddress + sizeof(int) + i * sizeof(Data),
                &sendData,
                sizeof(sendData));

            cout << "[Process 1] Written " << sendData.value << ' ' << sendData.doubleValue << endl;

            ReleaseMutex(*mutex);

        }

        WaitForSingleObject(
            *processHandle,
            INFINITE);

    }
    else if (string(argv[2]) == "event")
    {
        HANDLE_PTR_OBJECT writeEvent, readEvent, processThread, processHandle;
        {
            HANDLE writeEventHandle = CreateEvent(
                NULL,
                TRUE,
                FALSE,
                "WriteEvent");

            if (writeEventHandle == NULL)
            {
                cout << "CreateEvent(WriteEvent) failed. Last error = " << GetLastError() << endl;
                return 1;
            }

            HANDLE readEventHandle = CreateEvent(
                NULL,
                TRUE,
                TRUE,
                "ReadEvent");

            if (readEventHandle == NULL)
            {
                cout << "CreateEvent(ReadEvent) failed. Last error = " << GetLastError() << endl;
                return 1;
            }

            STARTUPINFOA startupInfo = { 0 };
            PROCESS_INFORMATION newProcess;
            startupInfo.cb = sizeof(startupInfo);

            string commandLine = argv[1];
            commandLine.append(" event");

            BOOL processCreateResult = CreateProcessA(
                NULL,
                (LPSTR)commandLine.c_str(),
                nullptr,
                nullptr,
                true,
                0,
                nullptr,
                (LPSTR)"./",
                &startupInfo,
                &newProcess);

            if (!processCreateResult)
            {
                cout << "Creating a new process failed. Last error = " << GetLastError() << endl;
                return 0;
            }

            writeEvent = writeEventHandle;
            readEvent = readEventHandle;
            processThread = newProcess.hThread;
            processHandle = newProcess.hProcess;
        }

        for (unsigned int i = 0; i < steps; ++i)
        {
            WaitForSingleObject(
                *readEvent,
                INFINITE);

            unsigned int randomValue = randomValueGenerator(rng);
            Data sendData{ randomValue, 2 * randomValue };
            void* viewAddress = *fileMappingView;

            CopyMemory(
                viewAddress,
                &sendData,
                sizeof(sendData));

            cout << "[Process 1] Written " << sendData.value << ' ' << sendData.doubleValue << endl;

            ResetEvent(*readEvent);
            SetEvent(*writeEvent);
        }

        WaitForSingleObject(
            *processHandle,
            INFINITE);
    }
}
