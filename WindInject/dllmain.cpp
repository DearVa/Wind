#include <iostream>
#include <Windows.h>
#include <atlconv.h>
#include <Psapi.h>
#pragma comment(lib,"psapi.lib")

#include <process.h>

#include "Hook.h"
#include "HookThread.h"
#include "log.h"

using namespace std;

bool injected;

EXTERN_C_START
void Inject() {
	if (injected == false) {
		injected = true;

		const HMODULE hKernel32Module = GetModuleHandle(TEXT("kernel32"));
		if (hKernel32Module == nullptr) {
			MessageBoxA(nullptr, "Hook Failed: GetModuleHandle", "Error", 0);
			exit(-1);
		}

		CreateHook(hKernel32Module, createRemoteThreadExHookInfo, "CreateRemoteThreadEx", WCreateRemoteThreadEx);
		CreateHook(hKernel32Module, getThreadIdHookInfo, "GetThreadId", WGetThreadId);
		CreateHook(hKernel32Module, closeHandleHookInfo, "CloseHandle", WCloseHandle);

		/*const HMODULE hUcrtbaseModule = GetModuleHandle(TEXT("ucrtbase"));
		if (hUcrtbaseModule != nullptr) {
			CreateHook(hUcrtbaseModule, beginthreadexHookInfo, "_beginthreadex", W_beginthreadex);
			CreateHook(hUcrtbaseModule, beginthreadHookInfo, "_beginthread", W_beginthread);
		}*/
	}
}

void WINAPI Initialize() {
	memset(wThreads, 0, sizeof(wThreads));
	wHProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, GetCurrentProcessId());
	Inject();
	DP1("[Wind] Injected, HProcess: 0x%p", wHProcess);
}
EXTERN_C_END

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		Initialize();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
