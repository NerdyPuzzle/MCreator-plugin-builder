#pragma once
#include <string>
#include <vector>
#include <map>
#include <imgui.h>

class Plugin {

public:
	std::string Dependencies[12] {
		"X",
		"Y",
		"Z",
		"ENTITY",
		"SOURCEENTITY",
		"IMMEDIATESOURCEENTITY",
		"WORLD",
		"ITEMSTACK",
		"BLOCKSTATE",
		"DIRECTION",
		"ADVANCEMENT",
		"DIMENSION"
	};

	std::string ComponentValues[7] {
		"Block",
		"Direction",
		"Entity",
		"Item",
		"Boolean",
		"Number",
		"Text"
	};

	struct Component {
		int type_int = 0;
		std::string name;
		std::string component_value = "Block";
		int value_int = 0;
		std::vector<std::string> dropdown_options;
		std::string datalist;
		bool disable_localvars = false;
		bool checkbox_checked = false;
		bool open = true;
	};

	struct Category {
		std::string name;
		ImVec4 color;
		bool isapi;
	};

	struct Procedure {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::map<std::pair<std::string, std::string>, std::string> code;
		std::vector<Component> components;
		int category = 0;
		std::string category_name = "Block data";
		int type = 0;
		int type_index = 0;
		ImVec4 color;
		std::string translationkey;
		std::string name;
		bool world_dependency = false;
	};

	struct GlobalTrigger {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::map<std::pair<std::string, std::string>, std::string> event_code;
		std::map<std::pair<std::string, std::string>, std::map<std::string, std::string>> dependency_mappings;
		bool dependencies[12] = {false, false, false, false, false, false, false, false, false, false, false, false};
		bool cancelable = false;
		int side = 0;
		std::string name;
	};

	struct Datalist {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> entries;
		std::map<std::pair<std::string, std::string>, std::vector<std::string>> mappings;
		std::string name;
	};

private:
	struct Data {
		std::string name;
		std::string id;
		std::string author;
		std::string version;
		std::string minversion;
		std::string maxversion;
		std::string description;
		std::string credits;
		std::vector<Category> categories;
		std::vector<Procedure> procedures;
		std::vector<GlobalTrigger> globaltriggers;
		std::vector<Datalist> datalists;
		std::vector<std::string> filenames;
	};

public:
	Data data;

};

