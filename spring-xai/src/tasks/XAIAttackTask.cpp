#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnit.hpp"
#include "../map/XAIThreatMap.hpp"

void XAIAttackTask::AddGroupMember(XAIGroup* g) {
	if (groups.empty()) {
		started = true;
	}

	this->XAIITask::AddGroupMember(g);

	// groups that cannot CMD_ATTACK
	// should never be added to us
	assert(g->CanGiveCommand(cmd.id));
	g->GiveCommand(cmd);
}

bool XAIAttackTask::CanAddGroupMember(XAIGroup* g) {
	if (tAttackeeUnitID == -1) {
		return false;
	}
	if ((g->GetUnitCount() + numUnits) > maxUnits) {
		return false;
	}

	if (!g->IsAttacker()) {
		return false;
	}
	if (!g->IsMobile()) {
		return false;
	}
	if (!g->CanGiveCommand(cmd.id)) {
		return false;
	}


	if (!groups.empty()) {
		const XAICUnit* newGroupLeadUnit = g->GetLeadUnitMember();
		const XAICUnit* curGroupLeadUnit = (*groups.begin())->GetLeadUnitMember();

		if (newGroupLeadUnit->GetUnitDefPtr()->GetID() != curGroupLeadUnit->GetUnitDefPtr()->GetID()) {
			// if we allow groups of units of type A and groups of units
			// of type B to share the same attack-task, new groups that
			// should be assigned to their own AttackTaskItem will join
			// existing ones for completely different items
			return false;
		}
	}


	const UnitDef* attackeeUnitDef = xaih->ccb->GetUnitDef(tAttackeeUnitID);

	if (attackeeUnitDef == NULL) {
		return false;
	}

	const float gETA = g->GetPositionETA(xaih->ccb->GetUnitPos(tAttackeeUnitID));
	const float dETA = (xaih->ccb->GetUnitMaxHealth(tAttackeeUnitID) - xaih->ccb->GetUnitHealth(tAttackeeUnitID)) / (age + 1);

	if (gETA < 0.0f) {
		return false;
	}
	if (gETA > dETA) {
		// if we can't arrive before the attackee
		// is likely to be destroyed, return false
	}

	// need extra checks here, or any attack
	// group can be added to any attack task
	return true;
}

bool XAIAttackTask::Update() {
	if (tAttackeeUnitID == -1) {
		return true;
	}

	const    UnitDef* sAttackeeUnitDef = xaih->ccb->GetUnitDef(tAttackeeUnitID);
	const XAIUnitDef* xAttackeeUnitDef = NULL;
	const float3&      attackeePos     = xaih->ccb->GetUnitPos(tAttackeeUnitID);

	float tPower = 0.0f;

	if (sAttackeeUnitDef == NULL) {
		// problem: if the enemy unit dies while outside of
		// LOS, we do not receive the EnemyDestroyed() event
		// so we need to poll whether the ID is still valid
		// (or make dangerous assumptions)
		tAttackeeUnitID = -1;
		return true;
	}

	xAttackeeUnitDef = xaih->unitDefHandler->GetUnitDefByID(sAttackeeUnitDef->id);

	// prevent the command from timing out
	if (((xaih->GetCurrFrame() % (GAME_SPEED)) == 0)) {
		// semi-hack: when giving an attack order over a huge distance
		// (eg. across the diagonal on CCR), a time-out seems to occur
		// before the command is completed, after which any subsequent
		// attack orders fail and cause a massive spike of UnitIdle()
		// events
		if (xaih->rcb->GetUnitDef(tAttackeeUnitID) != sAttackeeUnitDef) {
			cmdAux.params[0] = attackeePos.x;
			cmdAux.params[1] = attackeePos.y;
			cmdAux.params[2] = attackeePos.z;

			for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
				(*git)->GiveCommand(cmdAux);
				tPower += (*git)->GetPower();
			}
		} else {
			for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
				(*git)->GiveCommand(cmd);
				tPower += (*git)->GetPower();
			}
		}
	}

	const float tAttackeeCurHealth = xaih->ccb->GetUnitHealth(tAttackeeUnitID);
	const float tAttackeeMaxHealth = xaih->ccb->GetUnitMaxHealth(tAttackeeUnitID);

	tAttackProgress = (1.0f - (tAttackeeCurHealth / tAttackeeMaxHealth));
	return (tAttackProgress >= 1.0f || (tPower > 0.0f && xaih->threatMap->GetThreat(attackeePos) > tPower));
}
