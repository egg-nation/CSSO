#include <windows.h>
#include <iostream>
#include <fstream>
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

	WindowsPtr<Object, Deleter>& operator = (const Object& rhs) {
		this->objectData = rhs;
		return *this;
	}

	Object operator*() const {
		return objectData.has_value() ? objectData.value() : invalidObject;
	}

};

#define HANDLE_PTR_OBJECT WindowsPtr<HANDLE, BOOL(__stdcall*)(HANDLE)>
#define HANDLE_PTR_VIEW WindowsPtr<void*, BOOL(__stdcall*)(LPCVOID)>

tuple<bool, HANDLE_PTR_OBJECT, HANDLE_PTR_VIEW>
open_file_mapping_handle_and_view()
{
	constexpr const unsigned int fileMappingSize = 1024 * 1024;

	HANDLE fileMappingHandle = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
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
    if (argc != 2) 
	{
        cout << "Invalid number of arguments given" << endl;
        return 0;
    }

    constexpr const unsigned int steps = 420;


	auto fileMappingHandleView = open_file_mapping_handle_and_view();

	if (!get<0>(fileMappingHandleView))
	{
		return 0;
	}

	auto fileMappingHandle = get<1>(fileMappingHandleView);
	auto fileMappingView = get<2>(fileMappingHandleView);

	if (string(argv[1]) == "mutex") 
	{
		HANDLE mutexValue = OpenMutex(
			SYNCHRONIZE,
			FALSE,
			"SynchronisedMutex");

		if (!mutexValue)
		{
			CloseHandle(mutexValue);
			cout << "[Process 2] OpenMutex failed. Error code " << GetLastError() << endl;
			return 0;
		}


		HANDLE_PTR_OBJECT mutex(
			mutexValue,
			CloseHandle,
			INVALID_HANDLE_VALUE);

		int i = 0, index;
		while (i < steps) 
		{
			WaitForSingleObject(*mutex, INFINITE);

			void* buffer = *fileMappingView;

			CopyMemory(
				&index,
				buffer, 
				sizeof(int));

			if (index >= i) 
			{
				Data values;
				CopyMemory(
					&values, 
					(unsigned char*)buffer + sizeof(int) + index * sizeof(values), 
					sizeof(values));

				if (values.value * 2 != values.doubleValue) 
				{
					cout << "[Process 2] INCORRECT " << values.value << ' ' << values.doubleValue << endl;
				} 
				else 
				{
					cout << "[Process 2] CORRECT!\n";
					cout.flush();
				}

				i += 1;
			}

			ReleaseMutex(*mutex);
		}
	} 
	else if (string(argv[1]) == "event") 
	{
		HANDLE_PTR_OBJECT writeEvent, readEvent;
		{
			HANDLE writeEventHandle = OpenEvent(
				EVENT_MODIFY_STATE | SYNCHRONIZE,
				0,
				"WriteEvent");

			if (writeEventHandle == NULL)
			{
				cout << "CreateEvent(WriteEvent) failed. Last error = " << GetLastError() << endl;
				return 1;
			}

			HANDLE readEventHandle = OpenEvent(
				EVENT_MODIFY_STATE | SYNCHRONIZE,
				0,
				"ReadEvent");

			if (readEventHandle == NULL)
			{
				cout << "CreateEvent(ReadEvent) failed. Last error = " << GetLastError() << endl;
				return 1;
			}

			writeEvent = writeEventHandle;
			readEvent = readEventHandle;
		}

		for (int i = 0; i < steps; ++i) 
		{
			WaitForSingleObject(*writeEvent, INFINITE);
			Data values;
			void* buffer = *fileMappingView;
			CopyMemory(
				&values, 
				buffer, 
				sizeof(Data));

            if (values.value * 2 != values.doubleValue) 
			{
				cout << "[Process 2] INCORRECT " << values.value << ' ' << values.doubleValue << endl;
			} 
			else 
			{
				cout << "[Process 2] CORRECT!\n";
				cout.flush();
			}

			ResetEvent(*writeEvent);
			SetEvent(*readEvent);
		}
	}
}
