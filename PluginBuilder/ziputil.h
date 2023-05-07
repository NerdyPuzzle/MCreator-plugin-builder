#pragma once
#include <string>

std::wstring s2ws2(std::string str);

void RunExe(std::string path);

class Zip {
public:
	static Zip Create(std::string path);
	static Zip Open(std::string path);
	void AddFile(std::string path_from, std::string path_to);
	void AddFolder(std::string path);
	void Close();
private:
	void* value;
};