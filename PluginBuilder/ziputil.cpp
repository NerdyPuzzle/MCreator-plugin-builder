#define _CRT_SECURE_NO_WARNINGS
#include "ziputil.h"
#include "zip.h"
#include <atlstr.h>
#include <filesystem>
#include <tchar.h>
#include <iostream>
#include <atlstr.h>

#define MAX_TCHAR_BUFFER_SIZE 1024

namespace fs = std::filesystem;

std::wstring s2ws2(const std::string str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	std::wcout << wstrTo << L"\n";
	return wstrTo;
}

/*
TCHAR ToTchar(std::string str) {
	//TCHAR* t = new TCHAR[str.size()];
	TCHAR tt[512];
	std::wstring s = s2ws2(str);
	_tcscpy_s(tt, s.c_str());
	return *tt;
}
*/ // scrapped the concept as it corrupted the heap *shrug*

Zip Zip::Open(std::string path) {
	Zip zip;

	return zip;
}

Zip Zip::Create(std::string path) {
	Zip zip;
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	zip.value = CreateZip(t, 0);
	return zip;
}

void Zip::AddFile(std::string path_from, std::string path_to) {
	TCHAR to[MAX_TCHAR_BUFFER_SIZE];
	TCHAR from[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path_to);
	std::wstring s_ = s2ws2(path_from);
	_tcscpy_s(to, s.c_str());
	_tcscpy_s(from, s_.c_str());
	ZipAdd((HZIP)value, to, from);
}

void Zip::AddFolder(std::string path) {
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	ZipAddFolder((HZIP)value, t);
}

void Zip::Close() {
	CloseZip((HZIP)value);
}
