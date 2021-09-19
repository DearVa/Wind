#include "Wlink.h"
#include <iostream>

#define WLINK_SIZE (1024 * 1024)

HANDLE wHostMutex = nullptr;
HANDLE wClientMutex = nullptr;
HANDLE wLinkThread = nullptr;
BYTE *wHostMmp = nullptr;
BYTE *wClientMmp = nullptr;
size_t pReadBuf = 0;
size_t pSendBuf = 0;

const auto linkSize = 2 * sizeof(HANDLE) + 8;

void ReadMsgH(const uint size, void *buf) {
	if (pReadBuf == WLINK_SIZE) {
		pReadBuf = size;
		memcpy(buf, wClientMmp, size);
		return;
	}
	const auto firstSize = WLINK_SIZE - pReadBuf;
	if (firstSize >= size) {
		memcpy(buf, wClientMmp + pReadBuf, size);
		pReadBuf += size;
		if (pReadBuf == WLINK_SIZE) {
			pReadBuf = 0;
		}
	} else {
		memcpy(buf, wClientMmp + pReadBuf, firstSize);
		pReadBuf = size - firstSize;
		memcpy(static_cast<BYTE *>(buf) + firstSize, wClientMmp, pReadBuf);
	}
}

void ReadMsgC(const uint size, void *buf) {
	if (pReadBuf == WLINK_SIZE) {
		pReadBuf = size;
		memcpy(buf, wHostMmp, size);
		return;
	}
	const auto firstSize = WLINK_SIZE - pReadBuf;
	if (firstSize >= size) {
		memcpy(buf, wHostMmp + pReadBuf, size);
		pReadBuf += size;
		if (pReadBuf == WLINK_SIZE) {
			pReadBuf = 0;
		}
	} else {
		memcpy(buf, wHostMmp + pReadBuf, firstSize);
		pReadBuf = size - firstSize;
		memcpy(static_cast<BYTE *>(buf) + firstSize, wHostMmp, pReadBuf);
	}
}

BYTE msgBuf[WLINK_SIZE];

void LinkThreadWorkH() {
	while (true) {
		WaitForSingleObject(wClientMutex, INFINITE);
		while (true) {
			if (pReadBuf + sizeof(uint) > WLINK_SIZE) {
				pReadBuf = 0;
			}
			uint magicNumber;
			memcpy(&magicNumber, wClientMmp + pReadBuf, sizeof(uint));
			if (magicNumber != WMSG_MAGIC_NUMBER) {
				break;
			}
			const uint erase = 0;
			memcpy(wClientMmp + pReadBuf, &erase, sizeof(uint));
			pReadBuf += sizeof(uint);
			WLinkMsg wLinkMsg;
			ReadMsgH(sizeof(WLinkMsg), &wLinkMsg);
			if (wLinkMsg.msgSize > WLINK_SIZE) {
				MessageBoxA(nullptr, "WLink broken, maybe overflow.", "Critical", MB_ICONERROR);
				return;
			}
			ReadMsgH(wLinkMsg.msgSize, msgBuf);
			switch (wLinkMsg.msgType) {
			case 0:
				puts(reinterpret_cast<char *>(msgBuf));
				break;
			case 1:
				MessageBoxA(nullptr, reinterpret_cast<LPCSTR>(msgBuf), "Alert", 0);
			}
		}
		ReleaseMutex(wClientMutex);
	}
}

void LinkThreadWorkC() {
	while (true) {
		WaitForSingleObject(wHostMutex, INFINITE);
		while (true) {
			if (pReadBuf + sizeof(uint) > WLINK_SIZE) {
				pReadBuf = 0;
			}
			uint magicNumber;
			memcpy(&magicNumber, wHostMmp + pReadBuf, sizeof(uint));
			if (magicNumber != WMSG_MAGIC_NUMBER) {
				break;
			}
			const uint erase = 0;
			memcpy(wHostMmp + pReadBuf, &erase, sizeof(uint));
			pReadBuf += sizeof(uint);
			WLinkMsg wLinkMsg;
			ReadMsgC(sizeof(WLinkMsg), &wLinkMsg);
			if (wLinkMsg.msgSize > WLINK_SIZE) {
				MessageBoxA(nullptr, "WLink broken, maybe overflow.", "Critical", MB_ICONERROR);
				return;
			}
			ReadMsgC(wLinkMsg.msgSize, msgBuf);
			switch (wLinkMsg.msgType) {
			case 0:
				puts(reinterpret_cast<char *>(msgBuf));
				break;
			case 1:
				MessageBoxA(nullptr, reinterpret_cast<LPCSTR>(msgBuf), "Alert", 0);
			}
		}
		ReleaseMutex(wHostMutex);
	}
}


using namespace std;

