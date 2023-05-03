#define _CRT_SECURE_NO_WARNINGS
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <iostream>
#include <raylib.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include "rlImGui.h"
#include "Plugin.h"
#include "GuiTheme.h"
#include <filesystem>
#include <fstream>
#include "rlImGuiColors.h"
#include <sstream>
#include "TextEditor.h"
#include "ziputil.h"
#include "gui_file_dialogs.h"

namespace fs = std::filesystem;

bool mainmenu = true;
bool pluginmenu = false;
float offset = 0;
bool creatingplugin = false;
bool creatingplugin_set_pos = true;
bool delete_confirm = false;
bool delete_set_pos = true;
bool deletefile = false;
bool deletefile_set_pos = true;
int delete_procedure = -1;
int delete_globaltrigger = -1;
int delete_category = -1;
std::vector<const char*> tempchars_procedures;
std::vector<const char*> tempchars_globaltriggers;
std::vector<const char*> tempchars_categories;
bool active[3] = { false, false, false };
bool addfile = false;
bool addfile_set_pos = true;
int combo_item = 0;
std::string file_name;
bool file_open = false;
bool addversion = false;
bool addversion_set_pos = true;
std::string version_mc;
int generator_type = 0;
bool addcomp = false;
bool addcomp_set_pos = true;
std::string component_name;
bool change = true;
int tabsize = 0;
int tabsize_old = 0;

std::vector<Plugin> plugins;
Plugin loaded_plugin;
TextEditor code_editor;

// plugin properties variables
std::string pluginname;
std::string pluginid;
std::string pluginversion;
std::string pluginauthor;
std::string plugindescription;

