#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnit.hpp"
#include "../trackers/XAIIStateTracker.hpp"

void XAIBuildTask::AddGroupMember(XAIGroup* g) {
	this->XAIITask::AddGroupMember(g);

	g->GiveCommand(cmd);
}

bool XAIBuildTask::CanAddGroupMember(XAIGroup* g) {
	if (!started) {
		if (cmd.params.size() >= 3 && g->IsMobile() && g->GetBuildSpeed() > 0.0f) {
			const float3 buildPos = float3(cmd.params[0], cmd.params[1], cmd.params[2]);

			float sumGroupBS  = 0.0f; // build-speed
			float sumGroupETA = 0.0f; // in frames

			for (std::set<XAIGroup*>::const_iterator it = groups.begin(); it != groups.end(); it++) {
				sumGroupETA += (*it)->GetPositionETA(buildPos);
				sumGroupBS  += (*it)->GetBuildSpeed();
			}

			const XAIUnitDef* buildeeDef = xaih->unitDefHandler->GetUnitDefByID(-cmd.id);
			const float avgGroupETA = sumGroupETA / groups.size();
			const float btFrames = buildeeDef->GetBuildTimeFrames(sumGroupBS);

			if ((avgGroupETA + btFrames) > g->GetPositionETA(buildPos)) {
				return true;
			}
		}

		return false;
	}
	if (tBuildeeUnitID == -1) {
		// construction has not started yet, or was aborted
		return false;
	}
	if ((g->GetUnitCount() + numUnits) > maxUnits) {
		return false;
	}
	if (!g->IsMobile()) {
		// note: need an AssistTask for nano-tower groups
		return false;
	}
	if (g->GetBuildSpeed() <= 0.0f && g->GetRepairSpeed() <= 0.0f) {
		return false;
	}

	float gETA = g->GetPositionETA(xaih->rcb->GetUnitPos(tBuildeeUnitID));

	if (gETA < 0.0f) {
		return false;
	}

	gETA -= (g->GetBuildDist() / (g->GetMaxMoveSpeed() / GAME_SPEED));
	gETA  = std::max(0.0f, gETA);

	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	const XAIUnitDef* tBuildeeDef = xaih->unitDefHandler->GetUnitDefByID(-cmd.id);

	// we have the remaining build-time in frames and
	// the group's ETA to the build position in frames
	// decide whether build-assisting is "worth it"
	if ( gETA > tBuildTime         ) { return false; }
	if ((gETA / tBuildTime) > 0.75f) { return false; }

	if (tBuildeeDef->GetBuildTimeFrames(tBuildSpeed) <= TEAM_SU_INT_F) {
		return false;
	}

	return (ecoState->CanAffordNow(tBuildeeDef, tBuildProgress, (g->GetBuildSpeed() + tBuildSpeed), started, waiting));
}



bool XAIBuildTask::Update() {
	// this task is done when the object being built
	// reaches 100% build-progress (or full health)
	//
	// issues:
	//   need to implement GetBuildProgress() in the AI interface
	//   need some way to abort early if eg. unit move orders fail
	//
	const XAIUnitDef* tBuildeeDef = xaih->unitDefHandler->GetUnitDefByID(-cmd.id);
	const float3      tBuildeePos = (cmd.params.empty())? float3(-1.0f, 0.0f, 0.0f): float3(cmd.params[0], cmd.params[1], cmd.params[2]);

	if (!started) {
		if (tBuildeePos.x >= 0.0f && (!xaih->rcb->CanBuildAt(tBuildeeDef->GetDef(), tBuildeePos, 0))) {
			// somewhat hackish, this should not happen (but it does, spots
			// can get picked that have already been claimed by other groups)
			return true;
		} else {
			return false;
		}
	}

	if (tBuildeeUnitID == -1) {
		// if we got past the started check, then we
		// we must have received a UnitCreated event
		// that set <tBuildeeUnitID> to a value != -1;
		// since only UnitDestroyed events can reset
		// it to -1 we just let the task abort here
		return true;
	}

	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());

	const float tBuildeeCurHealth = xaih->rcb->GetUnitHealth(tBuildeeUnitID);
	const float tBuildeeMaxHealth = xaih->rcb->GetUnitMaxHealth(tBuildeeUnitID);

	tBuildProgress = tBuildeeCurHealth / tBuildeeMaxHealth;
	tBuildSpeed    = 0.0f;

	for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
		tBuildSpeed += ((*git)->GetBuildSpeed());
	}

	tBuildTime = tBuildeeDef->GetBuildTimeFrames(tBuildSpeed) * (1.0f - tBuildProgress);

	if (!ecoState->CanAffordNow(tBuildeeDef, tBuildProgress, tBuildSpeed, started, waiting)) {
		Wait(true);
	} else {
		Wait(false);
	}

	if (cmdAux.id == CMD_RECLAIM) {
		// todo: add an "are we still within reclaim-range?" check
		const UnitDef* attackerDef = xaih->ccb->GetUnitDef(int(cmdAux.params[0]));
		const float attackerHealth = xaih->ccb->GetUnitHealth(int(cmdAux.params[0]));

		if ((attackerDef == NULL || attackerHealth <= 0.0f)) {
			if (tBuildeeUnitID != -1) {
				cmdAux.id = CMD_REPAIR;
				cmdAux.params.clear();
				cmdAux.params.push_back(tBuildeeUnitID);
			} else {
				// note: this can also mean construction
				// had not started yet and builders were
				// en-route when attacked
				cmdAux.id = CMD_STOP;
				cmdAux.params.clear();
			}
		}
	}

	return (tBuildeeCurHealth >= tBuildeeMaxHealth);
}



void XAIBuildTask::UnitCreated(int buildeeID, int buildeeDefID, int builderID) {
	tBuildeeUnitID = buildeeID;
	tBuilderUnitID = builderID;

	cmdAux.id = CMD_REPAIR;
	cmdAux.params.clear();
	cmdAux.params.push_back(buildeeID);

	started = true;
	startFrame = xaih->GetCurrFrame();
}

void XAIBuildTask::UnitDestroyed(int unitID) {
	if (unitID != tBuildeeUnitID) {
		return;
	}

	// make sure the next update aborts us
	tBuildeeUnitID = -1;

	cmdAux.id = CMD_STOP;
	cmdAux.params.clear();
}

void XAIBuildTask::UnitDamaged(int unitID, int attackerID) {
	if (cmdAux.id != CMD_RECLAIM) {
		cmdAux.id = CMD_RECLAIM;
		cmdAux.params.clear();
		cmdAux.params.push_back(attackerID);
	}

	const float3& attackerPos = xaih->ccb->GetUnitPos(int(cmdAux.params[0]));

	for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
		XAIGroup* g = *git;
		XAICUnit* u = g->GetLeadUnitMember();

		const float3& gCenterPos = g->GetPos();
		const float   gBuildDist = g->GetBuildDist();

		if ((attackerPos - gCenterPos).SqLength() > (gBuildDist * gBuildDist)) {
			// this group is not currently within reclaim-distance
			continue;
		}
		if (u->GetCurrCmdID() == cmdAux.id) {
			// the group (of which the lead unit is part) is already reclaiming; we
			// cannot use g->GetCommand*ID() since those return t->GetCommand*().id
			continue;
		}
		if (!g->CanGiveCommand(cmdAux.id)) {
			continue;
		}

		// if the building's completion time is less than the
		// enemy reclaim time, don't give this order? however,
		// builders should always value their lives...
		g->GiveCommand(cmdAux);
	}
}
