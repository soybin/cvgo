#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <unordered_map>

struct patternStruct {
	const char* pattern, * mask;
	const wchar_t* moduleName;
	bool relative;
	int extra;
	std::vector<int> offsets;
};

class memory {
private:
	bool setProcessHandle(const wchar_t* name);
	bool setModuleInfo(const wchar_t* name);
	unsigned long patternScan(const char* pattern, const char* mask, const unsigned long begin, const unsigned long size);
public:
	memory();
	~memory();

	std::unordered_map<const wchar_t*, std::pair<unsigned long, unsigned long>> modules;

	HANDLE pHandle;
	int pId;

	bool init(const wchar_t* pName, std::vector<const wchar_t*> moduleNames);
	unsigned long getOffset(const patternStruct& patt);

	template<class T>
	inline T read(unsigned long address);
	template<class T>
	inline void write(unsigned long address, T value);
};

template<class T>
T memory::read(unsigned long address) {
	T ret;
	ReadProcessMemory(pHandle, (void*)address, &ret, sizeof(T), NULL);
	return ret;
}

template<class T>
void memory::write(unsigned long address, T value) {
	WriteProcessMemory(pHandle, (void*)address, &value, sizeof(T), NULL);
}