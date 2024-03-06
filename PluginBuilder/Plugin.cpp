#include "Plugin.h"

void Plugin::ConvertProcedure(Procedure* procedure, float oldVersion, float currentVersion) {

	if (oldVersion < 1.3) {
		if (procedure->category > 15) {
			std::vector<std::string> categories = { "Block data", "Block management (2023.4+ = Block actions)", "Command parameters", "Direction procedures", "Energy & fluid tanks", "Entity data", "Entity management (2023.4+ = Entity actions)", "Item procedures (2023.4+ = Item actions)", "Player data", "Player procedures (2023.4+ = Player actions)", "Projectile procedures", "Slot & GUI procedures", "World data", "World management (2023.4+ = World actions)", "Minecraft components", "Flow control", "Logic", "Math", "Text", "Advanced", "Damage procedures (2023.4+)", "Item data (2023.4+)", "World scoreboard (2023.4+)" };
			for (const Plugin::Category ct : data.categories)
				categories.push_back(ct.name);

			for (int i = 0; i < categories.size(); i++) {
				if (categories.at(i) == procedure->category_name) {
					procedure->category = i;
					break;
				}
			}
		}
		
	}

}

void Plugin::ConvertCategory(Category* category, float oldVersion, float currentVersion) {

	if (oldVersion < 1.3) {
		if (category->ischild && category->parent_int > 11)
			category->parent_int += 3;
	}

}