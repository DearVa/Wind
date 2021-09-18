#pragma once
#include <Windows.h>

extern HANDLE wHProcess;

struct HookInfo {
	BOOL hooked;
	BYTE oldCode[5];  // �ϵ�ϵͳAPI��ڴ���
	BYTE newCode[5];  // Ҫ��ת��API���� (jmp xxxx)
	FARPROC farProc;
};

void HookOn(HookInfo &hookInfo);
void HookOff(HookInfo &hookInfo);
void CreateHook(HMODULE hModule, HookInfo &hookInfo, LPCSTR hookName, void *callback);