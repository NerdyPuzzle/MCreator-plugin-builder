#pragma once
#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

std::wstring s2ws2(std::string str);

void RunExe(std::string path);
bool DownloadFile(const std::string link, const std::string location);
bool IsProcessRunning(const wchar_t* processName);

class Zip {
public:
	static Zip Create(std::string path);
	static Zip Open(std::string path);
	void AddFile(std::string path_from, std::string path_to);
	void AddFolder(std::string path);
	void UnzipFile(std::string path);
	void SetUnzipDir(std::string path);
	void UnzipEverything();
	void Close();
private:
	void* value;
};