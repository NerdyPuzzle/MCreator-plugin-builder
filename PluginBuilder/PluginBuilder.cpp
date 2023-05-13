#define _CRT_SECURE_NO_WARNINGS
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
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
#include "ziputil.h"
#include "gui_file_dialogs.h"
#include "pluginicon.h"
#include "pluginimage.h"
#include "sourceentity.h"
#include "immediatesourceentity.h"
#include "noentity.h"
#include "provideditemstack.h"
#include "providedblockstate.h"
#include "xyz.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>

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
int delete_datalist = -1;
int delete_translation = -1;
int delete_api = -1;
std::vector<const char*> tempchars_procedures;
std::vector<const char*> tempchars_globaltriggers;
std::vector<const char*> tempchars_categories;
std::vector<const char*> tempchars_datalists;
std::vector<const char*> tempchars_translations;
std::vector<const char*> tempchars_apis;
bool active[6] = { false, false, false, false, false, false };
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
bool mcreator_path_set = false;
std::string mcreator_path;
bool running_with_mcreator = false;
std::vector<std::string> mapping_clipboard;
std::vector<std::string> triggercode_clipboard;
std::string pluginsdir = (std::string)getenv("USERPROFILE") + "\\.mcreator\\plugins\\";
bool help_procedures = false;
bool help_procedures_set_pos = true;
std::vector<std::string> translationkey_clipboard;
bool git_installed = false;
bool git_workspace = false;
bool git_setup = false;
bool git_setup_set_pos = true;
std::string repository_url;
std::string dir;
bool terminal = false;
std::vector<std::string> terminal_lines;
bool localcommit = false;
bool localcommit_set_pos = true;
std::string commit_msg;
bool cloning = false;

std::vector<Plugin> plugins;
Plugin loaded_plugin;

