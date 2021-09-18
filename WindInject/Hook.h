#pragma once
#include <Windows.h>

extern HANDLE wHProcess;

struct HookInfo {
	BOOL hooked;
	BYTE oldCode[5];  // 老的系统API入口代码
	BYTE newCode[5];  // 要跳转的API代码 (jmp xxxx)
	FARPROC farProc;
};

void HookOn(HookInfo &hookInfo);
void HookOff(HookInfo &hookInfo);
void CreateHook(HMODULE hModule, HookInfo &hookInfo, LPCSTR hookName, void *callback);