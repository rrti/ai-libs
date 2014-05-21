#include "LegacyCpp/IAICallback.h"

#include "./XAIUnitDGunControllerHandler.hpp"
#include "./XAIUnitDGunController.hpp"
#include "./XAIUnitHandler.hpp"
#include "./XAIUnit.hpp"
#include "./XAIUnitDef.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"

bool XAICUnitDGunControllerHandler::AddController(int unitID) {
	if (controllers.find(unitID) == controllers.end()) {
		const XAICUnit*   unit = xaih->unitHandler->GetUnitByID(unitID);
		const XAIUnitDef* uDef = unit->GetUnitDefPtr();

		controllers[unitID] = new XAICUnitDGunController(xaih, unitID, uDef->GetDGunWeaponDef()->def);
		return true;
	}

	return false;
}

bool XAICUnitDGunControllerHandler::DelController(int unitID) {
	std::map<int, XAICUnitDGunController*>::iterator it = controllers.find(unitID);

	if (it != controllers.end()) {
		delete it->second;
		controllers.erase(it);
		return true;
	}

	return false;
}



XAICUnitDGunController* XAICUnitDGunControllerHandler::GetController(int unitID) const {
	std::map<int, XAICUnitDGunController*>::const_iterator it = controllers.find(unitID);

	if (it == controllers.end()) {
		return NULL;
	}

	return it->second;
}



void XAICUnitDGunControllerHandler::OnEvent(const XAIIEvent* e) {
	switch (e->type) {
		case XAI_EVENT_UNIT_FINISHED: {
			const XAIUnitFinishedEvent* ee = dynamic_cast<const XAIUnitFinishedEvent*>(e);

			const XAICUnit*   unit = xaih->unitHandler->GetUnitByID(ee->unitID);
			const XAIUnitDef* uDef = unit->GetUnitDefPtr();

			if (uDef->GetDGunWeaponDef() != 0) {
				AddController(ee->unitID);
			}
		} break;

		case XAI_EVENT_UNIT_DESTROYED: {
			const XAIUnitDestroyedEvent* ee = dynamic_cast<const XAIUnitDestroyedEvent*>(e);

			// silly to call for every destroyed unit, but
			// we can no longer retrieve the UnitDef here
			DelController(ee->unitID);
		} break;

		case XAI_EVENT_ENEMY_DESTROYED: {
			const XAIEnemyDestroyedEvent* ee = dynamic_cast<const XAIEnemyDestroyedEvent*>(e);

			std::map<int, XAICUnitDGunController*>::iterator it;

			for (it = controllers.begin(); it != controllers.end(); it++) {
				it->second->EnemyDestroyed(ee->attackerID, ee->unitID);
			}
		} break;

		case XAI_EVENT_UPDATE: {
			std::map<int, XAICUnitDGunController*>::iterator it;

			for (it = controllers.begin(); it != controllers.end(); it++) {
				it->second->Update();
			}
		} break;

		default: {
		} break;
	}
}
