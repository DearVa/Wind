#include "Hook.h"

HANDLE wHProcess;

void HookOn(HookInfo &hookInfo) {
	if (hookInfo.hooked) {
		return;
	}
	DWORD dwTemp = 0;
	DWORD dwOldProtect;

	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(wHProcess, hookInfo.farProc, hookInfo.newCode, 5, nullptr);
	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, dwOldProtect, &dwTemp);

	// DP2("[Wind] HookOn, farProc: 0x%X, newCode: 0x%X", hookInfo.farProc, hookInfo.newCode);
	
	hookInfo.hooked = true;
}

void HookOff(HookInfo &hookInfo) {
	if (!hookInfo.hooked) {
		return;
	}
	DWORD dwTemp = 0;
	DWORD dwOldProtect;

	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, PAGE_READWRITE, &dwOldProtect);
	WriteProcessMemory(wHProcess, hookInfo.farProc, hookInfo.oldCode, 5, nullptr);
	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, dwOldProtect, &dwTemp);
	hookInfo.hooked = false;
}

void CreateHook(const HMODULE hModule, HookInfo &hookInfo, const LPCSTR hookName, void *callback) {
	hookInfo.hooked = false;
	hookInfo.farProc = GetProcAddress(hModule, hookName);
	if (hookInfo.farProc == nullptr) {
		MessageBoxA(nullptr, "Hook Failed: GetProcAddress", "Error", 0);
		exit(-1);
	}
	DWORD dwTemp = 0;
	DWORD dwOldProtect;

	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, PAGE_READWRITE, &dwOldProtect);
	ReadProcessMemory(wHProcess, hookInfo.farProc, hookInfo.oldCode, 5, nullptr);
	VirtualProtectEx(wHProcess, hookInfo.farProc, 5, dwOldProtect, &dwTemp);

	hookInfo.newCode[0] = 0xe9;
	auto address = reinterpret_cast<ULONG_PTR>(callback) - reinterpret_cast<ULONG_PTR>(hookInfo.farProc) - 5;
	memcpy(hookInfo.newCode + 1, &address, 6);
	HookOn(hookInfo);
}