#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "LegacyCpp/UnitDef.h"
#include "Sim/Weapons/WeaponDef.h"

#include "./XAIUnitDGunController.hpp"
#include "./XAIUnitHandler.hpp"
#include "./XAIUnitDef.hpp"
#include "../main/XAIHelper.hpp"
#include "../commands/XAICommand.hpp"

XAICUnitDGunController::XAICUnitDGunController(XAIHelper* h, int id, const WeaponDef* wd): ownerID(id), ownerWD(wd), xaih(h) {
	enemyUnitsByID.resize(MAX_UNITS, 0);

	// set the owner unit to hold fire (we need this since
	// FAW and RF interfere with dgun and reclaim orders)
	// (xaih->unitHandler->GetUnitByID(ownerID))->SetFireState(0);
}



void XAICUnitDGunController::EnemyDestroyed(int attackerID, int targetID) {
	// if we were dgunning or reclaiming this unit and it died
	// (if an enemy unit is reclaimed, given attackerID is 0)
	if (attackerID == 0 || state.targetID == targetID) {
		Stop();
		state.Reset(0, true);
	}
}



// problems: giving reclaim order on moving target causes commander
// to walk (messing up subsequent dgun order if target still moving)
// and does not take commander torso rotation time into account
//
void XAICUnitDGunController::TrackAttackTarget(unsigned int currentFrame) {
	if (currentFrame - state.targetSelectionFrame == 5) {
		// five sim-frames have passed since selecting target, attack
		const UnitDef* udef = xaih->ccb->GetUnitDef(state.targetID);

		const float3 curTargetPos = xaih->ccb->GetUnitPos(state.targetID);        // current target position
		const float3 curOwnerPos  = xaih->ccb->GetUnitPos(ownerID);               // current owner position

		const float3 targetDif    = (curOwnerPos - curTargetPos);
		const float  targetDist   = targetDif.Length();                           // distance to target
		const float3 targetVel    = (curTargetPos - state.oldTargetPos);

		float3 targetMoveDir      = targetVel;
		float  targetMoveSpeed    = 0.0f;

		if (targetVel != ZeroVector) {
			targetMoveSpeed = targetVel.Length() / 5.0f;                          // target speed per sim-frame during tracking interval
			targetMoveDir   = targetVel / (targetMoveSpeed * 5.0f);               // target direction of movement
		}

		const float  dgunDelay  = targetDist / ownerWD->projectilespeed;          // sim-frames needed for dgun to reach target position
		const float3 leadPos    = targetMoveDir * (targetMoveSpeed * dgunDelay);
		const float3 dgunPos    = curTargetPos + leadPos;                         // position where target will be in <dgunDelay> frames
		const float  maxRange   = xaih->rcb->GetUnitMaxRange(ownerID);
		const float  maxRangeSq = maxRange * maxRange;

		bool haveClearShot = true;
		int  commandID     = -1;

		const int cnt = xaih->unitHandler->GetUnitIDsNearPosByTypeMask(curOwnerPos, maxRangeSq, NULL, MASK_BUILDER_STATIC | MASK_BUILDER_MOBILE);

		AIHCTraceRay rayData = {
			curOwnerPos,
			targetDif / targetDist,
			maxRange,
			ownerID,
			-1,
			0
		};

		xaih->ccb->HandleCommand(AIHCTraceRayId, &rayData);

		haveClearShot =
			(rayData.hitUID == -1) ||
			((rayData.hitUID != -1) &&
			(xaih->ccb->GetUnitAllyTeam(rayData.hitUID) != xaih->rcb->GetMyAllyTeam()) &&
			(cnt == 0));

		if ((curOwnerPos - dgunPos).Length() < maxRange * 0.9f) {
			// multiply by 0.9 to ensure commander does not have to walk
			if ((xaih->rcb->GetEnergy()) >= ownerWD->energycost) {
				if (udef != NULL && !udef->weapons.empty()) {
					if (haveClearShot) {
						IssueOrder(dgunPos, commandID = CMD_DGUN, 0);
					} else {
						IssueOrder(state.targetID, commandID = CMD_ATTACK, 0);
					}
				} else {
					IssueOrder(state.targetID, commandID = CMD_CAPTURE, 0);
				}
			} else {
				if (xaih->ccb->GetUnitHealth(state.targetID) < xaih->ccb->GetUnitMaxHealth(state.targetID) * 0.5f) {
					IssueOrder(state.targetID, commandID = CMD_RECLAIM, 0);
				} else {
					IssueOrder(state.targetID, commandID = CMD_CAPTURE, 0);
				}
			}

			if (commandID == CMD_DGUN   ) { state.dgunOrderFrame    = currentFrame; }
			if (commandID == CMD_RECLAIM) { state.reclaimOrderFrame = currentFrame; }
			if (commandID == CMD_CAPTURE) { state.captureOrderFrame = currentFrame; }
		} else {
			state.Reset(currentFrame, true);
		}
	}

	state.Reset(currentFrame, false);
}

