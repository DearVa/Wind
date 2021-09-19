#include "HookThread.h"
#include "Wlog.h"

WThread wThreads[MAX_THREAD_NUM];
HookInfo createRemoteThreadExHookInfo, getThreadIdHookInfo, closeHandleHookInfo, terminateThreadHookInfo;

BOOL WThread::isAlive() {
	if (this->internalHandle == NULL) {
		return false;
	}
	DWORD exitCode;
	if (GetExitCodeThread(this->internalHandle, &exitCode)) {
		if (exitCode == STILL_ACTIVE) {
			return true;
		}
		this->internalHandle = NULL;
		return false;
	}
	return false;
}

WThread *GetWThread(HANDLE hThread) {
	const auto index = reinterpret_cast<DWORD>(hThread);
	if (index >= MAGIC_NUMBER && index < MAGIC_NUMBER + MAX_THREAD_NUM) {
		auto &wThread = wThreads[index - MAGIC_NUMBER];
		if (wThread.originHandle != NULL && wThread.isAlive()) {
			return &wThread;
		}
	}
	return nullptr;
}

void ListWThreads(char *buf) {
	
}

EXTERN_C_START
// _beginthreadex, CreateThread, CreateRemoteThread finally call the CreateRemoteThreadEx.
_Ret_maybenull_ HANDLE WINAPI WCreateRemoteThreadEx(HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList, LPDWORD lpThreadId) {
	HookOff(createRemoteThreadExHookInfo);
	const auto hThread = CreateRemoteThreadEx(hProcess, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpAttributeList, lpThreadId);
	HookOn(createRemoteThreadExHookInfo);

	DP3("[Wind] call CreateThread, hProcess: %p, lpStartAddress: %p, lpThreadId: %p", hProcess, lpStartAddress, lpThreadId);

	if (hThread == NULL) {
		return NULL;
	}

	if (hProcess != reinterpret_cast<HANDLE>(0xFFFFFFFF) && hProcess != wHProcess) {
		return hThread;
	}

	int mappedThreadHandle = 0;
	for (int i = 0; i < MAX_THREAD_NUM; i++) {
		if (!wThreads[i].isAlive()) {  // Find a valid thread
			mappedThreadHandle = i + MAGIC_NUMBER;
			wThreads[i].mappedHandleId = mappedThreadHandle;
			wThreads[i].originHandle = hThread;
			wThreads[i].originId = *lpThreadId;
			wThreads[i].internalHandle = OpenThread(THREAD_ALL_ACCESS, false, *lpThreadId);
			*lpThreadId = mappedThreadHandle;
			break;
		}
	}

	if (mappedThreadHandle == 0) {
		MessageBoxA(nullptr, "Cannot Alloc Thread.", "Critical", 0);
		exit(-1);
	}

	DP2("[Wind] call CreateThread, Remap: %p -> %p", hThread, mappedThreadHandle);
	return reinterpret_cast<HANDLE>(mappedThreadHandle);
}

DWORD WINAPI WGetThreadId(HANDLE Thread) {
	DP1("[Wind] call GetThreadId, Thread: %p", Thread);

	const auto index = reinterpret_cast<DWORD>(Thread);
	if (index >= MAX_THREAD_NUM) {
		return 0;
	}
	auto &wThread = wThreads[index];
	if (wThread.isAlive()) {
		return wThread.mappedHandleId;
	}
	return 0;
}

BOOL WINAPI WCloseHandle(HANDLE hObject) {
	DP1("[Wind] call CloseHandle, hObject: %p", hObject);
	if (hObject == NULL) {
		return false;
	}
	BOOL result;
	const auto index = reinterpret_cast<DWORD>(hObject);
	if (index >= MAGIC_NUMBER && index < MAGIC_NUMBER + MAX_THREAD_NUM) {
		auto &wThread = wThreads[index - MAGIC_NUMBER];
		if (wThread.originHandle != NULL && wThread.isAlive()) {
			HookOff(closeHandleHookInfo);
			result = CloseHandle(wThread.originHandle);
			HookOn(closeHandleHookInfo);
			wThread.originHandle = NULL;
			DP1("[Wind] Close Mapped thread, alive thread count: %d", GetAliveThreadCount());
		} else {
			result = false;
		}
	} else {
		HookOff(closeHandleHookInfo);
		result = CloseHandle(hObject);
		HookOn(closeHandleHookInfo);
		DP0("[Wind] Close System Handle");
	}
	return result;
}

BOOL WINAPI WTerminateThread(HANDLE hThread, DWORD dwExitCode) {
	const auto wThread = GetWThread(hThread);
	if (wThread == nullptr) {
		return false;
	}
	HookOff(terminateThreadHookInfo);
	const BOOL result = TerminateThread(wThread->originHandle, dwExitCode);
	HookOn(terminateThreadHookInfo);
	wThread->originHandle = NULL;
	wThread->internalHandle = NULL;
	return result;
}
EXTERN_C_END

int GetAliveThreadCount() {
	int count = 0;
	for (int i = 0; i < MAX_THREAD_NUM; i++) {
		if (wThreads[i].isAlive()) {
			count++;
		}
	}
	return count;
}
