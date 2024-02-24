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

#include <wininet.h>
#pragma comment(lib,"wininet.lib")

std::string ReadWebsiteRawData(const std::string& url) {
	HINTERNET hInternet = InternetOpenA("MyBrowser", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) {
		std::cerr << "Error in InternetOpen: " << GetLastError() << std::endl;
		return "";
	}

	HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hConnect) {
		std::cerr << "Error in InternetOpenUrl: " << GetLastError() << std::endl;
		InternetCloseHandle(hInternet);
		return "";
	}

	std::string raw_data;
	char buffer[1024];
	DWORD bytesRead = 0;

	while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		raw_data.append(buffer, bytesRead);
	}

	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return raw_data;
}

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

void Zip::UnzipEverything() {
	ZIPENTRY ze; 
	GetZipItem((HZIP)value, -1, &ze);
	int numitems = ze.index;
	for (int i = 0; i < numitems; i++) {
		GetZipItem((HZIP)value, i, &ze);
		UnzipItem((HZIP)value, i, ze.name);
	}
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

void Zip::SetUnzipDir(std::string path) {
	TCHAR t[MAX_TCHAR_BUFFER_SIZE];
	std::wstring s = s2ws2(path);
	_tcscpy_s(t, s.c_str());
	SetUnzipBaseDir((HZIP)value, t);
}

void Zip::Close() {
	CloseZip((HZIP)value);
}
