#include "Wlink.h"
#include <iostream>

#define WLINK_SIZE (1024 * 1024)

char wMutexName[9] = { 0 };
HANDLE wMutex = NULL;
HANDLE wLinkThread = NULL;
BYTE *wMmp;
size_t pBuf = 0;

const auto linkSize = sizeof(HANDLE) + 9;

void ReadMsg(const uint size, void *buf) {
	if (pBuf == WLINK_SIZE) {
		pBuf = size;
		memcpy(buf, wMmp, size);
		return;
	}
	const auto firstSize = WLINK_SIZE - pBuf;
	if (firstSize >= size) {
		memcpy(buf, wMmp + pBuf, size);
		pBuf += size;
		if (pBuf == WLINK_SIZE) {
			pBuf = 0;
		}
	} else {
		memcpy(buf, wMmp + pBuf, firstSize);
		pBuf = size - firstSize;
		memcpy(static_cast<BYTE *>(buf) + firstSize, wMmp, pBuf);
	}
}

BYTE msgBuf[WLINK_SIZE];

void LinkThreadWork() {
	while (true) {
		WaitForSingleObject(wMutex, INFINITE);
		while (true) {
			if (pBuf + sizeof(uint) > WLINK_SIZE) {
				pBuf = 0;
			}
			uint magicNumber;
			memcpy(&magicNumber, wMmp + pBuf, sizeof(uint));
			if (magicNumber != WMSG_MAGIC_NUMBER) {
				break;
			}
			const uint erase = 0;
			memcpy(wMmp + pBuf, &erase, sizeof(uint));
			pBuf += sizeof(uint);
			WLinkMsg wLinkMsg;
			ReadMsg(sizeof(WLinkMsg), &wLinkMsg);
			if (wLinkMsg.msgSize > WLINK_SIZE) {
				MessageBoxA(NULL, "WLink broken, maybe overflow.", "Critical", MB_ICONERROR);
				return;
			}
			ReadMsg(wLinkMsg.msgSize, msgBuf);
			switch (wLinkMsg.msgType) {
			case 0:
				puts(reinterpret_cast<char *>(msgBuf));
				break;
			}
		}
		ReleaseMutex(wMutex);
	}
}

using namespace std;

bool CreateWLink(HANDLE hProcess) {
	if (wMutex != NULL) {
		return false;
	}
	GUID guid;
	CoCreateGuid(&guid);
	char buf[9];
	memcpy(buf, guid.Data4, sizeof(guid.Data4));
	buf[8] = 0;
	wMutex = CreateMutex(NULL, false, wMutexName);
	if (wMutex == NULL) {
		std::cout << "CreateMutex Failed\n";
		return false;
	}
	wLinkThread = CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LinkThreadWork), NULL, CREATE_SUSPENDED, NULL);

	// Use FileMapping to Pass Link Infos
	auto hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, linkSize, "WindMmp");
	if (hMapFile == NULL) {
		std::cout << "MapViewOfFile Failed\n";
		return false;
	}
	const auto lpMemFile = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, linkSize));
	if (lpMemFile == NULL) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}
	// Write Link Infos
	HANDLE dMutex;
	DuplicateHandle(GetCurrentProcess(), wMutex, hProcess, &dMutex, 0, FALSE, DUPLICATE_SAME_ACCESS);
	memcpy(lpMemFile, &dMutex, sizeof(dMutex));
	memcpy(lpMemFile + sizeof(dMutex), buf, sizeof(buf));

	hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, WLINK_SIZE, buf);
	if (hMapFile == nullptr) {
		std::cout << "MapViewOfFile Failed\n";
		return false;
	}
	wMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wMmp == nullptr) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}
	return true;
}

bool OpenWLink() {
	auto hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, "WindMmp");
	if (hMapFile == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	const auto lpMemFile = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, linkSize));
	if (lpMemFile == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	memcpy(&wMutex, lpMemFile, sizeof(wMutex));
	WaitForSingleObject(wMutex, INFINITE);
	char wMutexName[9];
	memcpy(wMutexName, lpMemFile + sizeof(wMutex), sizeof(wMutexName));
	wMutexName[8] = 0;
	CloseHandle(hMapFile);

	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, wMutexName);
	if (hMapFile == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	wMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wMmp == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	return true;
}

void WriteMsg(const size_t size, void *msg) {
	const auto firstSize = WLINK_SIZE - pBuf;
	if (firstSize >= size) {
		memcpy(wMmp + pBuf, msg, size);
		pBuf += size;
		if (pBuf == WLINK_SIZE) {
			pBuf = 0;
		}
	} else {
		memcpy(wMmp + pBuf, msg, firstSize);
		pBuf = size - firstSize;
		memcpy(wMmp, static_cast<BYTE *>(msg) + firstSize, pBuf);
	}
}

void SendWMessage(const uint msgType, const uint msgSize, void *msg) {
	if (wMmp == NULL) {
		return;
	}
	const auto fullMsgSize = msgSize + sizeof(WLinkMsg) + sizeof(uint);
	if (fullMsgSize > WLINK_SIZE) {
		return;
	}
	if (pBuf + sizeof(uint) >= WLINK_SIZE) {
		pBuf = 0;
	}
	const uint magicNumber = WMSG_MAGIC_NUMBER;
	memcpy(wMmp + pBuf, &magicNumber, sizeof(magicNumber));
	pBuf += sizeof(magicNumber);
	WLinkMsg wLinkMsg = { msgType, msgSize };
	WriteMsg(sizeof(wLinkMsg), &wLinkMsg);
	WriteMsg(msgSize, msg);
	ReleaseMutex(wMutex);
	WaitForSingleObject(wMutex, INFINITE);
}