bool HasDependencies(const bool dependencies[12]) {
    for (int i = 0; i < 12; i++)
        if (dependencies[i])
            return true;
    return false;
}
bool IsPluginFieldEmpty() {
    if (pluginname.empty() || pluginid.empty() || pluginversion.empty())
        return true;
    return false;
}
void ClearPluginVars() {
    pluginname.clear();
    pluginid.clear();
    pluginversion.clear();
    pluginauthor.clear();
    plugindescription.clear();
}
void SavePlugin(const Plugin* plugin) {
    if (!DirectoryExists("plugins/"))
        fs::create_directory("plugins/");
    std::string pluginpath = "plugins/" + plugin->data.name + "/";
    if (!DirectoryExists(pluginpath.c_str()))
        fs::create_directory(pluginpath);

    std::ofstream info(pluginpath + "info.txt");
    info << plugin->data.name << "\n";
    info << plugin->data.id << "\n";
    info << plugin->data.version << "\n";
    info << plugin->data.author << "\n";
    info << plugin->data.description << "\n";
    info << plugin->data.credits;
    info.close();

    if (!plugin->data.filenames.empty()) {
        std::ofstream filenames(pluginpath + "filenames.txt");
        bool first = true;
        for (const std::string s : plugin->data.filenames) {
            filenames << (first ? "" : "\n") << s;
            first = false;
        }
        filenames.close();
    }
    else if (fs::exists(pluginpath + "filenames.txt"))
        fs::remove(pluginpath + "filenames.txt");

    if (!plugin->data.categories.empty()) {
        if (!fs::exists(pluginpath + "categories/"))
            fs::create_directory(pluginpath + "categories/");
        for (const Plugin::Category ct : plugin->data.categories) {
            std::ofstream category(pluginpath + "categories/" + ct.name + ".txt");
            category << ct.name << "\n";
            category << ct.isapi << "\n";
            category << ct.color.x << " " << ct.color.y << " " << ct.color.z << " " << ct.color.w;
            category.close();
        }
    }
    else if (fs::exists(pluginpath + "categories/"))
        fs::remove_all(pluginpath + "categories/");

    if (!plugin->data.globaltriggers.empty()) {
        if (!fs::exists(pluginpath + "triggers/"))
            fs::create_directory(pluginpath + "triggers/");
        for (const Plugin::GlobalTrigger gt : plugin->data.globaltriggers) {
            std::ofstream trigger(pluginpath + "triggers/" + gt.name + ".txt");
            trigger << gt.name << "\n";
            trigger << gt.cancelable << "\n";
            trigger << gt.side;
            for (int i = 0; i < 12; i++)
                trigger << "\n" << gt.dependencies[i];
            for (const std::pair<std::string, std::string> version : gt.versions)
                trigger << "\n" << "[VERSION] " << version.first << " " << version.second;
            trigger << "\n[VERSION_END]";
            for (const std::pair<std::string, std::string> version : gt.versions) {
                trigger << "\n" + gt.event_code.at(version);
                for (int i = 0; i < 12; i++) {
                    if (gt.dependencies[i]) {
                        trigger << "\n" + gt.dependency_mappings.at(version).at(plugin->Dependencies[i]);
                    }
                }
            }
            trigger.close();
        }
    }
    else if (fs::exists(pluginpath + "triggers/"))
        fs::remove_all(pluginpath + "triggers/");

    if (!plugin->data.procedures.empty()) {
        if (!fs::exists(pluginpath + "procedures/"))
            fs::create_directory(pluginpath + "procedures/");
        for (const Plugin::Procedure pc : plugin->data.procedures) {
            std::ofstream procedure(pluginpath + "procedures/" + pc.name + ".txt");
            procedure << pc.name << "\n";
            procedure << pc.type << "\n";
            procedure << pc.color.x << " " << pc.color.y << " " << pc.color.z << " " << pc.color.w << "\n";
            procedure << pc.category_name << " [END]" << "\n";
            procedure << pc.category << "\n";
            procedure << pc.translationkey << "\n";
            procedure << pc.type_index << "\n";
            procedure << pc.components.size() << "\n";
            bool first = true;
            for (int i = 0; i < pc.components.size(); i++) {
                procedure << (first ? "" : " ") << pc.components[i].type_int;
                first = false;
            }
            for (const Plugin::Component comp : pc.components) {
                procedure << "\n" << comp.name;
                switch (comp.type_int) {
                case 0:
                    procedure << "\n" << comp.component_value;
                    procedure << "\n" << comp.value_int;
                    break;
                case 1:
                    break;
                case 2:
                    procedure << "\n" << comp.checkbox_checked;
                    break;
                case 3:
                    procedure << "\n" << comp.dropdown_options.size();
                    for (const std::string s : comp.dropdown_options)
                        procedure << "\n" << s;
                    break;
                case 4:
                    procedure << "\n" << comp.datalist;
                    break;
                case 5:
                    procedure << "\n" << comp.disable_localvars;
                    break;
                }
            }
            bool first__ = true;
            for (const std::pair<std::string, std::string> version : pc.versions) {
                procedure << (!first ? "\n" : "") << "[VERSION] " << version.first << " " << version.second;
                first = false;
            }
            procedure << "\n" << pc.world_dependency;
            if (!pc.code.empty()) {
                if (!fs::exists(pluginpath + "procedures/code/"))
                    fs::create_directory(pluginpath + "procedures/code/");
                for (const std::pair<std::string, std::string> version : pc.versions) {
                    std::string code = pc.code.at(version);
                    std::ofstream code_out(pluginpath + "procedures/code/" + pc.name + version.first + version.second + ".txt");
                    code_out << code;
                    code_out.close();
                }
            }
            procedure.close();
        }
    }
    else if (fs::exists(pluginpath + "procedures/"))
        fs::remove_all(pluginpath + "procedures/");

}
Plugin LoadPlugin(std::string path) {
    Plugin plugin;

    std::ifstream info(path + "info.txt"); // load plugin properties
    std::getline(info, plugin.data.name);
    std::getline(info, plugin.data.id);
    std::getline(info, plugin.data.version);
    std::getline(info, plugin.data.author);
    std::getline(info, plugin.data.description);
    std::getline(info, plugin.data.credits);
    info.close();

    std::ifstream filenames(path + "filenames.txt"); // load file names vector
    if (filenames.is_open()) {
        std::string temp;
        while (std::getline(filenames, temp))
            plugin.data.filenames.push_back(temp);
    }
    filenames.close();

    if (fs::exists(path + "categories/")) {
        for (const fs::path entry : fs::directory_iterator(path + "categories/")) { // load blockly categories
            std::ifstream category(entry.string());
            Plugin::Category ct;
            std::getline(category, ct.name);
            category >> ct.isapi;
            ImVec4 color;
            category >> color.x;
            category >> color.y;
            category >> color.z;
            category >> color.w;
            ct.color = color;
            category.close();
            plugin.data.categories.push_back(ct);
        }
    }

    if (fs::exists(path + "triggers/")) {
        for (const fs::path entry : fs::directory_iterator(path + "triggers/")) { // load global triggers
            std::ifstream trigger(entry.string());
            Plugin::GlobalTrigger gt;
            std::getline(trigger, gt.name);
            trigger >> gt.cancelable;
            trigger >> gt.side;
            for (int i = 0; i < 12; i++)
                trigger >> gt.dependencies[i];
            std::string line;
            std::string temp;
            while (std::getline(trigger, line) && line.find("[VERSION_END]") == std::string::npos) {
                if (line.find("[VERSION]") != std::string::npos) {
                    std::istringstream ss(line);
                    ss >> temp;
                    std::pair<std::string, std::string> pair;
                    ss >> pair.first;
                    ss >> pair.second;
                    gt.versions.push_back(pair);
                }
            }
            for (std::pair<std::string, std::string> version : gt.versions) {
                std::getline(trigger, gt.event_code[version]);
                for (int i = 0; i < 12; i++) {
                    if (gt.dependencies[i]) {
                        std::getline(trigger, gt.dependency_mappings[version][plugin.Dependencies[i]]);
                    }
                }
            }
            trigger.close();
            plugin.data.globaltriggers.push_back(gt);
        }
    }

    if (fs::exists(path + "procedures/")) {
        for (const fs::path entry : fs::directory_iterator(path + "procedures/")) { // load procedure blocks
            if (fs::is_regular_file(entry)) {
                std::ifstream procedure(entry.string());
                Plugin::Procedure pc;
                std::getline(procedure, pc.name);
                procedure >> pc.type;
                ImVec4 color;
                int options = 0;
                procedure >> color.x;
                procedure >> color.y;
                procedure >> color.z;
                procedure >> color.w;
                pc.color = color;
                bool first = true;
                while (true) { // for some damn reason std::getline doesn't want to work
                    std::string s;
                    procedure >> s;
                    if (s != "[END]") {
                        pc.category_name += (first ? "" : " ") + s;
                    }
                    else
                        break;
                    first = false;
                }
                procedure >> pc.category;
                procedure >> pc.translationkey;
                std::string temp;
                std::getline(procedure, temp);
                pc.translationkey.append(temp);
                procedure >> pc.type_index;
                int component_count = 0;
                std::vector<int> component_types;
                procedure >> component_count;
                for (int i = 0; i < component_count; i++) {
                    int type;
                    procedure >> type;
                    component_types.push_back(type);
                }
                for (const int pctype : component_types) {
                    Plugin::Component comp;
                    comp.type_int = pctype;
                    procedure >> comp.name;
                    std::getline(procedure, temp);
                    comp.name.append(temp);
                    temp.clear();
                    switch (pctype) {
                    case 0:
                        procedure >> comp.component_value;
                        procedure >> comp.value_int;
                        break;
                    case 1:
                        break;
                    case 2:
                        procedure >> comp.checkbox_checked;
                        break;
                    case 3:
                        options = 0;
                        procedure >> options;
                        for (int i = 0; i < options; i++) {
                            procedure >> temp;
                            comp.dropdown_options.push_back(temp);
                            std::getline(procedure, temp);
                            comp.dropdown_options[i].append(temp);
                            temp.clear();
                        }
                        break;
                    case 4:
                        procedure >> comp.datalist;
                        std::getline(procedure, temp);
                        comp.datalist.append(temp);
                        temp.clear();
                        break;
                    case 5:
                        procedure >> comp.disable_localvars;
                        break;
                    }
                    pc.components.push_back(comp);
                }
                std::string rtemp;
                std::string line_;
                while (std::getline(procedure, line_)) {
                    if (line_.find("[VERSION]") != std::string::npos) {
                        std::istringstream ss(line_);
                        ss >> rtemp;
                        std::pair<std::string, std::string> pair;
                        ss >> pair.first;
                        ss >> pair.second;
                        pc.versions.push_back(pair);
                        line_.clear();
                        rtemp.clear();
                    }
                    else if (line_.size() == 1)
                        pc.world_dependency = (line_ == "1" ? true : false);
                }
                if (fs::exists(path + "procedures/code/")) {
                    for (const std::pair<std::string, std::string> version : pc.versions) {
                        std::ifstream code_in(path + "procedures/code/" + pc.name + version.first + version.second + ".txt");
                        bool first_ = true;
                        bool blank = true;
                        std::string temp;
                        std::string code_ = "";
                        while (std::getline(code_in, temp)) {
                            code_ += temp + "\n";
                            first = false;
                        }
                        pc.code[version] = code_;
                        code_.clear();
                        temp.clear();
                        code_in.close();
                    }
                }
                procedure.close();
                plugin.data.procedures.push_back(pc);
            }
        }
    }

    return plugin;
}
void LoadAllPlugins() {
    if (DirectoryExists("plugins/")) {
        for (const fs::path entry : fs::directory_iterator("plugins/")) {
            if (fs::is_directory(entry))
                plugins.push_back(LoadPlugin(entry.string() + "\\"));
        }
    }
    code_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Json());
}
int IndexOf(const std::vector<std::string> data_, const std::string& element) {
    auto it = std::find(data_.begin(), data_.end(), element);
    return (it == data_.end() ? -1 : std::distance(data_.begin(), it));
}
std::string ImVecToHex(ImVec4 rgb) {
    rgb.x *= 255;
    rgb.y *= 255;
    rgb.z *= 255;
    std::stringstream ss;
    ss << "#";
    ss << std::hex << ((int)rgb.x << 16 | (int)rgb.y << 8 | (int)rgb.z);
    return ss.str();
}
std::string RegistryName(std::string str) {
    std::string newstr = "";
    for (int i = 0; i < str.size(); i++) {
        if (std::isupper(str[i]))
            newstr += std::tolower(str[i]);
        else if (std::isspace(str[i]))
            newstr += "_";
        else if (std::islower(str[i]))
            newstr += str[i];
        else
            newstr += str[i];
    }
    return newstr;
}
std::string LowerStr(std::string str) {
    std::string newstr = "";
    for (int i = 0; i < str.size(); i++)
        newstr += std::tolower(str[i]);
    return newstr;
}
void ExportPlugin(const Plugin plugin) {
    char path[1024] = { 0 };
    int result = GuiFileDialog(DIALOG_SAVE_FILE, "Export plugin", path, "*.zip", "MCreator plugin (*.zip)");
    if (result == 1) {
        fs::create_directory("temp_data");
        std::ofstream lang("temp_data\\texts.properties");
        std::ofstream pjson("temp_data\\plugin.json");
        Zip zip = Zip::Create((std::string)path);
        zip.AddFolder("lang");
        ////////////////////////
        pjson << "{\n";
        pjson << "  \"id\": \"" + RegistryName(plugin.data.id) + "\",\n";
        pjson << "  \"weight\": 30,\n";
        pjson << "  \"minversion\": 0,\n";
        pjson << "  \"info\": {\n";
        pjson << "    \"name\": \"" + plugin.data.name + "\",\n";
        pjson << "    \"version\": \"" + plugin.data.version + "\",\n";
        pjson << "    \"description\": \"" + plugin.data.description + "\",\n";
        pjson << "    \"author\": \"" + plugin.data.author + "\",\n";
        pjson << "    \"credits\": \"" + plugin.data.credits + "\"\n";
        pjson << "  }\n";
        pjson << "}";
        pjson.close();
        zip.AddFile("temp_data\\plugin.json", "plugin.json");
        //////////////////////// 
        if (!plugin.data.categories.empty() || !plugin.data.procedures.empty()) {
            zip.AddFolder("procedures");
            if (!plugin.data.categories.empty()) {
                for (const Plugin::Category ct : plugin.data.categories) {
                    std::ofstream ct_("temp_data\\" + RegistryName(ct.name) + ".json");
                    ct_ << "{\n";
                    ct_ << "  \"name\": \"" + ct.name + "\",\n";
                    ct_ << "  \"color\": \"" + ImVecToHex(ct.color) + "\",\n";
                    ct_ << "  \"api\": " + (std::string)(ct.isapi ? "true\n" : "false\n");
                    ct_ << "}";
                    ct_.close();
                    zip.AddFile("temp_data\\" + RegistryName(ct.name) + ".json", "procedures\\$" + RegistryName(ct.name) + ".json");
                    lang << "blockly.category." + RegistryName(ct.name) + "=" + ct.name + "\n";
                }
            }
            if (!plugin.data.procedures.empty()) {
                for (const Plugin::Procedure pc : plugin.data.procedures) {
                    std::ofstream pc_("temp_data\\" + RegistryName(pc.name) + ".json");
                    pc_ << "{\n";
                    pc_ << "  \"args0\": [\n";
                    int i = 0;
                    for (const Plugin::Component comp : pc.components) {
                        i++;
                        pc_ << "    {\n";
                        std::string type_;
                        switch (comp.type_int) {
                        case 0:
                            pc_ << "      \"type\": \"input_value\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\",\n";
                            pc_ << "      \"check\": \"";
                            if (comp.component_value == "Block")
                                type_ = "MCItemBlock";
                            else if (comp.component_value == "Item")
                                type_ = "MCItem";
                            else if (comp.component_value == "Text")
                                type_ = "String";
                            else
                                type_ = comp.component_value;
                            pc_ << type_ << "\"\n";
                            break;
                        case 1:
                            pc_ << "      \"type\": \"field_input\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\"\n";
                            break;
                        case 2:
                            pc_ << "      \"type\": \"field_checkbox\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\",\n";
                            pc_ << "      \"checked\": " + (std::string)(comp.checkbox_checked ? "true\n" : "false\n");
                            break;
                        case 3:
                            pc_ << "      \"type\": \"field_dropdown\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\",\n";
                            pc_ << "      \"options\": [\n";
                            for (int j = 0; j < comp.dropdown_options.size(); j++) {
                                pc_ << "        [\n";
                                pc_ << "          \"" + comp.dropdown_options[j] + "\",\n";
                                pc_ << "          \"" + comp.dropdown_options[j] + "\"\n";
                                pc_ << "        ]" + (std::string)(j == comp.dropdown_options.size() - 1 ? "\n" : ",\n");
                            }
                            pc_ << "      ]\n";
                            break;
                        case 4:
                            pc_ << "      \"type\": \"field_data_list_selector\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\",\n";
                            pc_ << "      \"datalist\": \"" + comp.datalist + "\"\n";
                            break;
                        case 5:
                            pc_ << "      \"type\": \"input_statement\",\n";
                            pc_ << "      \"name\": \"" + comp.name + "\"\n";
                            break;
                        }
                        pc_ << "    }" + (std::string)(i == pc.components.size() ? "\n" : ",\n");
                    }
                    pc_ << "  ],\n";
                    pc_ << "  \"inputsInLine\": true,\n";
                    if (pc.type == 0) { // output
                        pc_ << "  \"previousStatement\": null,\n";
                        pc_ << "  \"nextStatement\": null,\n";
                    }
                    else { // input
                        pc_ << "  \"output\": \"";
                        std::string type__;
                        if (plugin.ComponentValues[pc.type_index] == "Block")
                            type__ = "MCItemBlock";
                        else if (plugin.ComponentValues[pc.type_index] == "Item")
                            type__ = "MCItem";
                        else if (plugin.ComponentValues[pc.type_index] == "Text")
                            type__ = "String";
                        else
                            type__ = plugin.ComponentValues[pc.type_index];
                        pc_ << type__ << "\",\n";
                    }
                    pc_ << "  \"colour\": \"" + ImVecToHex(pc.color) + "\",\n";
                    pc_ << "  \"mcreator\": {\n";
                    pc_ << "    \"toolbox_id\": \"";
                    if (pc.category <= 16) {
                        if (pc.category_name == "Block data")
                            pc_ << "blockdata";
                        else if (pc.category_name == "Block management")
                            pc_ << "blockactions";
                        else if (pc.category_name == "Command parameters")
                            pc_ << "commands";
                        else if (pc.category_name == "Direction procedures")
                            pc_ << "directionactions";
                        else if (pc.category_name == "Energy & fluid tanks")
                            pc_ << "energyandfluid";
                        else if (pc.category_name == "Entity data")
                            pc_ << "entitydata";
                        else if (pc.category_name == "Entity management")
                            pc_ << "entitymanagement";
                        else if (pc.category_name == "Item procedures")
                            pc_ << "itemmanagement";
                        else if (pc.category_name == "Player data")
                            pc_ << "playerdata";
                        else if (pc.category_name == "Player procedures")
                            pc_ << "playermanagement";
                        else if (pc.category_name == "Projectile procedures")
                            pc_ << "projectilemanagement";
                        else if (pc.category_name == "Slot & GUI procedures")
                            pc_ << "guimanagement";
                        else if (pc.category_name == "World data")
                            pc_ << "worlddata";
                        else if (pc.category_name == "World management")
                            pc_ << "worldmanagement";
                        else if (pc.category_name == "Minecraft components")
                            pc_ << "mcelements";
                        else if (pc.category_name == "Flow control")
                            pc_ << "logicloops";
                        else if (pc.category_name == "Advanced")
                            pc_ << "advanced";
                    }
                    else
                        pc_ << RegistryName(pc.category_name);
                    pc_ << "\",\n";
                    pc_ << "    \"toolbox_init\": [\n";
                    i = 0;
                    for (const Plugin::Component comp : pc.components) {
                        i++;
                        if (comp.component_value == "Number" && (comp.name == "X" || comp.name == "Y" || comp.name == "Z" || comp.name == "x" || comp.name == "y" || comp.name == "z"))
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "coord_" << LowerStr(comp.name) << "\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Number")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "math_number\\" << '"' << ">" << "<field name=" << "\\" << '"' << "NUM\\" << '"' << ">0</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Direction")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "direction_unspecified\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Boolean")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "logic_boolean\\" << '"' << ">" << "<field name=" << "\\" << '"' << "BOOL\\" << '"' << ">TRUE</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Text")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "text\\" << '"' << ">" << "<field name=" << "\\" << '"' << "TEXT\\" << '"' << ">text</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Block")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "mcitem_allblocks\\" << '"' << ">" << "<field name=" << "\\" << '"' << "value\\" << '"' << "></field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Item")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "mcitem_all\\" << '"' << ">" << "<field name=" << "\\" << '"' << "value\\" << '"' << "></field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Entity")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "entity_from_deps\\" << '"' << "></block></value>" << '"';
                        pc_ << (std::string)(i == pc.components.size() ? "\n" : ",\n");
                    }
                    pc_ << "    ]" + (std::string)(!pc.components.empty() || pc.world_dependency ? ",\n" : "\n");
                    if (!pc.components.empty()) {
                        i = 0;
                        std::vector<std::string> inputs;
                        std::vector<std::string> fields;
                        std::vector<std::string> statements;
                        std::vector<bool> disable_locals;
                        for (const Plugin::Component comp : pc.components) {
                            switch (comp.type_int) {
                            case 0:
                                inputs.push_back(comp.name);
                                break;
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                                fields.push_back(comp.name);
                                break;
                            case 5:
                                statements.push_back(comp.name);
                                disable_locals.push_back(comp.disable_localvars);
                                break;
                            }
                        }
                        if (!inputs.empty()) {
                            pc_ << "    \"inputs\": [\n";
                            for (const std::string s : inputs) {
                                i++;
                                pc_ << "      \"" + s + "\"" + (std::string)(i == inputs.size() ? "\n" : ",\n");
                            }
                            pc_ << "    ]" + (std::string)(!fields.empty() || pc.world_dependency ? ",\n" : "\n");
                            i = 0;
                        }
                        if (!fields.empty()) {
                            pc_ << "    \"fields\": [\n";
                            for (const std::string s : fields) {
                                i++;
                                pc_ << "      \"" + s + "\"" + (std::string)(i == fields.size() ? "\n" : ",\n");
                            }
                            pc_ << "    ]" + (std::string)(!statements.empty() || pc.world_dependency ? ",\n" : "\n");
                            i = 0;
                        }
                        if (!statements.empty()) {
                            pc_ << "    \"statements\": [\n";
                            for (const std::string s : statements) {
                                i++;
                                pc_ << "      {\n";
                                pc_ << "        \"name\": \"" + s + "\",\n";
                                pc_ << "        \"disable_local_variables\": " + (std::string)(disable_locals[i - 1] ? "true\n" : "false\n");
                                pc_ << "      }\n";
                            }
                            pc_ << "    ]" + (std::string)(pc.world_dependency ? ",\n" : "\n");
                            i = 0;
                        }
                    }
                    if (pc.world_dependency) {
                        pc_ << "    \"dependencies\": [\n";
                        pc_ << "      {\n";
                        pc_ << "        \"name\": \"world\",\n";
                        pc_ << "        \"type\": \"world\"\n";
                        pc_ << "      }\n";
                        pc_ << "    ]\n";
                    }
                    pc_ << "  }\n";
                    pc_ << "}";
                    pc_.close();
                    zip.AddFile("temp_data\\" + RegistryName(pc.name) + ".json", "procedures\\" + RegistryName(pc.name) + ".json");
                    lang << "blockly.block." + RegistryName(pc.name) + "=" + pc.translationkey + "\n";
                }
            }
        }
        if (!plugin.data.globaltriggers.empty()) {
            zip.AddFolder("triggers");
            for (const Plugin::GlobalTrigger gt : plugin.data.globaltriggers) {
                std::ofstream gt_("temp_data\\" + gt.name + ".json");
                gt_ << "{\n";
                gt_ << "  \"dependencies_provided\": [\n";
                std::vector<int> deps;
                for (int i = 0; i < 12; i++) {
                    if (gt.dependencies[i])
                        deps.push_back(i);
                }
                for (int i = 0; i < deps.size(); i++) {
                    switch (deps[i]) {
                    case 0:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"x\",\n";
                        gt_ << "      \"type\": \"number\"\n";
                        gt_ << "    }";
                        break;
                    case 1:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"y\",\n";
                        gt_ << "      \"type\": \"number\"\n";
                        gt_ << "    }";
                        break;
                    case 2:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"z\",\n";
                        gt_ << "      \"type\": \"number\"\n";
                        gt_ << "    }";
                        break;
                    case 3:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"entity\",\n";
                        gt_ << "      \"type\": \"entity\"\n";
                        gt_ << "    }";
                        break;
                    case 4:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"sourceentity\",\n";
                        gt_ << "      \"type\": \"entity\"\n";
                        gt_ << "    }";
                        break;
                    case 5:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"immediatesourceentity\",\n";
                        gt_ << "      \"type\": \"entity\"\n";
                        gt_ << "    }";
                        break;
                    case 6:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"world\",\n";
                        gt_ << "      \"type\": \"world\"\n";
                        gt_ << "    }";
                        break;
                    case 7:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"itemstack\",\n";
                        gt_ << "      \"type\": \"itemstack\"\n";
                        gt_ << "    }";
                        break;
                    case 8:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"blockstate\",\n";
                        gt_ << "      \"type\": \"blockstate\"\n";
                        gt_ << "    }";
                        break;
                    case 9:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"direction\",\n";
                        gt_ << "      \"type\": \"direction\"\n";
                        gt_ << "    }";
                        break;
                    case 10:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"advancement\",\n";
                        gt_ << "      \"type\": \"advancement\"\n";
                        gt_ << "    }";
                        break;
                    case 11:
                        gt_ << "    {\n";
                        gt_ << "      \"name\": \"dimension\",\n";
                        gt_ << "      \"type\": \"dimension\"\n";
                        gt_ << "    }";
                        break;
                    }
                    if (i == deps.size() - 1)
                        gt_ << "\n";
                    else
                        gt_ << ",\n";
                }
                gt_ << "  ],\n";
                gt_ << "  \"cancelable\": " + (std::string)(gt.cancelable ? "true,\n" : "false,\n");
                gt_ << "  \"side\": \"";
                switch (gt.side) {
                case 0:
                    gt_ << "server";
                    break;
                case 1:
                    gt_ << "client";
                    break;
                case 2:
                    gt_ << "both";
                    break;
                }
                gt_ << "\"\n";
                gt_ << "}";
                gt_.close();
                zip.AddFile("temp_data\\" + gt.name + ".json", "triggers\\" + RegistryName(gt.name) + ".json");
                lang << "trigger." + RegistryName(gt.name) + "=" + gt.name + "\n";
            }
        }
        lang.close();
        zip.AddFile("temp_data\\texts.properties", "lang\\texts.properties");
        std::vector<std::string> versions;
        std::vector<std::string> triggerversions;
        std::vector<std::string> procedureversions;
        if (!plugin.data.procedures.empty()) {
            for (const Plugin::Procedure pc : plugin.data.procedures) {
                if (!pc.versions.empty()) {
                    for (const std::pair<std::string, std::string> version : pc.versions) {
                        std::string version_str = RegistryName(version.second) + "-" + version.first;
                        if (std::find(versions.begin(), versions.end(), version_str) == versions.end())
                            versions.push_back(version_str);
                        procedureversions.push_back(version_str);
                    }
                }
            }
        }
        if (!plugin.data.globaltriggers.empty()) {
            for (const Plugin::GlobalTrigger gt : plugin.data.globaltriggers) {
                if (!gt.versions.empty()) {
                    for (const std::pair<std::string, std::string> version : gt.versions) {
                        std::string version_str = RegistryName(version.second) + "-" + version.first;
                        if (std::find(versions.begin(), versions.end(), version_str) == versions.end())
                            versions.push_back(version_str);
                        triggerversions.push_back(version_str);
                    }
                }
            }
        }
        for (const std::string vers : versions) {
            zip.AddFolder(vers);
            if (std::find(procedureversions.begin(), procedureversions.end(), vers) != procedureversions.end())
                zip.AddFolder(vers + "\\procedures");
            if (std::find(triggerversions.begin(), triggerversions.end(), vers) != triggerversions.end())
                zip.AddFolder(vers + "\\triggers");
        }
        if (!plugin.data.procedures.empty()) {
            for (const Plugin::Procedure pc : plugin.data.procedures) {
                for (const std::pair<std::string, std::string> version : pc.versions) {
                    std::string foldername = RegistryName(version.second) + "-" + version.first;
                    zip.AddFile("plugins\\" + plugin.data.name + "\\procedures\\code\\" + pc.name + version.first + version.second + ".txt", foldername + "\\procedures\\" + RegistryName(pc.name) + ".java.ftl");
                }
            }
        }
        if (!plugin.data.globaltriggers.empty()) {
            for (const Plugin::GlobalTrigger gt : plugin.data.globaltriggers) {
                for (const std::pair<std::string, std::string> version : gt.versions) {
                    std::string foldername = RegistryName(version.second) + "-" + version.first;
                    std::ofstream gt_("temp_data\\" + gt.name + version.first + version.second + ".txt");
                    gt_ << "<#include \"procedures.java.ftl\">\n";
                    gt_ << "@Mod.EventBusSubscriber\n";
                    gt_ << "public class ${name}Procedure {\n";
                    gt_ << "    @SubscribeEvent\n";
                    gt_ << "    public static void(" + gt.event_code.at(version) + " event) {\n";
                    gt_ << "        <#assign dependenciesCode><#compress>\n";
                    if (HasDependencies(gt.dependencies)) {
                        gt_ << "            <@procedureDependenciesCode dependencies, {\n";
                        std::vector<int> deps;
                        for (int i = 0; i < 12; i++) {
                            if (gt.dependencies[i])
                                deps.push_back(i);
                        }
                        for (int i = 0; i < deps.size(); i++) {
                            gt_ << "            \"" + LowerStr(plugin.Dependencies[deps[i]]) + "\": \"event." + gt.dependency_mappings.at(version).at(plugin.Dependencies[deps[i]]) + "\",\n";
                        }
                        gt_ << "            \"event\": \"event\"\n";
                        gt_ << "            }/>\n";
                    }
                    gt_ << "        </#compress></#assign>\n";
                    gt_ << "        execute(event<#if dependenciesCode?has_content>,</#if>${dependenciesCode});\n";
                    gt_ << "    }\n";
                    gt_ << "}";
                    gt_.close();
                    zip.AddFile("temp_data\\" + gt.name + version.first + version.second + ".txt", foldername + "\\triggers\\" + RegistryName(gt.name) + ".java.ftl");
                }
            }
        }
        zip.Close();
        fs::remove_all("temp_data\\");
    }
    else {
        GuiFileDialog(DIALOG_MESSAGE, "Exporting failed", nullptr, nullptr, "Failed to export plugin!");
    }
}

