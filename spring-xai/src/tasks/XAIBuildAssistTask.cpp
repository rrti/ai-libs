#include <cassert>

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../trackers/XAIIStateTracker.hpp"
#include "../main/XAIHelper.hpp"

bool XAIBuildAssistTask::CanAddGroupMember(XAIGroup* g) {
	if (tAssisteeUnitID == -1) {
		return false;
	}
	if ((g->GetUnitCount() + numUnits) > maxUnits) {
		return false;
	}

	assert(g != 0);
	const std::map<int, XAICUnit*>& gUnits = g->GetUnitMembers();

	// if any unit in the group that we want to add to this
	// task is both immobile and NOT an assister, reject the
	// group (ie. nanotowers are the only _static_ structures
	// that may join a task of this type)
	for (std::map<int, XAICUnit*>::const_iterator git = gUnits.begin(); git != gUnits.end(); git++) {
		const XAICUnit*   u    = git->second;
		const XAIUnitDef* uDef = u->GetUnitDefPtr();

		if (!uDef->isMobile) {
			if ((uDef->typeMask & MASK_ASSISTER_STATIC) == 0) {
				return false;
			}
		} else {
			if ((uDef->typeMask & MASK_ASSISTER_MOBILE) == 0 && (uDef->typeMask & MASK_BUILDER_MOBILE) == 0) {
				return false;
			}
		}
	}

	return (CanAssistNow(g, false));
}

void XAIBuildAssistTask::AddGroupMember(XAIGroup* g) {
	if (groups.empty()) {
		started = true;
	}

	this->XAIITask::AddGroupMember(g);

	// groups that cannot CMD_GUARD
	// should never be added to us
	assert(g->CanGiveCommand(cmd.id));
	g->GiveCommand(cmd);
}



bool XAIBuildAssistTask::CanAssistNow(const XAIGroup* g, bool fromUpdate) const {
	const XAICUnit*   assisteeUnit      = xaih->unitHandler->GetUnitByID(tAssisteeUnitID);
	const XAIUnitDef* assisteeUnitDef   = assisteeUnit->GetUnitDefPtr();
	const XAIGroup*   assisteeUnitGroup = assisteeUnit->GetGroupPtr();
	const XAIITask*   assisteeUnitTask  = 0;

	if (!fromUpdate) {
		if (g->HasUnitMember(assisteeUnit)) {
			return false;
		}
	}

	if (assisteeUnitGroup == 0) {
		return false;
	}
	if ((assisteeUnitTask = assisteeUnitGroup->GetTaskPtr()) == 0) {
		return false;
	}

	// if a unit can only repair or guard, it's
	// no use to add it to a build-assist task
	//
	// note: the task handler should prevent
	// canBeAssisted from being false for the
	// assistee
	if (!assisteeUnitDef->GetDef()->canAssist) {
		return false;
	}

	// the unit that is the object of assistance by this
	// task must be busy with a construction task itself
	// since the unit's group can receive new tasks while
	// we are running, we must abort if this is no longer
	// a build-task (factory being ordered to attack...)
	if (!fromUpdate) {
		if (assisteeUnitTask->type != XAI_TASK_BUILD_UNIT) {
			return false;
		}
	} else {
		assert(assisteeUnitTask->type == XAI_TASK_BUILD_UNIT);
	}


	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	const int         assisteeTaskUnitDefID = -(assisteeUnitTask->GetCommand().id);
	const XAIUnitDef* assisteeTaskUnitDef   = xaih->unitDefHandler->GetUnitDefByID(assisteeTaskUnitDefID);

	float assisterBuildSpeed = 0.0f;

	if (!fromUpdate) {
		// <g> is non-NULL
		if (AssistLimitReached(assisteeUnit, g, &assisterBuildSpeed)) {
			return false;
		}
	} else {
		// <g> is NULL
		AssistLimitReached(assisteeUnit, 0, &assisterBuildSpeed);
	}

	return (ecoState->CanAffordNow(assisteeTaskUnitDef, assisteeUnitTask->GetProgress(), assisterBuildSpeed, true, waiting));
}

bool XAIBuildAssistTask::AssistLimitReached(const XAICUnit* assisteeUnit, const XAIGroup* g, float* bs) const {
	const XAIUnitDef* assisteeUnitDef   = assisteeUnit->GetUnitDefPtr();   assert(assisteeUnitDef   != 0);
	const XAIGroup*   assisteeUnitGroup = assisteeUnit->GetGroupPtr();     assert(assisteeUnitGroup != 0);
	const XAIITask*   assisteeUnitTask  = assisteeUnitGroup->GetTaskPtr(); assert(assisteeUnitTask  != 0);

	const std::set<XAIGroup*>& assisteeUnitTaskGroups = assisteeUnitTask->GetGroupMembers();

	float assisterBuildCostM = 0.0f;
	float assisterBuildCostE = 0.0f;
	float assisterBuildSpeed = (g != 0)? g->GetBuildSpeed(): 0.0f;

	// dedicated assister groups
	for (std::set<XAIGroup*>::const_iterator git = groups.begin(); git != groups.end(); git++) {
		assisterBuildSpeed += (*git)->GetBuildSpeed();
		assisterBuildCostM += (*git)->GetMtlCost();
		assisterBuildCostE += (*git)->GetNrgCost();
	}

	// temporary assister groups
	for (std::set<XAIGroup*>::const_iterator git = assisteeUnitTaskGroups.begin(); git != assisteeUnitTaskGroups.end(); git++) {
		assisterBuildSpeed += (*git)->GetBuildSpeed();
	}

	if (bs != 0) {
		*bs = assisterBuildSpeed;
	}

	// need to figure out whether adding another group
	// provides any benefit, given that factories have
	// a minimum build-time of 16 (or 32?) frames (can
	// look at the build option with highest buildtime)
	//
	// if (((g->GetBuildSpeed() + tAssistSpeed) / maxBuildTime) <= TEAM_SU_INT) {
	//	return false;
	// }
	//
	// costs are skewed when an expensive start-unit is added
	// if ((assisterBuildCostM < assisteeUnitDef->GetCost('M')) { return true; }
	// if (assisterBuildCostE > assisteeUnitDef->GetCost('E')) { return true; }
	if ((numUnits >= maxUnits) || (assisterBuildSpeed > (assisteeUnitDef->GetBuildSpeed() * 10.0f))) {
		return true;
	}

	return false;
}



bool XAIBuildAssistTask::Update() {
	if (!started) {
		return false;
	}
	if (tAssisteeUnitID == -1) {
		return true;
	}

	if (!CanAssistNow(NULL, true)) {
		Wait(true);
	} else {
		Wait(false);
	}

	return false;
}

void XAIBuildAssistTask::UnitDestroyed(int unitID) {
	if (unitID == tAssisteeUnitID) {
		// make sure the next update aborts us
		tAssisteeUnitID = -1;
	}
}