void XAICUnitDGunController::SelectTarget(unsigned int currentFrame) {
	const float3 ownerPos = xaih->rcb->GetUnitPos(ownerID);

	// if our commander is dead then position will be (0, 0, 0)
	if (ownerPos.x <= 0.0f && ownerPos.z <= 0.0f) {
		return;
	}

	// get all units within immediate (non-walking) dgun range
	const float maxRange = xaih->rcb->GetUnitMaxRange(ownerID);
	const int   numUnits = xaih->ccb->GetEnemyUnits(&enemyUnitsByID[0], ownerPos, maxRange * 0.9f);

	for (int i = 0; i < numUnits; i++) {
		const int enemyUnitID = enemyUnitsByID[i];

		if (enemyUnitID <= 0) {
			continue;
		}

		if (xaih->ccb->GetUnitHealth(enemyUnitID) <= 0.0f) {
			continue;
		}

		// check if unit still alive (needed since when UnitDestroyed()
		// triggered GetEnemyUnits() is not immediately updated as well)
		const UnitDef* enemyUnitDef = xaih->ccb->GetUnitDef(enemyUnitID);

		// don't directly pop enemy commanders
		if (enemyUnitDef && !enemyUnitDef->isCommander && !enemyUnitDef->canDGun) {
			state.targetSelectionFrame = currentFrame;
			state.targetID             = enemyUnitID;
			state.oldTargetPos         = xaih->ccb->GetUnitPos(enemyUnitID);
			return;
		}
	}
}

void XAICUnitDGunController::Update() {
	if (state.targetID != -1) {
		// we selected a target, track and attack it
		TrackAttackTarget(xaih->GetCurrFrame());
	} else {
		// we have no target yet, pick one
		SelectTarget(xaih->GetCurrFrame());
	}
}

void XAICUnitDGunController::Stop() const {
	XAICommand c(ownerID, true);
		c.id = CMD_STOP;

	c.Send(xaih);
}

bool XAICUnitDGunController::IsBusy() const {
	return (state.dgunOrderFrame > 0 || state.reclaimOrderFrame > 0 || state.captureOrderFrame > 0);
}



// need to do this differently, since
// giving orders can mess up a task we
// may be executing
void XAICUnitDGunController::IssueOrder(const float3& pos, int commandID, int keyMod) const {
	XAICommand c(ownerID, true);
		c.id = commandID;
		c.options |= keyMod;
		c.params.push_back(pos.x);
		c.params.push_back(pos.y);
		c.params.push_back(pos.z);

	c.Send(xaih);
}

void XAICUnitDGunController::IssueOrder(int target, int commandID, int keyMod) const {
	XAICommand c(ownerID, true);
		c.id = commandID;
		c.options |= keyMod;
		c.params.push_back(target);

	c.Send(xaih);
}
