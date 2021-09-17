#include <string>
#include <iostream>
#include <Windows.h>

using namespace std;

string injectDllPath;

void CreateProcess(const LPWSTR lpCommandLine) {
	//启动目标进程
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	//pi:创建线程返回的信息
	PROCESS_INFORMATION pi = { 0 };
	BOOL bRet = CreateProcess(NULL, lpCommandLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);

	const auto hProcess = pi.hProcess;
	// 创建虚拟内存地址，放置dll路径
	const auto pDllPath = VirtualAllocEx(hProcess, NULL, injectDllPath.size() + 1, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(hProcess, pDllPath, injectDllPath.c_str(), injectDllPath.size() + 1, NULL);
	// 获取LoadLibraryA地址:用于注入dll;
	const auto pfnLoadLib = reinterpret_cast<PTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "LoadLibraryA"));
	// 在线程中执行dll中的入口函数：即导入dll
	const HANDLE hNewThread = CreateRemoteThread(hProcess, NULL, 0, pfnLoadLib, pDllPath, 0, NULL);
	WaitForSingleObject(hNewThread, INFINITE);
	VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
	CloseHandle(hNewThread);
	CloseHandle(hProcess);
	ResumeThread(pi.hThread);
}

bool IsFileExist(string &path) {
	struct stat buffer;
	return stat(path.c_str(), &buffer) == 0;
}

int main() {
	TCHAR szFilePath[MAX_PATH + 1];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	size_t count;
	char path[MAX_PATH + 1] = { 0 };
	wcstombs_s(&count, path, szFilePath, MAX_PATH);
	injectDllPath = path;
	injectDllPath = injectDllPath.substr(0, injectDllPath.find_last_of('\\') + 1) + "WindInject.dll";
	if (!IsFileExist(injectDllPath)) {
		cout << "Error: " << injectDllPath << " Not Found!";
		return -1;
	}

	char commandLine[MAX_PATH + 1];
	cin >> commandLine;
	wchar_t wc[MAX_PATH + 1];
	mbstowcs_s(&count, wc, commandLine, MAX_PATH);
	CreateProcess(wc);
}