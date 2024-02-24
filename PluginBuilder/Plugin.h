#pragma once
#include <string>
#include <vector>
#include <map>
#include <imgui.h>
#include <deque>
#include <raylib.h>

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
		int width = 0;
		int height = 0;
		std::string source;
		int boolean_checked = 0;
		float number_default = 0;
		std::string text_default = "text";
		bool entry_provider = false;
		std::string provider_name;
		bool input_hastext = false;
		std::string input_text;
	};

	struct Category {
		std::string name;
		ImVec4 color;
		bool isapi = false;
		bool ischild = false;
		int parent_int = 0;
		std::string parent_name = "Block procedures";
	};

	struct Procedure {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> version_names;
		std::map<std::pair<std::string, std::string>, std::string> code;
		std::vector<Component> components;
		std::vector<std::string> component_names;
		int category = 0;
		std::string category_name = "Block data";
		int type = 0;
		int type_index = 0;
		ImVec4 color;
		std::string translationkey;
		std::string name;
		bool world_dependency = false;
		bool requires_api = false;
		std::string api_name;
		bool has_mutator = false;
		std::string mutator;
	};

	struct GlobalTrigger {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> version_names;
		std::map<std::pair<std::string, std::string>, std::string> event_code;
		std::map<std::pair<std::string, std::string>, std::map<std::string, std::string>> dependency_mappings;
		bool dependencies[12] = {false, false, false, false, false, false, false, false, false, false, false, false};
		bool cancelable = false;
		int side = 0;
		bool manual_code = false;
		std::string json_code;
		std::string name;
	};

	struct Datalist {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> version_names;
		std::vector<std::string> entries;
		std::map<std::pair<std::string, std::string>, std::vector<std::string>> mappings;
		std::map<std::pair<std::string, std::string>, std::deque<bool>> exclusions;
		std::string title;
		std::string message;
		std::string name;
	};

	struct Translation {
		std::vector<std::pair<std::string, std::string>> keys;
		std::string language;
		std::string name;
	};

	struct Api {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> version_names;
		std::map<std::pair<std::string, std::string>, std::string> code;
		std::string name;
	};

	struct Animation {
		std::vector<std::pair<int, std::string>> lines; // rotation, code
		std::string name;
	};

	struct Mutator {
		std::string container_name;
		std::string input_name;
		int variable_int = 0;
		ImVec4 color;
		std::string name;
	};

	static enum WidgetType {
		EMPTY_BOX, LABEL, CHECKBOX, NUMBER_FIELD, TEXT_FIELD, DROPDOWN, ITEM_SELECTOR, TEXTURE_SELECTOR, MODEL_SELECTOR, PROCEDURE_SELECTOR, ENTITY_SELECTOR
	};

	struct Widget {
		WidgetType type = EMPTY_BOX;
		int type_int = 0;
		std::string labeltext;
		std::string varname;
		bool has_tooltip = false;
		std::string tooltip;
		std::string displayname = "[EMPTY BOX]";
		bool append_label = false;
		float step_amount = 0.1;
		float max_value = 10;
		float min_value = 0;
		int textfield_length = 0;
		bool textfield_validated = false;
		bool textfield_elementname = false;
		std::vector<std::string> dropdown_options;
		bool blocks_only = false;
		int texture_type = 0;
		int model_type = 0;
		bool is_condition = false;
		int return_type = 0;
		bool dependencies[12] = { false, false, false, false, false, false, false, false, false, false, false, false };
		std::string procedure_title;
		std::string procedure_tooltip;
		int procedure_side = 0;
	};

	struct ModElement {
		std::vector<std::pair<std::string, std::string>> versions; // version, generator
		std::vector<std::string> version_names;
		std::string description;
		Texture light_icon = { 0 };
		Texture dark_icon = { 0 };
		std::string light_icon_path;
		std::string dark_icon_path;
		std::deque<std::pair<std::pair<int, bool>, std::string>> pages; // rows, name
		std::vector<std::string> page_names;
		std::map<std::string, std::map<std::pair<int, int>, Widget>> widgets;
		std::vector<std::string> local_templates;
		std::vector<std::string> global_templates;
		std::vector<std::string> template_names;
		std::map<std::pair<std::string, std::pair<std::string, std::string>>, std::string> code;
		int selected_global = -1;
		int selected_local = -1;
		std::deque<bool> needs_validator;
		int base_type = 0;
		std::string name;
	};

	struct TemplateOverride {
		std::pair<std::string, std::string> version; // version, generator
		std::string code;
		std::string dirpath;
		std::string name;
	};

	struct TemplateLists {
		std::vector<std::pair<std::string, std::string>> versions;
		std::map< std::pair<std::string, std::string>, std::vector<std::string>> dirpaths;
		std::map< std::pair<std::string, std::string> /* give a version to get template list */, std::vector< std::pair< std::string, std::string > /* a template (name/text) */ > /* end of result vector (template list) */ > templates;
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
		std::vector<Translation> translations;
		std::vector<Api> apis;
		std::vector<Animation> animations;
		std::vector<ModElement> modelements;
		std::vector<Mutator> mutators;
		std::vector<TemplateOverride> overrides;
		std::vector<std::string> filenames;
	};

public:
	Data data;

};