bool CreateWLink(const HANDLE hProcess) {
	if (wHostMutex != nullptr) {
		return false;
	}
	char randName[10];
	srand(time(nullptr));
	for (int i = 0; i < 8; i++) {
		randName[i] = rand() % 26 + 'a';
	}
	randName[9] = 0;

	randName[8] = 'H';
	wHostMutex = CreateMutex(nullptr, true, randName);
	if (wHostMutex == nullptr) {
		cout << "CreateMutex Failed\n";
		return false;
	}

	randName[8] = 'C';
	wClientMutex = CreateMutex(nullptr, false, randName);
	if (wClientMutex == nullptr) {
		cout << "CreateMutex Failed\n";
		return false;
	}
	wLinkThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LinkThreadWorkH), nullptr, CREATE_SUSPENDED, nullptr);

	// Use FileMapping to Pass Link Infos
	auto hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, linkSize, "WindMmp");
	if (hMapFile == nullptr) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}
	const auto lpMemFile = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, linkSize));
	if (lpMemFile == nullptr) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}

	// Write Link Infos
	HANDLE dMutex;
	DuplicateHandle(GetCurrentProcess(), wHostMutex, hProcess, &dMutex, 0, FALSE, DUPLICATE_SAME_ACCESS);
	memcpy(lpMemFile, &dMutex, sizeof(dMutex));
	DuplicateHandle(GetCurrentProcess(), wClientMutex, hProcess, &dMutex, 0, FALSE, DUPLICATE_SAME_ACCESS);
	memcpy(lpMemFile + sizeof(dMutex), &dMutex, sizeof(dMutex));

	memcpy(lpMemFile + 2 * sizeof(dMutex), randName, sizeof(randName) - 2);

	// Create Host mmap
	randName[8] = 'h';
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, WLINK_SIZE, randName);
	if (hMapFile == nullptr) {
		cout << "MapViewOfFile Failed: " << GetLastError();
		return false;
	}
	wHostMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wHostMmp == nullptr) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}

	// Create Client mmap
	randName[8] = 'c';
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, WLINK_SIZE, randName);
	if (hMapFile == nullptr) {
		cout << "MapViewOfFile Failed\n";
		return false;
	}
	wClientMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wClientMmp == nullptr) {
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
	memcpy(&wHostMutex, lpMemFile, sizeof(wHostMutex));
	memcpy(&wClientMutex, lpMemFile + sizeof(wHostMutex), sizeof(wClientMutex));
	WaitForSingleObject(wClientMutex, INFINITE);  // Get Mutex

	char wMutexName[10];
	memcpy(wMutexName, lpMemFile + 2 * sizeof(wHostMutex), sizeof(wMutexName) - 2);
	wMutexName[9] = 0;
	CloseHandle(hMapFile);
	
	wLinkThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LinkThreadWorkC), nullptr, CREATE_SUSPENDED, nullptr);

	wMutexName[8] = 'h';
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, wMutexName);
	if (hMapFile == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	wHostMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wHostMmp == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}

	wMutexName[8] = 'c';
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, wMutexName);
	if (hMapFile == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	wClientMmp = static_cast<BYTE *>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, WLINK_SIZE));
	if (wClientMmp == nullptr) {
		// DP0("[Wind] MapViewOfFile Failed");
		return false;
	}
	return true;
}

void WriteMsgH(const size_t size, void *msg) {
	if (pSendBuf == WLINK_SIZE) {
		pSendBuf = size;
		memcpy(wHostMmp, msg, size);
		return;
	}
	const auto firstSize = WLINK_SIZE - pSendBuf;
	if (firstSize >= size) {
		memcpy(wHostMmp + pSendBuf, msg, size);
		pSendBuf += size;
		if (pSendBuf == WLINK_SIZE) {
			pSendBuf = 0;
		}
	} else {
		memcpy(wHostMmp + pSendBuf, msg, firstSize);
		pSendBuf = size - firstSize;
		memcpy(wHostMmp, static_cast<BYTE *>(msg) + firstSize, pSendBuf);
	}
}

void WriteMsgC(const size_t size, void *msg) {
	if (pSendBuf == WLINK_SIZE) {
		pSendBuf = size;
		memcpy(wClientMmp, msg, size);
		return;
	}
	const auto firstSize = WLINK_SIZE - pSendBuf;
	if (firstSize >= size) {
		memcpy(wClientMmp + pSendBuf, msg, size);
		pSendBuf += size;
		if (pSendBuf == WLINK_SIZE) {
			pSendBuf = 0;
		}
	} else {
		memcpy(wClientMmp + pSendBuf, msg, firstSize);
		pSendBuf = size - firstSize;
		memcpy(wClientMmp, static_cast<BYTE *>(msg) + firstSize, pSendBuf);
	}
}

void SendWMessageH(const uint msgType, const uint msgSize, void *msg) {
	if (wHostMmp == nullptr) {
		return;
	}
	const auto fullMsgSize = msgSize + sizeof(WLinkMsg) + sizeof(uint);
	if (fullMsgSize > WLINK_SIZE) {
		return;
	}
	if (pSendBuf + sizeof(uint) >= WLINK_SIZE) {
		pSendBuf = 0;
	}
	const uint magicNumber = WMSG_MAGIC_NUMBER;
	memcpy(wHostMmp + pSendBuf, &magicNumber, sizeof(magicNumber));
	pSendBuf += sizeof(magicNumber);
	WLinkMsg wLinkMsg = { msgType, msgSize };
	WriteMsgH(sizeof(wLinkMsg), &wLinkMsg);
	WriteMsgH(msgSize, msg);
	ReleaseMutex(wHostMutex);
	WaitForSingleObject(wHostMutex, INFINITE);
}

void SendWMessageC(const uint msgType, const uint msgSize, void *msg) {
	if (wClientMmp == nullptr) {
		return;
	}
	const auto fullMsgSize = msgSize + sizeof(WLinkMsg) + sizeof(uint);
	if (fullMsgSize > WLINK_SIZE) {
		return;
	}
	if (pSendBuf + sizeof(uint) >= WLINK_SIZE) {
		pSendBuf = 0;
	}
	const uint magicNumber = WMSG_MAGIC_NUMBER;
	memcpy(wClientMmp + pSendBuf, &magicNumber, sizeof(magicNumber));
	pSendBuf += sizeof(magicNumber);
	WLinkMsg wLinkMsg = { msgType, msgSize };
	WriteMsgC(sizeof(wLinkMsg), &wLinkMsg);
	WriteMsgC(msgSize, msg);
	ReleaseMutex(wClientMutex);
	WaitForSingleObject(wClientMutex, INFINITE);
}
