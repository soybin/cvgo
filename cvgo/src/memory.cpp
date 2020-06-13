#include "memory.h"

bool memory::setProcessHandle(const wchar_t* name) {
	pId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 pEntry = { 0 };
	pEntry.dwSize = sizeof(pEntry);
	if (Process32First(hSnap, &pEntry)) {
		for (; Process32Next(hSnap, &pEntry); ) {
			if (!wcscmp(pEntry.szExeFile, name)) {
				CloseHandle(hSnap);
				pId = pEntry.th32ProcessID;
				pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pId);
				return true;
			}
		}
	}
	return false;
}

bool memory::setModuleInfo(const wchar_t* name) {
	MODULEENTRY32 modEntry = { 0 };
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pId);
	modEntry.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(hSnap, &modEntry)) {
		for (; Module32Next(hSnap, &modEntry); ) {
			if (!wcscmp(modEntry.szModule, name)) {
				modules[name] = { (unsigned long)modEntry.modBaseAddr, modEntry.modBaseSize };
				CloseHandle(hSnap);
				return true;
			}
		}
	}
	CloseHandle(hSnap);
	return false;
}

unsigned long memory::patternScan(const char* pattern, const char* mask, const unsigned long begin, const unsigned long size) {
	unsigned long ret = 0;
	char* buffer = new char[size];
	int length = strlen(mask);
	ReadProcessMemory(pHandle, (const void*)begin, buffer, size, NULL);
	for (int i = 0; i < size - length; ++i) {
		int j = 0;
		for (; j < length; ++j) {
			if (mask[j] != '?' && pattern[j] != *(char*)(buffer + i + j)) break;
		}
		if (j == length) { ret = begin + i; break; }
	}
	delete[] buffer;
	return ret;
}

memory::memory() {
}

memory::~memory() {
	CloseHandle(pHandle);
}

bool memory::init(const wchar_t* pName, std::vector<const wchar_t*> moduleNames) {
	if (!setProcessHandle(pName)) return false;
	for (auto& name : moduleNames) setModuleInfo(name);
	return true;
}

unsigned long memory::getOffset(const patternStruct& patt) {
	unsigned long addr = 0, oldprotect;
	MEMORY_BASIC_INFORMATION mbi;
	mbi.RegionSize = 0x1000;
	for (unsigned long curr = modules[patt.moduleName].first; curr < modules[patt.moduleName].first + modules[patt.moduleName].second; curr += mbi.RegionSize) {
		if (!VirtualQueryEx(pHandle, (const void*)curr, &mbi, sizeof(mbi))) continue;
		if (!VirtualProtectEx(pHandle, mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldprotect)) continue;
		VirtualProtectEx(pHandle, mbi.BaseAddress, mbi.RegionSize, oldprotect, &oldprotect);
		addr = patternScan(patt.pattern, patt.mask, curr, mbi.RegionSize);
		if (addr) break;
	}

	for (auto& offset : patt.offsets) {
		addr += offset;
		addr = read<unsigned long>(addr);
		addr -= modules[patt.moduleName].first;
	}
	if (patt.relative) return (addr + patt.extra);
	// i do not know why. it just works; i get rid of the first four bytes
	// and then the rest of the address equals to the offset.
	return (addr + patt.extra) & 0x0000ffff;
}
