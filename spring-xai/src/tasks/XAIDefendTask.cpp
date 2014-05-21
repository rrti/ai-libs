#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "LegacyCpp/UnitDef.h"

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"
#include "../main/XAIHelper.hpp"

void XAIDefendTask::AddGroupMember(XAIGroup* g) {
	if (groups.empty()) {
		started = true;
	}

	this->XAIITask::AddGroupMember(g);

	assert(g->CanGiveCommand(cmd.id));
	g->GiveCommand(cmd);
}

bool XAIDefendTask::CanAddGroupMember(XAIGroup* g) {
	if (tDefendeeUnitID == -1) {
		return false;
	}
	if ((g->GetUnitCount() + numUnits) > maxUnits) {
		return false;
	}

	if (!g->IsMobile()) {
		return false;
	}
	if (!g->IsAttacker()) {
		// can't defend without weapons
		return false;
	}

	if (!g->CanGiveCommand(cmd.id)) {
		return false;
	}

	const UnitDef* defendeeUnitDef = xaih->rcb->GetUnitDef(tDefendeeUnitID);

	if (defendeeUnitDef == NULL) {
		return false;
	}

	const float gETA = g->GetPositionETA(xaih->ccb->GetUnitPos(tDefendeeUnitID));
	const float dETA = (xaih->ccb->GetUnitMaxHealth(tDefendeeUnitID) - xaih->ccb->GetUnitHealth(tDefendeeUnitID)) / (age + 1);

	if (gETA < 0.0f) {
		return false;
	}
	if (gETA > dETA) {
		// if we can't arrive before the defendee
		// is likely to be destroyed, return false
	}


	float groupMtlCost = g->GetMtlCost();
	float groupNrgCost = g->GetNrgCost();

	for (std::set<XAIGroup*>::const_iterator it = groups.begin(); it != groups.end(); it++) {
		groupMtlCost += (*it)->GetMtlCost();
		groupNrgCost += (*it)->GetNrgCost();
	}

	if ((defendeeUnitDef->metalCost  > 1.0f) && (groupMtlCost > (defendeeUnitDef->metalCost  * 1.25f))) { return false; }
	if ((defendeeUnitDef->energyCost > 1.0f) && (groupNrgCost > (defendeeUnitDef->energyCost * 1.25f))) { return false; }

	// need extra checks here, or any defend
	// group can be added to any defend task
	return true;
}

bool XAIDefendTask::Update() {
	if (tDefendeeUnitID == -1) {
		return true;
	}

	if (((xaih->GetCurrFrame() % (GAME_SPEED * 2)) == 0)) {
		const float3& pos = xaih->ccb->GetUnitPos(tDefendeeUnitID);

		cmdAux.params[0] = pos.x;
		cmdAux.params[1] = pos.y;
		cmdAux.params[2] = pos.z;

		for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
			XAIGroup* g = *git;

			if ((g->GetPos() - pos).SqLength() > (g->GetLOSDist() * g->GetLOSDist())) {
				g->GiveCommand(cmdAux);
			} else {
				g->GiveCommand(cmd);
			}
		}
	}

	return false;
}

void XAIDefendTask::UnitDestroyed(int unitID) {
	if (unitID != tDefendeeUnitID) {
		return;
	}

	// make sure the next update aborts us
	tDefendeeUnitID = -1;

	cmdAux.id = CMD_STOP;
	cmdAux.params.clear();
}