// plugin properties variables
std::string pluginname;
std::string pluginid;
std::string pluginversion;
std::string pluginauthor;
std::string plugindescription;

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
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
                    if (comp.value_int == 4)
                        procedure << "\n" << comp.boolean_checked;
                    else if (comp.value_int == 5)
                        procedure << "\n" << comp.number_default;
                    else if (comp.value_int == 6)
                        procedure << "\n" << comp.text_default;
                    break;
                case 1:
                case 6:
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
                case 7:
                    procedure << "\n" << comp.source;
                    procedure << "\n" << comp.width;
                    procedure << "\n" << comp.height;
                    break;
                }
            }
            bool first__ = true;
            for (const std::pair<std::string, std::string> version : pc.versions) {
                procedure << (!first ? "\n" : "") << "[VERSION] " << version.first << " " << version.second;
                first = false;
            }
            procedure << "\n" << pc.world_dependency;
            procedure << "\n" << pc.requires_api;
            if (pc.requires_api)
                procedure << "\n" << pc.api_name;
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

    if (!plugin->data.datalists.empty()) {
        if (!fs::exists(pluginpath + "datalists/"))
            fs::create_directory(pluginpath + "datalists/");
        for (const Plugin::Datalist dl : plugin->data.datalists) {
            std::ofstream datalist(pluginpath + "datalists/" + dl.name + ".txt");
            datalist << dl.name << "\n";
            for (const std::string entry : dl.entries)
                datalist << entry << "\n";
            datalist << "[ENTRIES_END]\n";
            datalist << dl.versions.size() << "\n";
            datalist << dl.entries.size();
            for (const std::pair<std::string, std::string> version : dl.versions) {
                datalist << "\n" << version.first;
                datalist << "\n" << version.second;
                for (const std::string entry : dl.mappings.at(version)) {
                    datalist << "\n" << entry;
                }
                for (const bool exc : dl.exclusions.at(version)) {
                    datalist << "\n" << exc;
                }
            }
            datalist << "\n" << dl.title;
            datalist << "\n" << dl.message;
            datalist.close();
        }
    }
    else if (fs::exists(pluginpath + "datalists/"))
        fs::remove_all(pluginpath + "datalists/");

    if (!plugin->data.translations.empty()) {
        if (!fs::exists(pluginpath + "translations/"))
            fs::create_directory(pluginpath + "translations/");
        for (const Plugin::Translation tl : plugin->data.translations) {
            std::ofstream translation(pluginpath + "translations/" + tl.name + ".txt");
            translation << tl.name;
            translation << "\n" << tl.language;
            for (int i = 0; i < tl.keys.size(); i++) {
                translation << "\n" << tl.keys[i].first;
                translation << "\n" << tl.keys[i].second;
            }
            translation.close();
        }
    }
    else if (fs::exists(pluginpath + "translations/"))
        fs::remove_all(pluginpath + "translations/");

    if (!plugin->data.apis.empty()) {
        if (!fs::exists(pluginpath + "apis/"))
            fs::create_directory(pluginpath + "apis/");
        for (const Plugin::Api api : plugin->data.apis) {
            std::ofstream api_(pluginpath + "apis/" + api.name + ".txt");
            api_ << api.name << "\n";
            api_ << api.versions.size();
            for (int i = 0; i < api.versions.size(); i++) {
                api_ << "\n" << api.versions[i].first;
                api_ << "\n" << api.versions[i].second;
                if (!fs::exists(pluginpath + "apis/code"))
                    fs::create_directory(pluginpath + "apis/code");
                std::ofstream templ_out(pluginpath + "apis/code/" + api.name + api.versions[i].first + api.versions[i].second + ".txt");
                templ_out << api.code.at(api.versions[i]);
                templ_out.close();
            }
            api_.close();
        }
    }
    else if (fs::exists(pluginpath + "apis/"))
        fs::remove_all(pluginpath + "apis/");

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
                gt.version_names.push_back(version.first + version.second);
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
                pc.category_name.clear();
                while (true) { // for some damn reason std::getline doesn't want to work
                    std::string s;
                    procedure >> s;
                    if (s == "[END]")
                        break;
                    else
                        pc.category_name += (first ? "" : " ") + s;
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
                        if (comp.value_int == 4)
                            procedure >> comp.boolean_checked;
                        else if (comp.value_int == 5)
                            procedure >> comp.number_default;
                        else if (comp.value_int == 6) {
                            comp.text_default.clear();
                            procedure >> comp.text_default;
                            std::getline(procedure, temp);
                            comp.text_default.append(temp);
                            temp.clear();
                        }
                        break;
                    case 1:
                    case 6:
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
                    case 7:
                        procedure >> comp.source;
                        std::getline(procedure, temp);
                        comp.source.append(temp);
                        temp.clear();
                        procedure >> comp.width;
                        procedure >> comp.height;
                        break;
                    }
                    pc.component_names.push_back(comp.name);
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
                    else if (line_.size() == 1) {
                        pc.world_dependency = (line_ == "1" ? true : false);
                        break;
                    }
                }
                procedure >> pc.requires_api;
                if (pc.requires_api) {
                    procedure >> pc.api_name;
                    std::getline(procedure, temp);
                    pc.api_name.append(temp);
                    temp.clear();
                }
                if (fs::exists(path + "procedures/code/")) {
                    for (const std::pair<std::string, std::string> version : pc.versions) {
                        pc.version_names.push_back(version.first + version.second);
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

    if (fs::exists(path + "datalists/")) {
        for (const fs::path entry : fs::directory_iterator(path + "datalists/")) { // load datalists
            std::ifstream datalist(entry.string());
            Plugin::Datalist dl;
            std::getline(datalist, dl.name);
            std::string temp;
            while (std::getline(datalist, temp)) {
                if (!temp.empty() && temp != "[ENTRIES_END]")
                    dl.entries.push_back(temp);
                else if (temp == "[ENTRIES_END]")
                    break;
            }
            int versioncount = 0;
            int entrycount = 0;
            datalist >> versioncount;
            datalist >> entrycount;
            for (int i = 0; i < versioncount; i++) {
                std::pair<std::string, std::string> version;
                datalist >> version.first;
                datalist >> version.second;
                dl.version_names.push_back(version.first + version.second);
                dl.versions.push_back(version);
                for (int j = 0; j < entrycount; j++) {
                    datalist >> temp;
                    dl.mappings[version].push_back(temp);
                    std::getline(datalist, temp);
                    dl.mappings[version][dl.mappings[version].size() - 1].append(temp);
                    temp.clear();
                }
                for (int j = 0; j < entrycount; j++) {
                    bool exc;
                    datalist >> exc;
                    dl.exclusions[version].push_back(exc);
                }
            }
            temp.clear();
            datalist >> dl.title;
            std::getline(datalist, temp);
            dl.title.append(temp);
            datalist >> dl.message;
            temp.clear();
            std::getline(datalist, temp);
            dl.message.append(temp);
            datalist.close();
            plugin.data.datalists.push_back(dl);
        }
    }

    if (fs::exists(path + "translations/")) {
        for (const fs::path entry : fs::directory_iterator(path + "translations/")) { // load translations
            std::ifstream translation(entry.string());
            Plugin::Translation tl;
            std::getline(translation, tl.name);
            std::getline(translation, tl.language);
            while (!translation.eof()) {
                std::pair<std::string, std::string> keypair;
                std::getline(translation, keypair.first);
                std::getline(translation, keypair.second);
                tl.keys.push_back(keypair);
            }
            translation.close();
            plugin.data.translations.push_back(tl);
        }
    }

    if (fs::exists(path + "apis/")) {
        for (const fs::path entry : fs::directory_iterator(path + "apis/")) { // load external APIs
            if (fs::is_regular_file(entry)) {
                std::ifstream api_(entry.string());
                Plugin::Api api;
                std::getline(api_, api.name);
                int version_count = 0;
                api_ >> version_count;
                for (int i = 0; i < version_count; i++) {
                    std::pair<std::string, std::string> version_;
                    api_ >> version_.first;
                    api_ >> version_.second;
                    api.versions.push_back(version_);
                    api.version_names.push_back(version_.first + version_.second);
                    std::ifstream templ(path + "apis/code/" + api.name + version_.first + version_.second + ".txt");
                    std::string temp_s;
                    bool firstl = true;
                    while (std::getline(templ, temp_s)) {
                        api.code[version_].append((std::string)(firstl ? "" : "\n") + temp_s);
                        firstl = false;
                    }
                    templ.close();
                }
                api_.close();
                plugin.data.apis.push_back(api);
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
    if (!fs::exists("settings/"))
        fs::create_directory("settings/");
    std::ifstream mc_path("settings/mcreator.txt");
    if (mc_path.is_open()) {
        mcreator_path_set = true;
        std::getline(mc_path, mcreator_path);
    }
    mc_path.close();
    if (std::system("git --version") == 0)
        git_installed = true;
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
    std::string mcreator_plugins_path;
    int result = (!running_with_mcreator ? GuiFileDialog(DIALOG_SAVE_FILE, "Export plugin", path, "*.zip", "MCreator plugin (*.zip)") : 1);
    if (result == 1) {
        if (running_with_mcreator)
            mcreator_plugins_path = pluginsdir + plugin.data.id + ".zip";
        fs::create_directory("temp_data");
        std::ofstream lang("temp_data\\texts.properties");
        std::ofstream pjson("temp_data\\plugin.json");
        Zip zip = Zip::Create((!running_with_mcreator ? (std::string)path : mcreator_plugins_path));
        running_with_mcreator = false;
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
                        case 6:
                            pc_ << "      \"type\": \"input_dummy\"\n";
                            break;
                        case 7:
                            pc_ << "      \"type\": \"field_image\",\n";
                            pc_ << "      \"src\": \"" + comp.source + "\",\n";
                            pc_ << "      \"width\": \"" + std::to_string(comp.width) + "\",\n";
                            pc_ << "      \"height\": \"" + std::to_string(comp.height) + "\"\n";
                            break;
                        }
                        pc_ << "    }" + (std::string)(i == pc.components.size() ? "\n" : ",\n");
                    }
                    pc_ << "  ],\n";
                    pc_ << "  \"inputsInline\": true,\n";
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
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "math_number\\" << '"' << ">" << "<field name=" << "\\" << '"' << "NUM\\" << '"' << ">" << comp.number_default << "</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Direction")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "direction_unspecified\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Boolean")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "logic_boolean\\" << '"' << ">" << "<field name=" << "\\" << '"' << "BOOL\\" << '"' << ">" << (comp.boolean_checked == 0 ? "TRUE" : "FALSE") << "</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Text")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "text\\" << '"' << ">" << "<field name=" << "\\" << '"' << "TEXT\\" << '"' << ">" << comp.text_default << "</field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Block" && comp.name != "providedblockstate")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "mcitem_allblocks\\" << '"' << ">" << "<field name=" << "\\" << '"' << "value\\" << '"' << "></field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Block" && comp.name == "providedblockstate")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "blockstate_from_deps\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Item" && comp.name != "provideditemstack")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "mcitem_all\\" << '"' << ">" << "<field name=" << "\\" << '"' << "value\\" << '"' << "></field>" << "</block></value>" << '"';
                        else if (comp.component_value == "Item" && comp.name == "provideditemstack")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' <<  "itemstack_to_mcitem\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Entity" && comp.name != "sourceentity" && comp.name != "immediatesourceentity" && comp.name != "noentity")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "entity_from_deps\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Entity" && comp.name == "sourceentity")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "source_entity_from_deps\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Entity" && comp.name == "immediatesourceentity")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "immediate_source_entity_from_deps\\" << '"' << "></block></value>" << '"';
                        else if (comp.component_value == "Entity" && comp.name == "noentity")
                            pc_ << "      " << '"' << "<value name=\\" << '"' << comp.name << "\\" << '"' << "><block type=\\" << '"' << "entity_none\\" << '"' << "></block></value>" << '"';
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
                            case 6:
                            case 7:
                                break;
                            }
                        }
                        if (!inputs.empty()) {
                            pc_ << "    \"inputs\": [\n";
                            for (const std::string s : inputs) {
                                i++;
                                pc_ << "      \"" + s + "\"" + (std::string)(i == inputs.size() ? "\n" : ",\n");
                            }
                            pc_ << "    ]" + (std::string)(!fields.empty() || pc.world_dependency || pc.requires_api ? ",\n" : "\n");
                            i = 0;
                        }
                        if (!fields.empty()) {
                            pc_ << "    \"fields\": [\n";
                            for (const std::string s : fields) {
                                i++;
                                pc_ << "      \"" + s + "\"" + (std::string)(i == fields.size() ? "\n" : ",\n");
                            }
                            pc_ << "    ]" + (std::string)(!statements.empty() || pc.world_dependency || pc.requires_api ? ",\n" : "\n");
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
                            pc_ << "    ]" + (std::string)(pc.world_dependency || pc.requires_api ? ",\n" : "\n");
                            i = 0;
                        }
                    }
                    if (pc.world_dependency) {
                        pc_ << "    \"dependencies\": [\n";
                        pc_ << "      {\n";
                        pc_ << "        \"name\": \"world\",\n";
                        pc_ << "        \"type\": \"world\"\n";
                        pc_ << "      }\n";
                        pc_ << "    ]" + (std::string)(pc.requires_api ? ",\n" : "\n");
                    }
                    if (pc.requires_api) {
                        pc_ << "    \"required_apis\": [\n";
                        pc_ << "      \"" + RegistryName(pc.api_name) + "\"\n";
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
        if (!plugin.data.datalists.empty()) {
            zip.AddFolder("datalists");
            for (const Plugin::Datalist dl : plugin.data.datalists) {
                std::ofstream dl_("temp_data\\" + dl.name + ".txt");
                bool firstentry = true;
                for (int i = 0; i < dl.entries.size(); i++) {
                    dl_ << (firstentry ? "" : "\n") << "- " << dl.entries[i];
                    firstentry = false;
                }
                dl_.close();
                lang << "dialog.selector." + dl.name + ".title=" + dl.title + "\n";
                lang << "dialog.selector." + dl.name + ".message=" + dl.message + "\n";
                zip.AddFile("temp_data\\" + dl.name + ".txt", "datalists\\" + dl.name + ".yaml");
            }
        }
        if (!plugin.data.translations.empty()) {
            for (const Plugin::Translation tl : plugin.data.translations) {
                std::ofstream tl_("temp_data\\" + tl.name + ".txt");
                for (int i = 0; i < tl.keys.size(); i++) {
                    tl_ << tl.keys[i].first << "=" << tl.keys[i].second << "\n";
                }
                tl_.close();
                zip.AddFile("temp_data\\" + tl.name + ".txt", "lang\\texts_" + tl.language + ".properties");
            }
        }
        lang.close();
        zip.AddFile("temp_data\\texts.properties", "lang\\texts.properties");
        std::vector<std::string> versions;
        std::vector<std::string> triggerversions;
        std::vector<std::string> procedureversions;
        std::vector<std::string> datalistversions;
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
        if (!plugin.data.datalists.empty()) {
            for (const Plugin::Datalist dl : plugin.data.datalists) {
                if (!dl.versions.empty()) {
                    for (const std::pair<std::string, std::string> version : dl.versions) {
                        std::string version_str = RegistryName(version.second) + "-" + version.first;
                        if (std::find(versions.begin(), versions.end(), version_str) == versions.end())
                            versions.push_back(version_str);
                        datalistversions.push_back(version_str);
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
            if (std::find(datalistversions.begin(), datalistversions.end(), vers) != datalistversions.end())
                zip.AddFolder(vers + "\\mappings");
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
                    gt_ << "    public static void onEventTriggered(" + gt.event_code.at(version) + " event) {\n";
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
                    gt_ << "    }";
                    gt_.close();
                    zip.AddFile("temp_data\\" + gt.name + version.first + version.second + ".txt", foldername + "\\triggers\\" + RegistryName(gt.name) + ".java.ftl");
                }
            }
        }
        if (!plugin.data.datalists.empty()) {
            for (const Plugin::Datalist dl : plugin.data.datalists) {
                for (const std::pair<std::string, std::string> version : dl.versions) {
                    std::string foldername = RegistryName(version.second) + "-" + version.first;
                    std::ofstream dl_("temp_data\\" + dl.name + version.first + version.second + ".txt");
                    bool firstline = true;
                    for (int k = 0; k < dl.entries.size(); k++) {
                        if (!dl.exclusions.at(version).at(k)) {
                            dl_ << (firstline ? "" : "\n") << dl.entries.at(k) << ": " + dl.mappings.at(version).at(k);
                            firstline = false;
                        }
                    }
                    dl_.close();
                    zip.AddFile("temp_data\\" + dl.name + version.first + version.second + ".txt", foldername + "\\mappings\\" + dl.name + ".yaml");
                }
            }
        }
        if (!plugin.data.apis.empty()) {
            zip.AddFolder("apis");
            for (const Plugin::Api api : plugin.data.apis) {
                std::ofstream api_("temp_data\\" + api.name + ".txt");
                api_ << "---";
                for (int i = 0; i < api.versions.size(); i++) {
                    api_ << "\n" << LowerStr(api.versions[i].second) << "-" << api.versions[i].first << ":";
                    api_ << "\n" << "  gradle: |";
                    std::string templ;
                    std::ifstream in_code("plugins\\" + plugin.data.name + "\\apis\\code\\" + api.name + api.versions[i].first + api.versions[i].second + ".txt");
                    while (std::getline(in_code, templ))
                        api_ << "\n" << "    " << templ;
                    in_code.close();
                    api_ << "\n" << "  update_files:";
                    api_ << "\n" << "    - ~";
                    api_ << "\n";
                }
                api_ << "name: \"" + api.name + "\"";
                api_.close();
                zip.AddFile("temp_data\\" + api.name + ".txt", "apis\\" + RegistryName(api.name) + ".yaml");
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
    PROCEDURE, GLOBALTRIGGER, CATEGORY, DATALIST, TRANSLATION, API
};
struct TabWindow {
    WindowType type;
    bool open = true;
    Plugin::Procedure* procedure;
    Plugin::GlobalTrigger* globaltrigger;
    Plugin::Category* category;
    Plugin::Datalist* datalist;
    Plugin::Translation* translation;
    Plugin::Api* api;
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

    Image icon = { 0 };
    icon.height = PLUGINICON_HEIGHT;
    icon.width = PLUGINICON_WIDTH;
    icon.format = PLUGINICON_FORMAT;
    icon.data = PLUGINICON_DATA;
    icon.mipmaps = 1;
    Image icon_temp = { 0 };
    icon_temp.height = PLUGINIMAGE_HEIGHT;
    icon_temp.width = PLUGINIMAGE_WIDTH;
    icon_temp.format = PLUGINIMAGE_FORMAT;
    icon_temp.data = PLUGINIMAGE_DATA;
    icon_temp.mipmaps = 1;
    Image sourceentity_temp = { 0 };
    sourceentity_temp.height = SOURCEENTITY_HEIGHT;
    sourceentity_temp.width = SOURCEENTITY_WIDTH;
    sourceentity_temp.format = SOURCEENTITY_FORMAT;
    sourceentity_temp.data = SOURCEENTITY_DATA;
    sourceentity_temp.mipmaps = 1;
    Image immediatesourceentity_temp = { 0 };
    immediatesourceentity_temp.height = IMMEDIATESOURCEENTITY_HEIGHT;
    immediatesourceentity_temp.width = IMMEDIATESOURCEENTITY_WIDTH;
    immediatesourceentity_temp.format = IMMEDIATESOURCEENTITY_FORMAT;
    immediatesourceentity_temp.data = IMMEDIATESOURCEENTITY_DATA;
    immediatesourceentity_temp.mipmaps = 1;
    Image noentity_temp = { 0 };
    noentity_temp.height = NOENTITY_HEIGHT;
    noentity_temp.width = NOENTITY_WIDTH;
    noentity_temp.format = NOENTITY_FORMAT;
    noentity_temp.data = NOENTITY_DATA;
    noentity_temp.mipmaps = 1;
    Image provideditemstack_temp = { 0 };
    provideditemstack_temp.height = PROVIDEDITEMSTACK_HEIGHT;
    provideditemstack_temp.width = PROVIDEDITEMSTACK_WIDTH;
    provideditemstack_temp.format = PROVIDEDITEMSTACK_FORMAT;
    provideditemstack_temp.data = PROVIDEDITEMSTACK_DATA;
    provideditemstack_temp.mipmaps = 1;
    Image providedblockstate_temp = { 0 };
    providedblockstate_temp.height = PROVIDEDBLOCKSTATE_HEIGHT;
    providedblockstate_temp.width = PROVIDEDBLOCKSTATE_WIDTH;
    providedblockstate_temp.format = PROVIDEDBLOCKSTATE_FORMAT;
    providedblockstate_temp.data = PROVIDEDBLOCKSTATE_DATA;
    providedblockstate_temp.mipmaps = 1;
    Image xyz_temp = { 0 };
    xyz_temp.height = XYZ_HEIGHT;
    xyz_temp.width = XYZ_WIDTH;
    xyz_temp.format = XYZ_FORMAT;
    xyz_temp.data = XYZ_DATA;
    xyz_temp.mipmaps = 1;
    Texture PluginIcon = LoadTextureFromImage(icon_temp);
    Texture Sourceentity = LoadTextureFromImage(sourceentity_temp);
    Texture Immediatesourceentity = LoadTextureFromImage(immediatesourceentity_temp);
    Texture Noentity = LoadTextureFromImage(noentity_temp);
    Texture Provideditemstack = LoadTextureFromImage(provideditemstack_temp);
    Texture Providedblockstate = LoadTextureFromImage(providedblockstate_temp);
    Texture Xyz = LoadTextureFromImage(xyz_temp);

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
                        std::string title = "Plugin Builder - [" + loaded_plugin.data.name + "]";
                        SetWindowTitle(title.c_str());
                        if (fs::exists("plugins\\" + loaded_plugin.data.name + "\\.git"))
                            git_workspace = true;
                        dir = "cd " + (std::string)GetWorkingDirectory() + "\\plugins\\" + loaded_plugin.data.name + " && ";
                    }
                if (ImGui::Button("Delete", { ImGui::GetColumnWidth(), 50 }))
                    if (selected_plugin != -1) {
                        delete_confirm = true;
                    }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::Button("Clone remote", { ImGui::GetColumnWidth(), 50 })) {
                    cloning = true;
                    git_setup = true;
                }
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

        if (git_setup) {
            if (git_setup_set_pos) {
                if (!ImGui::IsPopupOpen("Setup remote project"))
                    ImGui::OpenPopup("Setup remote project");
                ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 97) / 2 });
            }
            git_setup_set_pos = false;
            ImGui::SetNextWindowSize({ 300, 97 }, ImGuiCond_Once);
            if (ImGui::BeginPopupModal("Setup remote project", &git_setup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Github repository HTTPS:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                ImGui::InputText(" ", &repository_url);
                ImGui::Spacing();
                ImGui::SetCursorPosX(100);
                if (ImGui::Button("Create", { 100, 30 })) {
                    if (git_installed) {
                        if (!cloning) {
                            if (!fs::exists("plugins\\" + loaded_plugin.data.name + "\\.git")) {
                                std::string init = dir + "git init";
                                terminal_lines.push_back(exec(init.c_str()));
                                std::string add = dir + "git add .";
                                terminal_lines.push_back(exec(add.c_str()));
                                std::string commit = dir + "git commit -m \"Initial commit\"";
                                terminal_lines.push_back(exec(commit.c_str()));
                            }
                            std::string repo_url = dir + "git remote add origin " + repository_url;
                            if (std::system(repo_url.c_str()) == 0) {
                                std::string push = dir + "git push --set-upstream origin master";
                                terminal_lines.push_back(exec(push.c_str()));
                                git_setup = false;
                                git_workspace = true;
                            }
                        }
                        else {
                            fs::create_directory("temp_clone");
                            std::string clone_temp = "cd " + (std::string)GetWorkingDirectory() + "\\temp_clone && git clone " + repository_url;
                            if (std::system(clone_temp.c_str()) == 0) {
                                std::string path;
                                for (const fs::path entry : fs::directory_iterator("temp_clone\\")) {
                                    if (fs::is_directory(entry.string()) && entry.filename() != ".git") {
                                        path = entry.string();
                                        break;
                                    }
                                }
                                std::ifstream in_(path + "\\info.txt");
                                std::string plgn_name;
                                std::getline(in_, plgn_name);
                                in_.close();
                                if (!plgn_name.empty()) {
                                    std::string remover = "attrib -r " + (std::string)GetWorkingDirectory() + "\\" + path + "\\*.* /s";
                                    std::system(remover.c_str());
                                    fs::remove_all("temp_clone\\");
                                    if (fs::exists("plugins\\" + plgn_name))
                                        fs::remove_all("plugins\\" + plgn_name + "\\");
                                    fs::create_directory("plugins\\" + plgn_name);
                                    std::string clone = "cd " + (std::string)GetWorkingDirectory() + "\\plugins\\" + plgn_name + " && git clone " + repository_url + " .";
                                    std::system(clone.c_str());
                                    plugins.clear();
                                    LoadAllPlugins();
                                    git_setup = false;
                                }
                            }
                        }
                    }
                    else
                        OpenURL("https://git-scm.com/downloads");
                }
                ImGui::End();
            }
        }
        else {
            git_setup_set_pos = true;
            if (!repository_url.empty())
                repository_url.clear();
            cloning = false;
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
                    ImGui::Separator();
                    if (ImGui::MenuItem("Close")) {
                        SavePlugin(&loaded_plugin);
                        pluginmenu = false;
                        open_tabs.clear();
                        open_tab_names.clear();
                        mainmenu = true;
                        if (mcreator_path_set) {
                            if (fs::exists(mcreator_path + "\\plugins\\" + loaded_plugin.data.id + ".zip"))
                                fs::remove(mcreator_path + "\\plugins\\" + loaded_plugin.data.id + ".zip");
                        }
                        terminal = false;
                        terminal_lines.clear();
                        SetWindowTitle("Plugin Builder");
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Build")) {
                    if (ImGui::MenuItem("Set MCreator path")) {
                        char path[1024] = { 0 };
                        int result = GuiFileDialog(DIALOG_OPEN_DIRECTORY, "Select MCreator folder", path, nullptr, nullptr);
                        if (result == 1) {
                            mcreator_path_set = true;
                            mcreator_path = (std::string)path;
                            std::ofstream out("settings\\mcreator.txt");
                            out << (std::string)path;
                            out.close();
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Export")) {
                        ExportPlugin(loaded_plugin);
                    }
                    if (ImGui::MenuItem("Run with MCreator", NULL, nullptr, mcreator_path_set)) {
                        if (!IsProcessRunning(L"mcreator.exe")) {
                            running_with_mcreator = true;
                            if (fs::exists(pluginsdir + loaded_plugin.data.id + ".zip"))
                                fs::remove(pluginsdir + loaded_plugin.data.id + ".zip");
                            ExportPlugin(loaded_plugin);
                            RunExe(mcreator_path + "\\mcreator.exe");
                        }
                    }
                    if (IsProcessRunning(L"mcreator.exe") && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("MCreator is already running!");
                        ImGui::EndTooltip();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Git")) {
                    if (ImGui::MenuItem("Install git", nullptr, nullptr, !git_installed)) {
                        OpenURL("https://git-scm.com/downloads");
                    }
                    if (ImGui::MenuItem("Setup remote project", nullptr, nullptr, git_installed)) {
                        git_setup = true;
                    }
                    if (ImGui::MenuItem("Open git terminal", nullptr, nullptr, !terminal)) {
                        terminal = true;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Commit changes", nullptr, nullptr, git_installed && git_workspace)) {
                        localcommit = true;
                    }
                    if (ImGui::MenuItem("Discard local commits", nullptr, nullptr, git_installed && git_workspace && false)) {
                        std::string reset = dir + "git reset --hard";
                        terminal_lines.push_back(exec(reset.c_str()));
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Fetch origin", nullptr, nullptr, git_installed && git_workspace)) {
                        std::string fetch = dir + "git fetch --set-upstream origin master";
                        terminal_lines.push_back(exec(fetch.c_str()));
                    }
                    if (ImGui::MenuItem("Pull origin", nullptr, nullptr, git_installed && git_workspace)) {
                        std::string pull = dir + "git pull --set-upstream origin master";
                        terminal_lines.push_back(exec(pull.c_str()));
                        std::string pluginname = loaded_plugin.data.name;
                        open_tabs.clear();
                        open_tab_names.clear();
                        loaded_plugin = LoadPlugin("plugins\\" + pluginname + "\\");
                    }
                    if (ImGui::MenuItem("Push origin", nullptr, nullptr, git_installed && git_workspace)) {
                        std::string push = dir + "git push --set-upstream origin master";
                        terminal_lines.push_back(exec(push.c_str()));
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help")) {
                    if (ImGui::MenuItem("Procedures")) {
                        help_procedures = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (terminal) {
                ImGui::SetNextWindowSize({ 300, 100 }, ImGuiCond_Once);
                ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 100) / 2 }, ImGuiCond_Once);
                if (ImGui::Begin("Git terminal", &terminal)) {
                    ImGui::BeginChild(248);
                    for (int i = 0; i < terminal_lines.size(); i++)
                        ImGui::Text(terminal_lines[i].c_str());
                    ImGui::EndChild();
                }
                ImGui::End();
            }

            if (localcommit) {
                if (localcommit_set_pos) {
                    if (!ImGui::IsPopupOpen("Commit changes")) {
                        ImGui::OpenPopup("Commit changes");
                        std::string getchanges = dir + "git diff --stat";
                        terminal_lines.push_back(exec(getchanges.c_str()));
                    }
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 400) / 2, (float)(GetScreenHeight() - 230) / 2 });
                }
                localcommit_set_pos = false;
                ImGui::SetNextWindowSize({ 400, 230 }, ImGuiCond_Once);
                if (ImGui::BeginPopupModal("Commit changes", &localcommit, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Commit message:");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::InputText(" ", &commit_msg);
                    ImGui::NewLine();
                    ImGui::Text("Changes");
                    ImGui::BeginChild(283, { ImGui::GetColumnWidth(), 100 });
                    ImGui::TextColored(rlImGuiColors::Convert(YELLOW), terminal_lines[terminal_lines.size() - 1].c_str());
                    ImGui::EndChild();
                    ImGui::SetCursorPosX(150);
                    if (ImGui::Button("Commit", { 100, 30 }) && !commit_msg.empty()) {
                        std::string addall = dir + "git add .";
                        terminal_lines.push_back(exec(addall.c_str()));
                        std::string commit = dir + "git commit -m \"" + commit_msg + "\"";
                        terminal_lines.push_back(exec(commit.c_str()));
                        localcommit = false;
                    }
                    ImGui::End();
                }
            }
            else {
                localcommit_set_pos = true;
                if (!commit_msg.empty())
                    commit_msg.clear();
            }

            if (deletefile) {
                ImGui::OpenPopup("Delete file");
                if (deletefile_set_pos) {
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 300) / 2, (float)(GetScreenHeight() - 270) / 2 });
                    for (int j = 0; j < loaded_plugin.data.procedures.size(); j++) {
                        tempchars_procedures.push_back(loaded_plugin.data.procedures[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.globaltriggers.size(); j++) {
                        tempchars_globaltriggers.push_back(loaded_plugin.data.globaltriggers[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.categories.size(); j++) {
                        tempchars_categories.push_back(loaded_plugin.data.categories[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.datalists.size(); j++) {
                        tempchars_datalists.push_back(loaded_plugin.data.datalists[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.translations.size(); j++) {
                        tempchars_translations.push_back(loaded_plugin.data.translations[j].name.c_str());
                    }
                    for (int j = 0; j < loaded_plugin.data.apis.size(); j++) {
                        tempchars_apis.push_back(loaded_plugin.data.apis[j].name.c_str());
                    }
                }
                deletefile_set_pos = false;
                ImGui::SetNextWindowSize({ 300, 270 });
                if (ImGui::BeginPopupModal("Delete file", &deletefile, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
                    if (delete_procedure != -1 && !active[0]) {
                        delete_globaltrigger = -1;
                        delete_category = -1;
                        delete_datalist = -1;
                        delete_translation = -1;
                        delete_api = -1;
                        active[0] = true;
                        active[1] = false;
                        active[2] = false;
                        active[3] = false;
                        active[4] = false;
                        active[5] = false;
                    }
                    else if (delete_globaltrigger != -1 && !active[1]) {
                        delete_procedure = -1;
                        delete_category = -1;
                        delete_datalist = -1;
                        delete_translation = -1;
                        delete_api = -1;
                        active[1] = true;
                        active[0] = false;
                        active[2] = false;
                        active[3] = false;
                        active[4] = false;
                        active[5] = false;
                    }
                    else if (delete_category != -1 && !active[2]) {
                        delete_procedure = -1;
                        delete_globaltrigger = -1;
                        delete_datalist = -1;
                        delete_translation = -1;
                        delete_api = -1;
                        active[2] = true;
                        active[1] = false;
                        active[0] = false;
                        active[3] = false;
                        active[4] = false;
                        active[5] = false;
                    }
                    else if (delete_datalist != -1 && !active[3]) {
                        delete_procedure = -1;
                        delete_globaltrigger = -1;
                        delete_category = -1;
                        delete_translation = -1;
                        delete_api = -1;
                        active[3] = true;
                        active[2] = false;
                        active[1] = false;
                        active[0] = false;
                        active[4] = false;
                        active[5] = false;
                    }
                    else if (delete_api != -1 && !active[4]) {
                        delete_procedure = -1;
                        delete_globaltrigger = -1;
                        delete_category = -1;
                        delete_translation = -1;
                        delete_datalist = -1;
                        active[5] = true;
                        active[2] = false;
                        active[1] = false;
                        active[0] = false;
                        active[4] = false;
                        active[3] = false;
                    }

                    ImGui::Text("Procedures");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("procedures", &delete_procedure, tempchars_procedures.data(), loaded_plugin.data.procedures.size());
                    ImGui::Text("Global Triggers");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("globaltriggers", &delete_globaltrigger, tempchars_globaltriggers.data(), loaded_plugin.data.globaltriggers.size());
                    ImGui::Text("Blockly Categories");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("categories", &delete_category, tempchars_categories.data(), loaded_plugin.data.categories.size());
                    ImGui::Text("Datalists");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("datalists", &delete_datalist, tempchars_datalists.data(), loaded_plugin.data.datalists.size());
                    ImGui::Text("Translations");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("translations", &delete_translation, tempchars_translations.data(), loaded_plugin.data.translations.size());
                    ImGui::Text("APIs");
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::ListBox("apis", &delete_api, tempchars_apis.data(), loaded_plugin.data.apis.size());

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
                    else if (delete_datalist != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.datalists[delete_datalist].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }
                    else if (delete_translation != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.translations[delete_translation].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }
                    else if (delete_api != -1) {
                        if (std::find(open_tab_names.begin(), open_tab_names.end(), loaded_plugin.data.filenames[IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.apis[delete_api].name)]) != open_tab_names.end())
                            file_open = true;
                        else
                            file_open = false;
                    }

                    ImGui::SetCursorPosX(100);
                    if (ImGui::Button("Delete", { 100, 30 })) {
                        if (delete_procedure != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/procedures/" + loaded_plugin.data.procedures[delete_procedure].name + ".txt");
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
                        else if (delete_datalist != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/datalists/" + loaded_plugin.data.datalists[delete_datalist].name + ".txt");
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.datalists[delete_datalist].name));
                            loaded_plugin.data.datalists.erase(loaded_plugin.data.datalists.begin() + delete_datalist);
                            tempchars_datalists.erase(tempchars_datalists.begin() + delete_datalist);
                        }
                        else if (delete_translation != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/translations/" + loaded_plugin.data.translations[delete_translation].name + ".txt");
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.translations[delete_translation].name));
                            loaded_plugin.data.translations.erase(loaded_plugin.data.translations.begin() + delete_translation);
                            tempchars_translations.erase(tempchars_translations.begin() + delete_translation);
                        }
                        else if (delete_api != -1 && !file_open) {
                            fs::remove("plugins/" + loaded_plugin.data.name + "/apis/" + loaded_plugin.data.apis[delete_api].name + ".txt");
                            loaded_plugin.data.filenames.erase(loaded_plugin.data.filenames.begin() + IndexOf(loaded_plugin.data.filenames, loaded_plugin.data.apis[delete_api].name));
                            loaded_plugin.data.apis.erase(loaded_plugin.data.apis.begin() + delete_api);
                            tempchars_apis.erase(tempchars_apis.begin() + delete_api);
                        }
                        if ((delete_procedure != -1 || delete_globaltrigger != -1 || delete_category != -1 || delete_datalist != -1 || delete_translation != -1 || delete_api != -1) && !file_open) {
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
                delete_translation = -1;
                delete_api = -1;
                active[0] = false; active[1] = false; active[2] = false; active[3] = false; active[4] = false; active[5] = false;
                if (!tempchars_procedures.empty())
                    tempchars_procedures.clear();
                if (!tempchars_globaltriggers.empty())
                    tempchars_globaltriggers.clear();
                if (!tempchars_categories.empty())
                    tempchars_categories.clear();
                if (!tempchars_datalists.empty())
                    tempchars_datalists.clear();
                if (!tempchars_translations.empty())
                    tempchars_translations.clear();
                if (!tempchars_apis.empty())
                    tempchars_apis.clear();
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
                    ImGui::Combo(" ", &combo_item, "Procedure\0Global Trigger\0Blockly Category\0Datalist\0Translation\0API");
                    ImGui::Spacing();
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Name: "); ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                    ImGui::PushID(1);
                    ImGui::InputText(" ", &file_name, (combo_item == 3 ? ImGuiInputTextFlags_CharsNoBlank : ImGuiInputTextFlags_None));
                    ImGui::PopID();
                    ImGui::Spacing();
                    ImGui::SetCursorPosX(100);
                    bool should_add = std::find(loaded_plugin.data.filenames.begin(), loaded_plugin.data.filenames.end(), file_name) == loaded_plugin.data.filenames.end();
                    bool tabs_open = !open_tabs.empty();
                    if (ImGui::Button("Add", { 100, 30 })) {
                        if (should_add && !tabs_open && !file_name.empty()) {
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
                            else if (combo_item == 2) {
                                window.type = CATEGORY;
                                Plugin::Category category;
                                category.name = file_name;
                                category.color = rlImGuiColors::Convert(WHITE);
                                category.isapi = false;
                                loaded_plugin.data.categories.push_back(category);
                                window.category = &loaded_plugin.data.categories[loaded_plugin.data.categories.size() - 1];
                            }
                            else if (combo_item == 3) {
                                window.type = DATALIST;
                                Plugin::Datalist datalist;
                                datalist.name = file_name;
                                loaded_plugin.data.datalists.push_back(datalist);
                                window.datalist = &loaded_plugin.data.datalists[loaded_plugin.data.datalists.size() - 1];
                            }
                            else if (combo_item == 4) {
                                window.type = TRANSLATION;
                                Plugin::Translation translation;
                                translation.name = file_name;
                                loaded_plugin.data.translations.push_back(translation);
                                window.translation = &loaded_plugin.data.translations[loaded_plugin.data.translations.size() - 1];
                            }
                            else if (combo_item == 5) {
                                window.type = API;
                                Plugin::Api api;
                                api.name = file_name;
                                loaded_plugin.data.apis.push_back(api);
                                window.api = &loaded_plugin.data.apis[loaded_plugin.data.apis.size() - 1];
                            }
                            loaded_plugin.data.filenames.push_back(file_name);
                            open_tabs.push_back(window);
                            open_tab_names.push_back(file_name);
                            addfile = false;
                            SavePlugin(&loaded_plugin);
                        }
                    }
                    if ((!should_add || tabs_open) && ImGui::IsItemHovered()) {
                        if (ImGui::BeginTooltip()) {
                            ImGui::Text(!tabs_open ? "File name already exists!" : "Cannot make new files while tabs are open.");
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
                    bool should_add = false;
                    if (versiontype == PROCEDURE) {
                        should_add = std::find(open_tabs[current_tab].procedure->version_names.begin(), open_tabs[current_tab].procedure->version_names.end(), version_mc + (generator_type == 0 ? "Forge" : "Fabric")) == open_tabs[current_tab].procedure->version_names.end();
                    }
                    else if (versiontype == GLOBALTRIGGER) {
                        should_add = std::find(open_tabs[current_tab].globaltrigger->version_names.begin(), open_tabs[current_tab].globaltrigger->version_names.end(), version_mc + (generator_type == 0 ? "Forge" : "Fabric")) == open_tabs[current_tab].globaltrigger->version_names.end();
                    }
                    else if (versiontype == DATALIST) {
                        should_add = std::find(open_tabs[current_tab].datalist->version_names.begin(), open_tabs[current_tab].datalist->version_names.end(), version_mc + (generator_type == 0 ? "Forge" : "Fabric")) == open_tabs[current_tab].datalist->version_names.end();
                    }
                    else {
                        should_add = true;
                    }
                    if (ImGui::Button("Add", { 100, 30 }) && should_add) {
                        if (versiontype == GLOBALTRIGGER && !tooltip) {
                            open_tabs[current_tab].globaltrigger->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            open_tabs[current_tab].globaltrigger->version_names.push_back(version_mc + (generator_type == 0 ? "Forge" : "Fabric"));
                            addversion = false;
                        }
                        else if (versiontype == PROCEDURE) {
                            open_tabs[current_tab].procedure->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            open_tabs[current_tab].procedure->version_names.push_back(version_mc + (generator_type == 0 ? "Forge" : "Fabric"));
                            addversion = false;
                        }
                        else if (versiontype == DATALIST) {
                            open_tabs[current_tab].datalist->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            open_tabs[current_tab].datalist->version_names.push_back(version_mc + (generator_type == 0 ? "Forge" : "Fabric"));
                            addversion = false;
                        }
                        else if (versiontype == API) {
                            open_tabs[current_tab].api->versions.push_back({ version_mc, (generator_type == 0 ? "Forge" : "Fabric") });
                            open_tabs[current_tab].api->version_names.push_back(version_mc + (generator_type == 0 ? "Forge" : "Fabric"));
                            addversion = false;
                        }
                    }
                    if (tooltip && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Global triggers not supported for Fabric generator.");
                        ImGui::EndTooltip();
                    }
                    else if (!should_add && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("A template of this version already exists!");
                        ImGui::EndTooltip();
                    }
                    ImGui::End();
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
                    bool should_add = std::find(open_tabs[current_tab].procedure->component_names.begin(), open_tabs[current_tab].procedure->component_names.end(), component_name) == open_tabs[current_tab].procedure->component_names.end();
                    if (ImGui::Button("Add", { 100, 30 }) && should_add) {
                        Plugin::Component comp;
                        comp.name = component_name;
                        open_tabs[current_tab].procedure->component_names.push_back(comp.name);
                        open_tabs[current_tab].procedure->components.push_back(comp);
                        addcomp = false;
                    }
                    if (!should_add && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("A component with this name already exists!");
                        ImGui::EndTooltip();
                    }
                    ImGui::End();
                }
            }
            else {
                addcomp_set_pos = true;
                if (!component_name.empty())
                    component_name.clear();
            }

            if (help_procedures) {
                if (help_procedures_set_pos) {
                    if (!ImGui::IsPopupOpen("Procedures help"))
                        ImGui::OpenPopup("Procedures help");
                    ImGui::SetNextWindowPos({ (float)(GetScreenWidth() - 400) / 2, (float)(GetScreenHeight() - 500) / 2 });
                }
                help_procedures_set_pos = false;
                ImGui::SetNextWindowSize({ 400, 500 }, ImGuiCond_Once);
                if (ImGui::BeginPopupModal("Procedures help", &help_procedures, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                    if (ImGui::BeginTabBar("pchelp")) {
                        if (ImGui::BeginTabItem("Colors")) {
                            ImGui::Spacing();
                            ImGui::Text("Block procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(2);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 153, 153, 96, 255 })))
                                ImGui::SetClipboardText("#999960");
                            ImGui::PopID();
                            ImGui::Text("Item procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(3);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 153, 96, 105, 255 })))
                                ImGui::SetClipboardText("#996069");
                            ImGui::PopID();
                            ImGui::Text("Math procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(4);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 96, 105, 153, 255 })))
                                ImGui::SetClipboardText("#606999");
                            ImGui::PopID();
                            ImGui::Text("Logic procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(5);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 96, 124, 153, 255 })))
                                ImGui::SetClipboardText("#607c99");
                            ImGui::PopID();
                            ImGui::Text("Entity procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(6);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 96, 138, 153, 255 })))
                                ImGui::SetClipboardText("#608a99");
                            ImGui::PopID();
                            ImGui::Text("Player procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(7);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 96, 153, 148, 255 })))
                                ImGui::SetClipboardText("#609994");
                            ImGui::PopID();
                            ImGui::Text("Direction procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(8);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 153, 115, 96, 255 })))
                                ImGui::SetClipboardText("#997360");
                            ImGui::PopID();
                            ImGui::Text("World procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(9);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 153, 129, 96, 255 })))
                                ImGui::SetClipboardText("#998160");
                            ImGui::PopID();
                            ImGui::Text("Text procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(10);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 96, 153, 134, 255 })))
                                ImGui::SetClipboardText("#609986");
                            ImGui::PopID();
                            ImGui::Text("Projectile procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(11);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 153, 96, 153, 255 })))
                                ImGui::SetClipboardText("#996099");
                            ImGui::PopID();
                            ImGui::Text("GUI procedures color: ");
                            ImGui::SameLine();
                            ImGui::PushID(12);
                            if (ImGui::ColorButton(" ", rlImGuiColors::Convert({ 105, 153, 96, 255 })))
                                ImGui::SetClipboardText("#699960");
                            ImGui::PopID();
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("Component names")) {
                            ImGui::Spacing();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
                            ImGui::Text("sourceentity: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
                            rlImGuiImage(&Sourceentity);
                            ImGui::Text("immediatesourceentity: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
                            rlImGuiImage(&Immediatesourceentity);
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                            ImGui::Text("noentity: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 20);
                            rlImGuiImage(&Noentity);
                            ImGui::Text("provideditemstack: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 15);
                            rlImGuiImage(&Provideditemstack);
                            ImGui::Text("providedblockstate: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 18);
                            rlImGuiImage(&Providedblockstate);
                            ImGui::Text("x | y | z: ");
                            ImGui::SameLine();
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 17);
                            rlImGuiImage(&Xyz);
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("MCreator images")) {
                            ImGui::Spacing();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Server-side indicator: ");
                            ImGui::SameLine();
                            if (ImGui::Button("Copy source"))
                                SetClipboardText("./res/server.png");
                            ImGui::Text("Width: 8");
                            ImGui::Text("Height: 24");
                            ImGui::NewLine();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Client-side indicator: ");
                            ImGui::SameLine();
                            ImGui::PushID(2);
                            if (ImGui::Button("Copy source"))
                                SetClipboardText("./res/client.png");
                            ImGui::PopID();
                            ImGui::Text("Width: 8");
                            ImGui::Text("Height: 24");
                            ImGui::NewLine();
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("Null value indicator: ");
                            ImGui::SameLine();
                            ImGui::PushID(3);
                            if (ImGui::Button("Copy source"))
                                SetClipboardText("./res/null.png");
                            ImGui::PopID();
                            ImGui::Text("Width: 8");
                            ImGui::Text("Height: 24");
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                    ImGui::End();
                }
            }
            else {
                help_procedures_set_pos = true;
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
                if (ImGui::CollapsingHeader("Datalists")) {
                    if (!loaded_plugin.data.datalists.empty()) {
                        for (Plugin::Datalist& datalist : loaded_plugin.data.datalists) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(datalist.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), datalist.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = DATALIST;
                                    window.datalist = &datalist;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(datalist.name);
                                }
                            }
                        }
                    }
                }
                if (ImGui::CollapsingHeader("Translations")) {
                    if (!loaded_plugin.data.translations.empty()) {
                        for (Plugin::Translation& translation : loaded_plugin.data.translations) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(translation.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), translation.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = TRANSLATION;
                                    window.translation = &translation;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(translation.name);
                                }
                            }
                        }
                    }
                }
                if (ImGui::CollapsingHeader("APIs")) {
                    if (!loaded_plugin.data.apis.empty()) {
                        for (Plugin::Api& api : loaded_plugin.data.apis) {
                            ImGui::Bullet();
                            ImGui::SameLine();
                            if (ImGui::MenuItem(api.name.c_str())) {
                                if (std::find(open_tab_names.begin(), open_tab_names.end(), api.name) == open_tab_names.end()) {
                                    TabWindow window;
                                    window.type = API;
                                    window.api = &api;
                                    open_tabs.push_back(window);
                                    open_tab_names.push_back(api.name);
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
                            int entryID = 888;
                            int mappingID = 4999;
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
                            case API:
                                ImGui::Spacing();
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("API name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tabs[i].api->name, ImGuiInputTextFlags_ReadOnly);
                                ImGui::Spacing();
                                ImGui::Text("Gradle templates");
                                if (ImGui::BeginTabBar("gradle", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable)) {
                                    if (ImGui::TabItemButton("+")) {
                                        addversion = true;
                                        versiontype = API;
                                    }
                                    for (int j = 0; j < open_tabs[i].api->versions.size(); j++) {
                                        std::string version__ = open_tabs[i].api->versions[j].first + " " + open_tabs[i].api->versions[j].second;
                                        if (ImGui::BeginTabItem(version__.c_str())) {
                                            ImGui::InputTextMultiline(" ", &open_tabs[i].api->code[open_tabs[i].api->versions[j]], { ImGui::GetColumnWidth(), 400 }, ImGuiInputTextFlags_AllowTabInput);
                                            ImGui::EndTabItem();
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                break;
                            case TRANSLATION:
                                ImGui::Spacing();
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Translation name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Localization: ");
                                ImGui::SameLine();
                                ImGui::PushID(2);
                                ImGui::InputTextWithHint(" ", "fr_FR", &open_tabs[i].translation->language, ImGuiInputTextFlags_CharsNoBlank);
                                ImGui::PopID();
                                ImGui::Spacing();
                                ImGui::Text("Translation entries");
                                if (ImGui::Button("Add entry")) {
                                    open_tabs[i].translation->keys.push_back({ "", "" });
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Remove entry")) {
                                    if (!open_tabs[i].translation->keys.empty())
                                        open_tabs[i].translation->keys.pop_back();
                                }
                                for (int l = 0; l < open_tabs[i].translation->keys.size(); l++) {
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text("Translation key: ");
                                    ImGui::SameLine();
                                    ImGui::PushID(10 + l);
                                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth() / 3);
                                    ImGui::InputText(" ", &open_tabs[i].translation->keys[l].first, ImGuiInputTextFlags_CharsNoBlank);
                                    ImGui::PopID();
                                    ImGui::SameLine();
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text("Translation text: ");
                                    ImGui::SameLine();
                                    ImGui::PushID(-10 - l);
                                    ImGui::SetNextItemWidth(ImGui::GetColumnWidth() / 1.5);
                                    ImGui::InputText(" ", &open_tabs[i].translation->keys[l].second);
                                    ImGui::PopID();
                                }
                                if (ImGui::Button("Copy keys")) {
                                    if (!translationkey_clipboard.empty() && ! open_tabs[i].translation->keys.empty())
                                        translationkey_clipboard.clear();
                                    for (int l = 0; l < open_tabs[i].translation->keys.size(); l++)
                                        translationkey_clipboard.push_back(open_tabs[i].translation->keys[l].first);
                                }
                                ImGui::SameLine();
                                if (ImGui::Button("Paste keys")) {
                                    if (!translationkey_clipboard.empty()) {
                                        for (int l = 0; l < translationkey_clipboard.size(); l++) {
                                            open_tabs[i].translation->keys.push_back({ translationkey_clipboard[l], "" });
                                        }
                                    }
                                }
                                break;
                            case DATALIST:
                                ImGui::Spacing();
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Datalist name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::Text("Datalist selector title: ");
                                ImGui::SameLine();
                                ImGui::PushID(232);
                                ImGui::InputText(" ", &open_tabs[i].datalist->title);
                                ImGui::PopID();
                                ImGui::Text("Datalist selector message: ");
                                ImGui::SameLine();
                                ImGui::PushID(4828);
                                ImGui::InputText(" ", &open_tabs[i].datalist->message);
                                ImGui::PopID();
                                ImGui::Spacing();
                                ImGui::Text("Datalist entries");
                                if (ImGui::Button("Add entry"))
                                    open_tabs[i].datalist->entries.push_back("");
                                ImGui::SameLine();
                                if (ImGui::Button("Remove entry"))
                                    if (!open_tabs[i].datalist->entries.empty())
                                        open_tabs[i].datalist->entries.pop_back();
                                if (!open_tabs[i].datalist->entries.empty()) {
                                    ImGui::BeginChild(42, { 600, 200 });
                                    for (std::string& entry : open_tabs[i].datalist->entries) {
                                        ImGui::PushID(entryID);
                                        ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
                                        ImGui::InputText(" ", &entry, ImGuiInputTextFlags_CharsNoBlank);
                                        ImGui::PopID();
                                        entryID++;
                                    }
                                    ImGui::EndChild();
                                }
                                ImGui::Spacing();
                                ImGui::Text("Entry mappings");
                                if (ImGui::BeginTabBar("mappings", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable)) {
                                    if (ImGui::TabItemButton("+")) {
                                        addversion = true;
                                        versiontype = DATALIST;
                                    }
                                    for (int j = 0; j < open_tabs[i].datalist->versions.size(); j++) {
                                        std::string tabname_dl = open_tabs[i].datalist->versions[j].first + " " + open_tabs[i].datalist->versions[j].second;
                                        if (ImGui::BeginTabItem(tabname_dl.c_str())) {
                                            ImGui::Spacing();
                                            while (open_tabs[i].datalist->entries.size() > open_tabs[i].datalist->mappings[open_tabs[i].datalist->versions[j]].size())
                                                open_tabs[i].datalist->mappings[open_tabs[i].datalist->versions[j]].push_back("");
                                            while (open_tabs[i].datalist->entries.size() > open_tabs[i].datalist->exclusions[open_tabs[i].datalist->versions[j]].size())
                                                open_tabs[i].datalist->exclusions[open_tabs[i].datalist->versions[j]].push_back(false);
                                            for (int l = 0; l < open_tabs[i].datalist->entries.size(); l++) {
                                                std::string entryStr = open_tabs[i].datalist->entries[l] + ": ";
                                                ImGui::AlignTextToFramePadding();
                                                ImGui::Text(entryStr.c_str());
                                                ImGui::SameLine();
                                                ImGui::PushID(mappingID);
                                                ImGui::InputText(" ", &open_tabs[i].datalist->mappings[open_tabs[i].datalist->versions[j]][l]);
                                                ImGui::SameLine();
                                                ImGui::Checkbox("Exclude this entry", &open_tabs[i].datalist->exclusions[open_tabs[i].datalist->versions[j]][l]);
                                                ImGui::PopID();
                                                mappingID++;
                                            }
                                            mappingID = 4999;
                                            if (ImGui::Button("Copy mappings")) {
                                                if (!mapping_clipboard.empty())
                                                    mapping_clipboard.clear();
                                                for (int l = 0; l < open_tabs[i].datalist->entries.size(); l++)
                                                    mapping_clipboard.push_back(open_tabs[i].datalist->mappings[open_tabs[i].datalist->versions[j]][l]);
                                            }
                                            ImGui::SameLine();
                                            if (ImGui::Button("Paste mappings")) {
                                                if (!mapping_clipboard.empty()) {
                                                    for (int l = 0; l < open_tabs[i].datalist->entries.size(); l++)
                                                        open_tabs[i].datalist->mappings[open_tabs[i].datalist->versions[j]][l] = mapping_clipboard[l];
                                                }
                                            }
                                            ImGui::EndTabItem();
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                break;
                            case PROCEDURE:
                                ImGui::Spacing();
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Procedure name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Procedure type: ");
                                ImGui::SameLine();
                                ImGui::PushID(1);
                                ImGui::Combo(" ", &open_tabs[i].procedure->type, "Input\0Output");
                                if (open_tabs[i].procedure->type == 1) {
                                    ImGui::AlignTextToFramePadding();
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
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Procedure color: ");
                                ImGui::SameLine();
                                ImGui::ColorEdit4(" ", cols);
                                open_tabs[i].procedure->color = { cols[0], cols[1], cols[2], cols[3] };
                                ImGui::AlignTextToFramePadding();
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
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Procedure translation key: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tabs[i].procedure->translationkey);
                                ImGui::PopID();
                                ImGui::Checkbox("Requires world dependency", &open_tabs[i].procedure->world_dependency);
                                ImGui::PushID(2394);
                                ImGui::Checkbox("Requires an external API", &open_tabs[i].procedure->requires_api);
                                ImGui::PopID();
                                if (open_tabs[i].procedure->requires_api) {
                                    ImGui::PushID(2838);
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text("API name: ");
                                    ImGui::SameLine();
                                    ImGui::InputText(" ", &open_tabs[i].procedure->api_name);
                                    ImGui::PopID();
                                }
                                ImGui::Spacing();
                                ImGui::Spacing();
                                ImGui::Text("Procedure block components");
                                if (ImGui::BeginTabBar("args", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable)) {
                                    if (ImGui::TabItemButton("+")) {
                                        addcomp = true;
                                    }
                                    for (int j = 0; j < open_tabs[i].procedure->components.size() && !open_tabs[i].procedure->components.empty(); j++) {
                                        if (ImGui::BeginTabItem(open_tabs[i].procedure->components[j].name.c_str(), &open_tabs[i].procedure->components[j].open)) {
                                            ImGui::Spacing();
                                            ImGui::AlignTextToFramePadding();
                                            ImGui::Text("Component name: ");
                                            ImGui::SameLine();
                                            ImGui::InputText(" ", &open_tabs[i].procedure->components[j].name, ImGuiInputTextFlags_CharsNoBlank);
                                            ImGui::AlignTextToFramePadding();
                                            ImGui::Text("Component type: ");
                                            ImGui::SameLine();
                                            ImGui::PushID(56);
                                            ImGui::Combo(" ", &open_tabs[i].procedure->components[j].type_int, "Input value\0Field input\0Field checkbox\0Field dropdown\0Datalist selector\0Input statement\0Dummy input\0Field image\0");
                                            ImGui::PopID();
                                            switch (open_tabs[i].procedure->components[j].type_int) {
                                            case 0:
                                                ImGui::AlignTextToFramePadding();
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
                                                if (open_tabs[i].procedure->components[j].value_int == 4) {
                                                    ImGui::PushID(88);
                                                    ImGui::AlignTextToFramePadding();
                                                    ImGui::Text("Boolean default value: ");
                                                    ImGui::SameLine();
                                                    ImGui::Combo(" ", &open_tabs[i].procedure->components[j].boolean_checked, "True\0False");
                                                    ImGui::PopID();
                                                }
                                                else if (open_tabs[i].procedure->components[j].value_int == 5) {
                                                    ImGui::PushID(88);
                                                    ImGui::AlignTextToFramePadding();
                                                    ImGui::Text("Number default value: ");
                                                    ImGui::SameLine();
                                                    ImGui::InputInt(" ", &open_tabs[i].procedure->components[j].number_default);
                                                    ImGui::PopID();
                                                }
                                                else if (open_tabs[i].procedure->components[j].value_int == 6) {
                                                    ImGui::PushID(88);
                                                    ImGui::AlignTextToFramePadding();
                                                    ImGui::Text("Text default value: ");
                                                    ImGui::SameLine();
                                                    ImGui::InputText(" ", &open_tabs[i].procedure->components[j].text_default);
                                                    ImGui::PopID();
                                                }
                                                break;
                                            case 1:
                                            case 6:
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
                                                ImGui::AlignTextToFramePadding();
                                                ImGui::Text("Datalist name: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(3);
                                                ImGui::InputText(" ", &open_tabs[i].procedure->components[j].datalist);
                                                ImGui::PopID();
                                                break;
                                            case 5:
                                                ImGui::Checkbox("Disable local variables in statement", &open_tabs[i].procedure->components[j].disable_localvars);
                                                break;
                                            case 7:
                                                ImGui::AlignTextToFramePadding();
                                                ImGui::Text("Image source: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(3883);
                                                ImGui::InputText(" ", &open_tabs[i].procedure->components[j].source);
                                                ImGui::PopID();
                                                ImGui::AlignTextToFramePadding();
                                                ImGui::Text("Image width: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(3881);
                                                ImGui::InputInt(" ", &open_tabs[i].procedure->components[j].width);
                                                ImGui::PopID();
                                                ImGui::AlignTextToFramePadding();
                                                ImGui::Text("Image height: ");
                                                ImGui::SameLine();
                                                ImGui::PushID(3880);
                                                ImGui::InputInt(" ", &open_tabs[i].procedure->components[j].height);
                                                ImGui::PopID();
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
                                                case 6:
                                                case 7:
                                                    break;
                                                }
                                                if (open_tabs[i].procedure->components[j].type_int != 6 && open_tabs[i].procedure->components[j].type_int != 7)
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
                                    for (int l = 0; l < open_tabs[i].procedure->versions.size() && !open_tabs[i].procedure->versions.empty(); l++) {
                                        std::string tabname_pc = open_tabs[i].procedure->versions[l].first + " " + open_tabs[i].procedure->versions[l].second;
                                        if (ImGui::BeginTabItem(tabname_pc.c_str())) {
                                            current_version = l;
                                            ImGui::InputTextMultiline(" ", &open_tabs[i].procedure->code[open_tabs[i].procedure->versions[l]], {ImGui::GetColumnWidth(), 400}, ImGuiInputTextFlags_AllowTabInput);
                                            old_current_version = l;
                                            ImGui::EndTabItem();
                                        }
                                    }
                                    ImGui::EndTabBar();
                                }
                                break;
                            case GLOBALTRIGGER:
                                ImGui::Spacing();
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("Trigger name: ");
                                ImGui::SameLine();
                                ImGui::InputText(" ", &open_tab_names[i], ImGuiInputTextFlags_ReadOnly);
                                ImGui::AlignTextToFramePadding();
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
                                                if (ImGui::Button("Copy code")) {
                                                    if (!triggercode_clipboard.empty())
                                                        triggercode_clipboard.clear();
                                                    for (int j = 0; j < 12; j++) {
                                                        if (open_tabs[i].globaltrigger->dependencies[j]) {
                                                            triggercode_clipboard.push_back(open_tabs[i].globaltrigger->dependency_mappings[version][loaded_plugin.Dependencies[j]]);
                                                        }
                                                    }
                                                    triggercode_clipboard.push_back(open_tabs[i].globaltrigger->event_code[version]);
                                                }
                                                ImGui::SameLine();
                                                if (ImGui::Button("Paste code")) {
                                                    if (!triggercode_clipboard.empty()) {
                                                        int n = 0;
                                                        for (int j = 0; j < 12; j++) {
                                                            if (open_tabs[i].globaltrigger->dependencies[j]) {
                                                                open_tabs[i].globaltrigger->dependency_mappings[version][loaded_plugin.Dependencies[j]] = triggercode_clipboard[n];
                                                                n++;
                                                            }
                                                        }
                                                        open_tabs[i].globaltrigger->event_code[version] = triggercode_clipboard[triggercode_clipboard.size() - 1];
                                                    }
                                                }
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

    UnloadTexture(PluginIcon);
    UnloadTexture(Sourceentity);
    UnloadTexture(Immediatesourceentity);
    UnloadTexture(Noentity);
    UnloadTexture(Provideditemstack);
    UnloadTexture(Providedblockstate);
    UnloadTexture(Xyz);

    if (pluginmenu) {
        if (mcreator_path_set) {
            if (fs::exists(pluginsdir + loaded_plugin.data.id + ".zip"))
                fs::remove(pluginsdir + loaded_plugin.data.id + ".zip");
        }
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}