#include <string>
#include <iostream>
#include <Windows.h>
#include "../WLink/Wlink.h"

using namespace std;

string injectDllPath;
HANDLE hProcess = nullptr;
HANDLE hThread = nullptr;
HANDLE hMapFile = nullptr;
LPVOID lpMemFile = nullptr;

void HandleCommand() {
	while (true) {
		//cout << '>';
		string command;
		cin >> command;
		if (command == "quit") {
			return;
		}
	}
}

bool CreateProcess(const LPSTR lpCommandLine) {
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	//pi:创建线程返回的信息
	PROCESS_INFORMATION pi = { 0 };
	const BOOL bRet = CreateProcess(NULL, lpCommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	if (!bRet) {
		cout << "CreateProcess Failed\n";
		return false;
	}

	hProcess = pi.hProcess;
	hThread = pi.hThread;
}

bool InjectDll() {
	const auto pDllPath = VirtualAllocEx(hProcess, NULL, injectDllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
	if (pDllPath == nullptr) {
		cout << "VirtualAllocEx Failed\n";
		return false;
	}
	WriteProcessMemory(hProcess, pDllPath, injectDllPath.c_str(), injectDllPath.size() + 1, NULL);
	const auto pLoadLibrary = GetProcAddress(GetModuleHandle("kernel32"), "LoadLibraryA");
	if (pLoadLibrary == nullptr) {
		cout << "LoadLibraryA Failed\n";
		return false;
	}
	auto pLib = reinterpret_cast<PTHREAD_START_ROUTINE>(pLoadLibrary);
	HANDLE hNewThread = CreateRemoteThread(hProcess, NULL, 0, pLib, pDllPath, 0, NULL);
	if (hNewThread == INVALID_HANDLE_VALUE) {
		cout << "CreateRemoteThread Failed\n";
		return false;
	}
	// SetThreadDescription(hNewThread, L"Wind");
	WaitForSingleObject(hNewThread, INFINITE);
	VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
	CloseHandle(hNewThread);

	ResumeThread(hThread);
	ResumeThread(wLinkThread);

}

bool IsFileExist(string &path) {
	struct stat buffer;
	return stat(path.c_str(), &buffer) == 0;
}

int main() {
	char szFilePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	injectDllPath = szFilePath;
	injectDllPath = injectDllPath.substr(0, injectDllPath.find_last_of('\\') + 1) + "WindInject.dll";
	if (!IsFileExist(injectDllPath)) {
		cout << "Error: " << injectDllPath << " Not Found!";
		return -1;
	}

	char commandLine[MAX_PATH + 1];
	cin >> commandLine;
	if (!CreateProcess(commandLine) && hProcess != NULL) {
		TerminateProcess(hProcess, -1);
		return -1;
	}
	if (!CreateWLink(hProcess)) {
		return -1;
	}
	if (!InjectDll() && hProcess != NULL) {
		TerminateProcess(hProcess, -1);
		return -1;
	}
	HandleCommand();
	TerminateProcess(hProcess, -1);
	CloseHandle(hProcess);
}