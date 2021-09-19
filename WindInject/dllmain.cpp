#include <iostream>
#include <Windows.h>
#include <atlconv.h>
#include <Psapi.h>
#pragma comment(lib,"psapi.lib")

#include <process.h>

#include "Hook.h"
#include "HookThread.h"
#include "../WLink/Wlink.h"
#include "Wlog.h"

using namespace std;

bool injected;
HANDLE hMapFile;
BYTE *lpMemFile;

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
		//CreateHook(hKernel32Module, terminateThreadHookInfo, "TerminateThread", WTerminateThread);
	}
}

void WINAPI Initialize() {
	memset(wThreads, 0, sizeof(wThreads));

	if (!OpenWLink()) {
		return;
	}
	
	wHProcess = OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
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
