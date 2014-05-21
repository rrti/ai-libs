#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "Sim/Features/FeatureDef.h"

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"
#include "../main/XAIHelper.hpp"
#include "../main/XAIConstants.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../trackers/XAIIStateTracker.hpp"

void XAIReclaimTask::AddGroupMember(XAIGroup* g) {
	this->XAIITask::AddGroupMember(g);

	g->GiveCommand(cmd);
}

bool XAIReclaimTask::CanAddGroupMember(XAIGroup* g) {
	if ((g->GetUnitCount() + numUnits) > maxUnits) {
		return false;
	}
	if (!g->IsMobile()) {
		return false;
	}
	if (!g->CanGiveCommand(cmd.id)) {
		return false;
	}

	if (cmd.params.size() == 1) {
		assert(tReclaimeeID != -1);

		const float reclaimSpeed = g->GetReclaimSpeed() + tReclaimSpeed;
		const void* tReclaimeeDef = 0;

		float3 tReclaimeePos = ZeroVector;
		float  tReclaimTime  = 0.0f;

		if (tReclaimeeID >= MAX_UNITS) {
			tReclaimeePos = xaih->rcb->GetFeaturePos(tReclaimeeID - MAX_UNITS);
			tReclaimeeDef = xaih->rcb->GetFeatureDef(tReclaimeeID - MAX_UNITS);

			// in SU ticks; assumes reclaim has not started yet
			tReclaimTime = (tReclaimeeDef != 0)? (((const FeatureDef*) tReclaimeeDef)->reclaimTime / reclaimSpeed): 1.0f;
		} else {
			tReclaimeePos = xaih->rcb->GetUnitPos(tReclaimeeID);
			tReclaimeeDef = xaih->rcb->GetUnitDef(tReclaimeeID);
			tReclaimTime = (tReclaimeeDef != 0)? (((const UnitDef*) tReclaimeeDef)->buildTime / reclaimSpeed): 1.0f;
		}

		if (g->GetPositionETA(tReclaimeePos) < (tReclaimTime * TEAM_SU_INT_F)) {
			return true;
		}
	} else {
		assert(tReclaimeeID == -1);

		// note: need to consider more than just ETA here
		if (g->GetPositionETA(tReclaimPos) < 450.0f) {
			return true;
		}
	}

	// FIXME: always end up here
	return false;
}



bool XAIReclaimTask::Update() {
	tReclaimSpeed = 0.0f;

	for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
		tReclaimSpeed += ((*git)->GetReclaimSpeed());
	}

	if (cmd.params.size() == 1) {
		assert(tReclaimeeID != -1);

		if (tReclaimeeID >= MAX_UNITS) {
			tReclaimProgress = (1.0f - xaih->rcb->GetFeatureReclaimLeft(tReclaimeeID - MAX_UNITS));
		} else {
			tReclaimProgress = (xaih->rcb->GetUnitHealth(tReclaimeeID) / xaih->rcb->GetUnitMaxHealth(tReclaimeeID));
		}

		if (xaih->rcb->GetFeatureDef(tReclaimeeID - MAX_UNITS) == 0) {
			return true;
		} else {
			return (tReclaimProgress >= 1.0f);
		}
	} else {
		static int fIDs[MAX_FEATURES] = {0};
		return (xaih->ccb->GetFeatures(fIDs, MAX_FEATURES, tReclaimPos, tReclaimRad) == 0);
	}
}