// tab windows stuff
enum WindowType {
    PROCEDURE, GLOBALTRIGGER, CATEGORY
};
struct TabWindow {
    WindowType type;
    bool open = true;
    Plugin::Procedure* procedure;
    Plugin::GlobalTrigger* globaltrigger;
    Plugin::Category* category;
};
std::vector<TabWindow> open_tabs;
std::vector<std::string> open_tab_names;
int current_tab = -1;
int old_current_tab = -1;
int current_version = -1;
int old_current_version = -1;
WindowType versiontype;

int main() {

    InitWindow(1200, 600, "Plugin Builder");
    SetWindowMinSize(600, 300);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    rlImGuiSetup(true);
    SetGuiTheme();
    LoadAllPlugins();

    Image icon = LoadImage("plugin_icon.png");
    Texture PluginIcon = LoadTexture("plugin.png");

    SetWindowIcon(icon);

    int selected_plugin = -1;

    while (!WindowShouldClose()) {

        ClearBackground(BLACK);
        rlImGuiBegin();

        if (mainmenu) {
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize({ (float)GetScreenWidth() - 250, (float)GetScreenHeight() });
            if (ImGui::Begin("plugins", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize)) {
                ImGui::BeginChild(1);
                int i = 0;
                if (!plugins.empty()) {
                    for (Plugin& plugin : plugins) {
                        rlImGuiImageSize(&PluginIcon, 65, 65);
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 17.5);
                        ImGui::SetWindowFontScale(2);
                        ImGui::Text(plugin.data.name.c_str());
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 45);
                        ImGui::SetWindowFontScale(1);
                        ImGui::Text(plugin.data.version.c_str());
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(0);
                        if (ImGui::InvisibleButton(std::to_string(i).c_str(), { ImGui::GetColumnWidth(), 65 })) {
                            selected_plugin = i;
                        }
                        DrawRectangle(ImGui::GetWindowPos().x + ImGui::GetCursorPosX(), ImGui::GetWindowPos().y - ImGui::GetScrollY() + ImGui::GetCursorPosY() - 70, ImGui::GetColumnWidth(), 65, GRAY);
                        if (ImGui::IsItemHovered() || selected_plugin == i)
                            DrawRectangle(ImGui::GetWindowPos().x + ImGui::GetCursorPosX(), ImGui::GetWindowPos().y - ImGui::GetScrollY() + ImGui::GetCursorPosY() - 70, ImGui::GetColumnWidth(), 65, (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ImGui::IsItemHovered() ? BLACK : DARKGRAY));
                        DrawRectangleLines(ImGui::GetWindowPos().x + ImGui::GetCursorPosX(), ImGui::GetWindowPos().y - ImGui::GetScrollY() + ImGui::GetCursorPosY() - 70, ImGui::GetColumnWidth(), 65, WHITE);
                        i++;
                    }
                }
                else {
                    ImGui::SetWindowFontScale(3);
                    ImGui::SetCursorPosY((float)(GetScreenHeight() - 100) / 2);
                    ImGui::SetCursorPosX((float)(GetScreenWidth() - 650 ) / 2);
                    ImGui::Text("No plugin projects");
                }
                ImGui::EndChild();
                ImGui::End();
            }

            ImGui::SetNextWindowPos({ (float)GetScreenWidth() - 251, 0 });
            ImGui::SetNextWindowSize({ 251, (float)GetScreenHeight() });
            if (ImGui::Begin("pluginoptions", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize)) {
                ImGui::SetWindowFontScale(1.9);
                if (ImGui::Button("Create new", { ImGui::GetColumnWidth(), 50 })) {
                    selected_plugin = -1;
                    creatingplugin = true;
                }
                if (ImGui::Button("Open", { ImGui::GetColumnWidth(), 50 }))
                    if (selected_plugin != -1) {
                        mainmenu = false;
                        loaded_plugin = plugins[selected_plugin];
                        selected_plugin = -1;
                        pluginmenu = true;
                        MaximizeWindow();
                    }
                if (ImGui::Button("Delete", { ImGui::GetColumnWidth(), 50 }))
                    if (selected_plugin != -1) {
                        delete_confirm = true;
                    }
                ImGui::SetWindowFontScale(1);
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::End();
            }

            if (delete_confirm) {
                ImGui::OpenPopup("Are you sure?");
                if (delete_set_pos)
                    ImGui::SetNextWindowPos({ ((float)GetScreenWidth() - 350) / 2, ((float)GetScreenHeight() - 125) / 2 });
                delete_set_pos = false;
                ImGui::SetNextWindowSize({ 350, 125 });
                if (ImGui::BeginPopupModal("Are you sure?", &delete_confirm, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
                    ImGui::NewLine();
                    ImGui::SetCursorPosX(20);
                    ImGui::Text("Are you sure you want to delete this plugin?");
                    ImGui::NewLine();
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(90);
                    if (ImGui::Button("Yes", { 75, 30 })) {
                        fs::remove_all("plugins/" + plugins[selected_plugin].data.name + "/");
                        plugins.erase(plugins.begin() + selected_plugin);
                        delete_confirm = false;
                        selected_plugin = -1;
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(185);
                    if (ImGui::Button("No", { 75, 30 })) {
                        delete_confirm = false;
                    }
                    ImGui::End();
                }
            }
            else
                delete_set_pos = true;
        }

        if (creatingplugin) {
            ImGui::OpenPopup("Plugin properties");
            if (creatingplugin_set_pos) {
                ImGui::SetNextWindowPos({ ((float)GetScreenWidth() - 350) / 2, ((float)GetScreenHeight() - 200) / 2 });
                if (!mainmenu) {
                    pluginname = loaded_plugin.data.name;
                    pluginid = loaded_plugin.data.id;
                    pluginversion = loaded_plugin.data.version;
                    pluginauthor = loaded_plugin.data.author;
                    plugindescription = loaded_plugin.data.description;
                }
            }
            creatingplugin_set_pos = false;
            ImGui::SetNextWindowSize({ 350, 200 });
            if (ImGui::BeginPopupModal("Plugin properties", &creatingplugin, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Plugin Name: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::InputText(" ", &pluginname);
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Plugin ID: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::PushID(1);
                ImGui::InputText(" ", &pluginid, ImGuiInputTextFlags_CharsNoBlank);
                ImGui::PopID();
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Plugin Version: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::PushID(2);
                ImGui::InputText(" ", &pluginversion);
                ImGui::PopID();
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Plugin Author: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::PushID(3);
                ImGui::InputText(" ", &pluginauthor);
                ImGui::PopID();
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Plugin Description: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::PushID(4);
                ImGui::InputText(" ", &plugindescription);
                ImGui::PopID();
                ImGui::Spacing();

                ImGui::SetCursorPosX(100);
                if (ImGui::Button((mainmenu ? "Create Plugin" : "Save Changes"), {150, 30})) {
                    if (!IsPluginFieldEmpty()) {
                        if (mainmenu) {
                            Plugin plugin;
                            plugin.data.name = pluginname;
                            plugin.data.id = pluginid;
                            plugin.data.version = pluginversion;
                            plugin.data.author = (pluginauthor.empty() ? "Unknown" : pluginauthor);
                            plugin.data.description = (plugindescription.empty() ? "Unknown" : plugindescription);
                            plugin.data.credits = "Made with MCreator plugin builder by NerdyPuzzle";
                            SavePlugin(&plugin);
                            ClearPluginVars();
                            plugins.push_back(plugin);
                            creatingplugin = false;
                        }
                        else {
                            loaded_plugin.data.name = pluginname;
                            loaded_plugin.data.id = pluginid;
                            loaded_plugin.data.version = pluginversion;
                            loaded_plugin.data.author = (pluginauthor.empty() ? "Unknown" : pluginauthor);
                            loaded_plugin.data.description = (plugindescription.empty() ? "Unknown" : plugindescription);
                            SavePlugin(&loaded_plugin);
                            ClearPluginVars();
                            creatingplugin = false;
                        }
                    }
                }
                
                ImGui::End();
            }
        }
        else {
            creatingplugin_set_pos = true;
            if (!IsPluginFieldEmpty())
                ClearPluginVars();
        }

        if (pluginmenu) {
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Project")) {
                        creatingplugin = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Add file")) {
                        addfile = true;
                    }
                    if (ImGui::MenuItem("Delete file")) {
                        deletefile = true;
                    }
                    if (ImGui::MenuItem("Save")) {
                        SavePlugin(&loaded_plugin);
                    }
                    if (ImGui::MenuItem("Export")) {
                        ExportPlugin(loaded_plugin);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Close")) {
                        SavePlugin(&loaded_plugin);
                        pluginmenu = false;
                        open_tabs.clear();
                        open_tab_names.clear();
                        mainmenu = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            } // implement menu bar later

            if (deletefile) {
                ImGui::OpenPopup("Delete file");
                if (deletefile_set_pos) {
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 177) / 2 });
                    for (int j = 0; j < loaded_plugin.data.procedures.size(); j++) {
                        tempchars_procedures.push_back(loaded_plugin.data.procedures[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.globaltriggers.size(); j++) {
                        tempchars_globaltriggers.push_back(loaded_plugin.data.globaltriggers[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.categories.size(); j++) {
                        tempchars_categories.push_back(loaded_plugin.data.categories[j].name.c_str());
                    }
                }
                deletefile_set_pos = false;
                ImGui::SetNextWindowSize({ 300, 177 });
                if (ImGui::BeginPopupModal("Delete file", &deletefile, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
                    if (delete_procedure != -1 && !active[0]) {
                        delete_globaltrigger = -1;
                        delete_category = -1;
                        active[0] = true;
                        active[1] = false;
                        active[2] = false;
                    }
                    else if (delete_globaltrigger != -1 && !active[1]) {
                        delete_procedure = -1;
                        delete_category = -1;
                        active[1] = true;
                        active[0] = false;
                        active[2] = false;
                    }
                    else if (delete_category != -1 && !active[2]) {
                        delete_procedure = -1;
                        delete_globaltrigger = -1;
                        active[2] = true;
                        active[1] = false;
                        active[0] = false;
                    }

                    ImGui::Text("Procedures");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("wsgawsg", &delete_procedure, tempchars_procedures.data(), loaded_plugin.data.procedures.size());
                    ImGui::Text("Global Triggers");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("hawsh", &delete_globaltrigger, tempchars_globaltriggers.data(), loaded_plugin.data.globaltriggers.size());
                    ImGui::Text("Blockly Categories");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("ejse", &delete_category, tempchars_categories.data(), loaded_plugin.data.categories.size());

                    if (delete_procedure != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.procedures[delete_procedure].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }
                    else if (delete_globaltrigger != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.globaltriggers[delete_globaltrigger].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }
                    else if (delete_category != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.categories[delete_category].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }

                    ImGui::SetCursorPosX(100);
                    if (ImGui::Button("Delete", { 100, 30 })) {
                        if (delete_procedure != -1 && !file_open) {
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.procedures[delete_procedure].name));
                            loaded_plugin.data.procedures.erase(loaded_plugin.data.procedures.begin() + delete_procedure);
                            tempchars_procedures.erase(tempchars_procedures.begin() + delete_procedure);
                        }
                        else if (delete_globaltrigger != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/triggers/" + loaded_plugin.data.globaltriggers[delete_globaltrigger].name + ".txt");
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.globaltriggers[delete_globaltrigger].name));
                            loaded_plugin.data.globaltriggers.erase(loaded_plugin.data.globaltriggers.begin() + delete_globaltrigger);
                            tempchars_globaltriggers.erase(tempchars_globaltriggers.begin() + delete_globaltrigger);
                        }
                        else if (delete_category != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/categories/" + loaded_plugin.data.categories[delete_category].name + ".txt");
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.categories[delete_category].name));
                            loaded_plugin.data.categories.erase(loaded_plugin.data.categories.begin() + delete_category);
                            tempchars_categories.erase(tempchars_categories.begin() + delete_category);
                        }
                        if (delete_procedure != -1 || delete_globaltrigger != -1 || delete_category != -1 && !file_open) {
                            SavePlugin(&loaded_plugin);
                            deletefile = false;
                        }
                    }

                    if (file_open && ImGui::IsItemHovered()) {
                        if (ImGui::BeginTooltip()) {
                            ImGui::Text("Cannot delete file while still open in a tab!");
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::End();
                }
            }
            else {
                deletefile_set_pos = true;
                delete_procedure = -1;
                delete_globaltrigger = -1;
                delete_category = -1;
                active[0] = false; active[1] = false; active[2] = false;
                if (!tempchars_procedures.empty())
                    tempchars_procedures.clear();
                if (!tempchars_globaltriggers.empty())
                    tempchars_globaltriggers.clear();
                if (!tempchars_categories.empty())
                    tempchars_categories.clear();
                if (file_open)
                    file_open = false;
            }

            if (addfile) {
                if (addfile_set_pos) {
                    if (!ImGui::IsPopupOpen("New file"))
                        ImGui::OpenPopup("New file");
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 120) / 2 });
                }
                addfile_set_pos = false;
                ImGui::SetNextWindowSize({ 300, 120 }, ImGuiCond_Once);
                if (ImGui::BeginPopupModal("New file", &addfile, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::Combo(" ", &combo_item, "Procedure\0Global Trigger\0Blockly Category");
                    ImGui::Spacing();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Name: "); ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::PushID(1);
                    ImGui::InputText(" ", &file_name);
                    ImGui::PopID();
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(100);
                    bool should_add = std::find(loaded_plugin.data.filenames.begin(), loaded_plugin.data.filenames.end(), file_name) == loaded_plugin.data.filenames.end();
                    if (ImGui::Button("Add", { 100, 30 })) {
                        if (should_add && !file_name.empty()) {
                            TabWindow window;
                            if (combo_item == 0) {
                                window.type = PROCEDURE;
                                Plugin::Procedure procedure;
                                procedure.name = file_name;
                                procedure.color = rlImGuiColors::Convert(WHITE);
                                procedure.translationkey = "";
                                loaded_plugin.data.procedures.push_back(procedure);
                                window.procedure = &loaded_plugin.data.procedures[loaded_plugin.data.procedures.size() - 1];
                            }
                            else if (combo_item == 1) {
                                window.type = GLOBALTRIGGER;
                                Plugin::GlobalTrigger trigger;
                                trigger.name = file_name;
                                loaded_plugin.data.globaltriggers.push_back(trigger);
                                window.globaltrigger = &loaded_plugin.data.globaltriggers[loaded_plugin.data.globaltriggers.size() - 1];
                            }
                            else {
                                window.type = CATEGORY;
                                Plugin::Category category;
                                category.name = file_name;
                                category.color = rlImGuiColors::Convert(WHITE);
                                category.isapi = false;
                                loaded_plugin.data.categories.push_back(category);
                                window.category = &loaded_plugin.data.categories[loaded_plugin.data.categories.size() - 1];
                            }
                            open_tabs.push_back(window);
                            open_tab_names.push_back(file_name);
                            loaded_plugin.data.filenames.push_back(file_name);
                            addfile = false;
                            SavePlugin(&loaded_plugin);
                        }
                    }
                    if (!should_add && ImGui::IsItemHovered()) {
                        if (ImGui::BeginTooltip()) {
                            ImGui::Text("File name already exists!");
                            ImGui::EndTooltip();
                        }
                    }
                    ImGui::End();
                }
            }
            else {
                addfile_set_pos = true;
                if (combo_item != 0)
                    combo_item = 0;
                if (!file_name.empty())
                    file_name.clear();
            }

            if (addversion) {
                if (addversion_set_pos) {
                    if (!ImGui::IsPopupOpen("New template"))
                        ImGui::OpenPopup("New template");
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 120) / 2 });
                }
                addversion_set_pos = false;
                ImGui::SetNextWindowSize({ 300, 120 }, ImGuiCond_Once);
                if (ImGui::BeginPopupModal("New template", &addversion, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                    ImGui::Spacing();
                    ImGui::Text("Minecraft version: ");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::InputText(" ", &version_mc);
                    ImGui::Text("Generator type: ");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::PushID(709);
                    ImGui::Combo(" ", &generator_type, "Forge\0Fabric");
                    ImGui::PopID();
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(100);
                    bool tooltip = generator_type == 1 && versiontype == GLOBALTRIGGER;
                    if (ImGui::Button("Add", { 100, 30 })) {
                        if (versiontype == GLOBALTRIGGER && !tooltip) {
                            open_tabs[current_tab].globaltrigger->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            addversion = false;
                        }
                        else if (versiontype == PROCEDURE) {
                            open_tabs[current_tab].procedure->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            addversion = false;
                            code_editor.SetText("");
                        }
                    }
                    if (tooltip && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Global triggers not supported for Fabric generator.");
                        ImGui::EndTooltip();
                    }
                }
            }
            else {
                addversion_set_pos = true;
                if (!(generator_type == 0))
                    generator_type = 0;
                if (!version_mc.empty())
                    version_mc.clear();
            }

            if (addcomp) {
                if (addcomp_set_pos) {
                    if (!ImGui::IsPopupOpen("New component"))
                        ImGui::OpenPopup("New component");
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 97) / 2 });
                }
                addcomp_set_pos = false;
                ImGui::SetNextWindowSize({ 300, 97 }, ImGuiCond_Once);
                if (ImGui::BeginPopupModal("New component", &addcomp, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                    ImGui::Spacing();
                    ImGui::Text("Component name: ");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::InputText(" ", &component_name, ImGuiInputTextFlags_CharsNoBlank);
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(100);
                    if (ImGui::Button("Add", { 100, 30 })) {
                        Plugin::Component comp;
                        comp.name = component_name;
                        open_tabs[current_tab].procedure->components.push_back(comp);
                        addcomp = false;
                    }
                }
            }
            else {
                addcomp_set_pos = true;
                if (!component_name.empty())
                    component_name.clear();
            }

            // directory tree
            ImGui::SetNextWindowPos({ 0, 18 });
            ImGui::SetNextWindowSize({ 200, (float)GetScreenHeight() - 18 }, ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints({ 100, (float)GetScreenHeight() - 18 }, { 500, (float)GetScreenHeight() - 18 });
            if (ImGui::Begin("Project files", NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
                offset = ImGui::GetWindowWidth() - 200;
                if (ImGui::CollapsingHeader("Procedures")) {
                    if (!loaded_plugin.data.procedures.empty()) {
                        for (Plugin::Procedure& procedure : loaded_plugin.data.procedures) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(procedure.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), procedure.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = PROCEDURE;
                                    window.procedure = &procedure;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(procedure.name);
                                }
                            }
                        }
                    }
                }
                if (ImGui::CollapsingHeader("Global Triggers")) {
                    if (!loaded_plugin.data.globaltriggers.empty()) {
                        for (Plugin::GlobalTrigger& globaltrigger : loaded_plugin.data.globaltriggers) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(globaltrigger.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), globaltrigger.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = GLOBALTRIGGER;
                                    window.globaltrigger = &globaltrigger;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(globaltrigger.name);
                                }
                            }
                        }
                    }
                }
                if (ImGui::CollapsingHeader("Blockly Categories")) {
                    if (!loaded_plugin.data.categories.empty()) {
                        for (Plugin::Category& category : loaded_plugin.data.categories) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(category.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), category.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = CATEGORY;
                                    window.category = &category;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(category.name);
                                }
                            }
                        }
                    }
                }
                ImGui::End();
            }

            // tabs
            ImGui::SetNextWindowPos({ 199 + offset, 12 });
            ImGui::SetNextWindowSize({ (float)GetScreenWidth() - 199 - offset, (float)GetScreenHeight() - 12 });
            if (ImGui::Begin("tabbar", NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration)) {
                if (ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_TabListPopupButton)) {
                    if (open_tabs.empty())
                        change = true;
                    for (int i = 0; i < open_tabs.size(); i++) {
                        if (ImGui::BeginTabItem(open_tab_names[i].c_str(), &open_tabs[i].open)) {
                            current_tab = i;
                            tabsize = open_tabs.size();

                            float cols[4]; // stupid thing wont work in a switch
                            std::vector<std::string> categories = { "Block data", "Block management", "Command parameters", "Direction procedures", "Energy & fluid tanks", "Entity data", "Entity management", "Item procedures", "Player data", "Player procedures", "Projectile procedures", "Slot & GUI procedures", "World data", "World management", "Minecraft components", "Flow control", "Advanced" };
                            if (open_tabs[i].type == PROCEDURE) {
                                cols[0] = open_tabs[i].procedure->color.x;
                                cols[1] = open_tabs[i].procedure->color.y;
                                cols[2] = open_tabs[i].procedure->color.z;
                                cols[3] = open_tabs[i].procedure->color.w;
                                for (const Plugin::Category ct : loaded_plugin.data.categories)
                                    categories.push_back(ct.name);
                            }

                            switch (open_tabs[i].type) {
                            case PROCEDURE:
                                ImGui::Spacing();
                                ImGui::Text("Procedure name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::Text("Procedure type: ");
                                ImGui::SameLine();
                                ImGui::PushID(1);
                                ImGui::Combo(" ", &open_tabs[i].procedure->type, "Input\0Output");
                                if (open_tabs[i].procedure->type == 1) {
                                    ImGui::Text("Procedure output type: ");
                                    ImGui::SameLine();
                                    ImGui::PushID(225);
                                    if (ImGui::BeginCombo(" ", loaded_plugin.ComponentValues[open_tabs[i].procedure->type_index].c_str())) {
                                        for (int k = 0; k < 7; k++) {
                                            if (ImGui::Selectable(loaded_plugin.ComponentValues[k].c_str(), k == open_tabs[i].procedure->type_index)) {
                                                open_tabs[i].procedure->type_index = k;
                                            }
                                        }
                                        ImGui::EndCombo();
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::PopID();
                                ImGui::Text("Procedure color: ");
                                ImGui::SameLine();
                                ImGui::ColorEdit4(" ", cols);
                                open_tabs[i].procedure->color = { cols[0], cols[1], cols[2], cols[3] };
                                ImGui::Text("Procedure category: ");
                                ImGui::SameLine();
                                ImGui::PushID(2);
                                if (open_tabs[i].procedure->category > 16) {
                                    if (std::find(loaded_plugin.data.filenames.begin(), loaded_plugin.data.filenames.end(), open_tabs[i].procedure->category_name) == loaded_plugin.data.filenames.end()) {
                                        open_tabs[i].procedure->category = 0;
                                        open_tabs[i].procedure->category_name = categories[0];
                                    }
                                }
                                if (ImGui::BeginCombo(" ", categories[open_tabs[i].procedure->category].c_str())) {
                                    for (int j = 0; j < categories.size(); j++) {
                                        if (ImGui::Selectable(categories[j].c_str(), open_tabs[i].procedure->category == j)) {
                                            open_tabs[i].procedure->category = j;
                                            open_tabs[i].procedure->category_name = categories[j];
                                        }
                                    }
                                    ImGui::EndCombo();
                                }
                                ImGui::PopID();
                                ImGui::PushID(772);
                                ImGui::Text("Procedure translation key: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tabs[i].procedure->translationkey);
                                ImGui::PopID();
                                ImGui::Checkbox("Require world dependency", &open_tabs[i].procedure->world_dependency);
                                ImGui::Spacing();
                                ImGui::Spacing();
                                ImGui::Text("Procedure block components");
                                if (ImGui::BeginTabBar("args", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable)) {
                                    if (ImGui::TabItemButton("+")) {
                                        addcomp = true;
                                    }
                                    for (int j = 0; j < open_tabs[i].procedure->components.size(); j++) {
                                        if (ImGui::BeginTabItem(open_tabs[i].procedure->components[j].name.c_str(), &open_tabs[i].procedure->components[j].open)) {
                                            ImGui::Spacing();
                                            ImGui::Text("Component name: ");
                                            ImGui::SameLine();
                                            ImGui::InputText(" ", &open_tabs[i].procedure->components[j].name, ImGuiInputTextFlags_CharsNoBlank);
                                            ImGui::Text("Component type: ");
                                            ImGui::SameLine();
                                            ImGui::PushID(56);
                                            ImGui::Combo(" ", &open_tabs[i].procedure->components[j].type_int, "Input value\0Field input\0Field checkbox\0Field dropdown\0Datalist selector\0Input statement\0");
                                            ImGui::PopID();
                                            switch (open_tabs[i].procedure->components[j].type_int) {
                                            case 0:
                                                ImGui::Text("Value type: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(2);
                                                if (ImGui::BeginCombo(" ", loaded_plugin.ComponentValues[open_tabs[i].procedure->components[j].value_int].c_str())) {
                                                    for (int k = 0; k < 7; k++) {
                                                        if (ImGui::Selectable(loaded_plugin.ComponentValues[k].c_str(), k == open_tabs[i].procedure->components[j].value_int)) {
                                                            open_tabs[i].procedure->components[j].value_int = k;
                                                            open_tabs[i].procedure->components[j].component_value = loaded_plugin.ComponentValues[k];
                                                        }
                                                    }
                                                    ImGui::EndCombo();
                                                }
                                                ImGui::PopID();
                                                break;
                                            case 1:
                                                break;
                                            case 2:
                                                ImGui::Checkbox("Is checked by default", &open_tabs[i].procedure->components[j].checkbox_checked);
                                                break;
                                            case 3:
                                                if (ImGui::Button("Add option")) {
                                                    open_tabs[i].procedure->components[j].dropdown_options.push_back("");
                                                }
                                                ImGui::SameLine();
                                                if (ImGui::Button("Remove option") && !open_tabs[i].procedure->components[j].dropdown_options.empty()) {
                                                    open_tabs[i].procedure->components[j].dropdown_options.pop_back();
                                                }
                                                for (int f = 0; f < open_tabs[i].procedure->components[j].dropdown_options.size(); f++) {
                                                    ImGui::PushID(f + 2);
                                                    ImGui::InputText(" ", &open_tabs[i].procedure->components[j].dropdown_options[f]);
                                                    ImGui::PopID();
                                                }
                                                break;
                                            case 4:
                                                ImGui::Text("Datalist name: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(3);
                                                ImGui::InputText(" ", &open_tabs[i].procedure->components[j].datalist);
                                                ImGui::PopID();
                                                break;
                                            case 5:
                                                ImGui::Checkbox("Disable local variables in statement", &open_tabs[i].procedure->components[j].disable_localvars);
                                                break;
                                            }
                                            std::string index_str = "Translation key index: %%" + std::to_string(j + 1);
                                            ImGui::Text(index_str.c_str());
                                            if (ImGui::Button("Copy component code")) {
                                                std::string ccode;
                                                switch (open_tabs[i].procedure->components[j].type_int) {
                                                case 0:
                                                case 5:
                                                    ccode = "${input$" + open_tabs[i].procedure->components[j].name + "}";
                                                    break;
                                                case 1:
                                                case 2:
                                                case 3:
                                                case 4:
                                                    ccode = "${field$" + open_tabs[i].procedure->components[j].name + "}";
                                                    break;
                                                }
                                                SetClipboardText(ccode.c_str());
                                            }
                                            ImGui::EndTabItem();
                                        } // check if closed
                                        for (int u = 0; u < open_tabs[i].procedure->components.size(); u++) {
                                            if (!open_tabs[i].procedure->components[u].open)
                                                open_tabs[i].procedure->components.erase(open_tabs[i].procedure->components.begin() + u);
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                ImGui::Spacing(); ImGui::Spacing();
                                ImGui::Text("Templates");
                                if (ImGui::BeginTabBar("procedure_templates", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable)) {
                                    if (ImGui::TabItemButton("+")) {
                                        addversion = true;
                                        versiontype = PROCEDURE;
                                    }
                                    for (int l = 0; l < open_tabs[i].procedure->versions.size(); l++) {
                                        std::string tabname_pc = open_tabs[i].procedure->versions[l].first + " " + open_tabs[i].procedure->versions[l].second;
                                        if (ImGui::BeginTabItem(tabname_pc.c_str())) {
                                            current_version = l;
                                            ImGui::BeginChild(83, { ImGui::GetColumnWidth(), 500 });
                                            if (current_tab != old_current_tab || tabsize != tabsize_old) {
                                                change = true;
                                                current_version = l;
                                                tabsize = open_tabs.size();
                                            }
                                            if (current_version != old_current_version)
                                                change = true;
                                            if (change) {
                                                code_editor.SetText("");
                                                code_editor.SetText(open_tabs[i].procedure->code[open_tabs[i].procedure->versions[l]]);
                                                change = false;
                                            }
                                            code_editor.Render("code"); // last thing to implement, code editor and saving the code templates (obviously exporting to a zip too)
                                            if (!code_editor.GetText().empty())
                                                open_tabs[i].procedure->code[open_tabs[i].procedure->versions[l]] = code_editor.GetText();
                                            ImGui::EndChild();
                                            old_current_version = l;
                                            ImGui::EndTabItem();
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                break;
                            case GLOBALTRIGGER:
                                ImGui::Spacing();
                                ImGui::Text("Trigger name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::Text("Trigger type: ");
                                ImGui::SameLine();
                                ImGui::PushID(1);
                                ImGui::Combo(" ", &open_tabs[i].globaltrigger->side, "Server-side only\0Client-side only\0Both");
                                ImGui::PopID();
                                ImGui::Checkbox("Is cancelable", &open_tabs[i].globaltrigger->cancelable);
                                ImGui::Spacing();
                                ImGui::Text("Dependencies");
                                if (ImGui::BeginListBox(" ")) {
                                    for (int j = 0; j < 12; j++) {
                                        ImGui::Selectable(loaded_plugin.Dependencies[j].c_str(), &open_tabs[i].globaltrigger->dependencies[j]);
                                    }
                                }
                                ImGui::EndListBox();
                                ImGui::Spacing();
                                ImGui::Text("Templates");
                                if (ImGui::BeginTabBar("template")) {
                                    if (ImGui::TabItemButton("+")) {
                                        addversion = true;
                                        versiontype = GLOBALTRIGGER;
                                    }
                                    for (const std::pair<std::string, std::string> version : open_tabs[i].globaltrigger->versions) {
                                        std::string tabname = version.first + " " + version.second;
                                        if (ImGui::BeginTabItem(tabname.c_str())) {
                                            ImGui::Spacing();
                                            ImGui::Text("Trigger event class: ");
                                            ImGui::SameLine();
                                            ImGui::PushID(69);
                                            ImGui::InputText(" ", &open_tabs[i].globaltrigger->event_code[version]);
                                            ImGui::PopID();
                                            if (HasDependencies(open_tabs[i].globaltrigger->dependencies)) {
                                                ImGui::Spacing();
                                                ImGui::Text("Dependency code");
                                                ImGui::Spacing();
                                                ImGui::BeginGroup();
                                                for (int j = 0; j < 12; j++) {  
                                                    if (open_tabs[i].globaltrigger->dependencies[j]) {
                                                        std::string dep = loaded_plugin.Dependencies[j] + ": ";
                                                        ImGui::AlignTextToFramePadding();
                                                        ImGui::Text(dep.c_str());
                                                        ImGui::SameLine();
                                                        ImGui::PushID(j);
                                                        ImGui::InputText(" ", &open_tabs[i].globaltrigger->dependency_mappings[version][loaded_plugin.Dependencies[j]]);
                                                        ImGui::PopID();
                                                    }
                                                }
                                                ImGui::EndGroup();
                                            }
                                            ImGui::EndTabItem();
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                break;
                            case CATEGORY:
                                ImGui::Spacing();
                                ImGui::Text("Category name: "); 
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::Text("Category color: ");
                                ImGui::SameLine(); 
                                float col[4] = { open_tabs[i].category->color.x, open_tabs[i].category->color.y, open_tabs[i].category->color.z, open_tabs[i].category->color.w };
                                ImGui::ColorEdit4(" ", col);
                                open_tabs[i].category->color = { col[0], col[1], col[2], col[3] };
                                ImGui::Checkbox("Is API category", &open_tabs[i].category->isapi);
                                break;
                            }
                            
                            tabsize_old = open_tabs.size();
                            old_current_tab = i;
                            ImGui::EndTabItem();
                        }
                    }
                    for (int i = 0; i < open_tabs.size(); i++) {
                        if (!open_tabs[i].open) {
                            open_tabs.erase(open_tabs.begin() + i);
                            open_tab_names.erase(open_tab_names.begin() + i);
                        }
                    }
                    ImGui::EndTabBar();
                }
                ImGui::End();
            }
        }

        rlImGuiEnd();
        BeginDrawing();
        EndDrawing();
    }

    UnloadImage(icon);
    UnloadTexture(PluginIcon);

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}