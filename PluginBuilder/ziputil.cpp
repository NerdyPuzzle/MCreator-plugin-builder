#define _CRT_SECURE_NO_WARNINGS
#include "ziputil.h"
#include "zip.h"
#pragma comment(lib, "Urlmon.lib")
#include "unzip.h"
#include <atlstr.h>
#include <tchar.h>
#include <iostream>
#include <atlstr.h>
#include <tlhelp32.h>

#define MAX_TCHAR_BUFFER_SIZE 1024

std::wstring s2ws2(const std::string str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	std::wcout << wstrTo << L"\n";
	return wstrTo;
}

void RunExe(std::string path) {
	ShellExecute(NULL, L"open", s2ws2(path).c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

bool DownloadFile(const std::string link, const std::string location) {
	const wchar_t* sURL = s2ws2(link).c_str();
	const wchar_t* dFile = s2ws2(location).c_str();
	if (S_OK == URLDownloadToFile(NULL, sURL, dFile, 0, NULL))
	{
		std::wcout << L"successfully downloaded file " + s2ws2(link) + L"\n";
		return true;
	}
	else
	{
		std::wcerr << L"Failed to download file from link " + s2ws2(link) + L"\n";
		return false;
	}
}

bool IsProcessRunning(const wchar_t* processName) {
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry))
		while (Process32Next(snapshot, &entry))
			if (!_wcsicmp(entry.szExeFile, processName))
				exists = true;

	CloseHandle(snapshot);
	return exists;
}

Zip Zip::Open(std::string path) {
	Zip zip;
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	zip.value = OpenZip(t, 0);
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

std::vector<fs::path> Zip::ZipIterator() {
	std::vector<fs::path> paths;
	ZIPENTRY ze; 
	GetZipItem((HZIP)value, -1, &ze);
	int numitems = ze.index;
	for (int i = 0; i < numitems; i++) {
		GetZipItem((HZIP)value, i, &ze);
		paths.push_back(ze.name);
	}
	return paths;
}

std::vector<fs::path> Zip::ZipIterator(std::string path) {
	std::vector<fs::path> paths;
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	ZIPENTRY ze;
	SetUnzipBaseDir((HZIP)value, t);
	GetZipItem((HZIP)value, -1, &ze);
	int numitems = ze.index;
	for (int i = 0; i < numitems; i++) {
		GetZipItem((HZIP)value, i, &ze);
		paths.push_back(ze.name);
	}
	return paths;
}

void Zip::UnzipFile(std::string path) {
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	ZIPENTRY ze;
	int index = -1;
	FindZipItem((HZIP)value, t, false, &index, &ze);
	UnzipItem((HZIP)value, index, ze.name);
}

void Zip::Close() {
	CloseZip((HZIP)value);
}
