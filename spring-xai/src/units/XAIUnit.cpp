#include <cassert>

#include "LegacyCpp/IAICallback.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "System/float3.h"

#include "./XAIUnit.hpp"
#include "./XAIUnitDef.hpp"
#include "../main/XAIHelper.hpp"
#include "../commands/XAICommand.hpp"
#include "../groups/XAIGroup.hpp"
#include "../tasks/XAIITask.hpp"
#include "../map/XAIThreatMap.hpp"

void XAICUnit::SetActiveState(bool wantActive) {
	if (CanGiveCommand(CMD_ONOFF)) {
		if (active != wantActive) {
			active = !active;

			Command c;
				c.id = CMD_ONOFF;
				c.params.push_back(wantActive);
			GiveCommand(c);
		}
	}
}

void XAICUnit::Init() {
	currCmdID = 0;

	age = 0;
	limboTime = 0;

	waiting = false;

	pos = ZeroVector;
	vel = ZeroVector;
	dir = ZeroVector;
}

void XAICUnit::Update() {
	assert(unitDef != 0);

	UpdatePosition();
	UpdateCommand();
	UpdateWait();

	age += 1;
}

void XAICUnit::UpdatePosition() {
	// velocity is always one frame behind
	//
	// dir cannot be derived reliably in all
	// circumstances, so just set it to zero
	// if inferred velocity is zero
	if (xaih->rcb->GetUnitPos(id) == pos) {
		limboTime += 1;
	} else {
		limboTime = 0;
	}

	vel = xaih->rcb->GetUnitPos(id) - pos;
	pos = xaih->rcb->GetUnitPos(id);
	dir = (vel != ZeroVector)? (vel / vel.Length()): ZeroVector;
	spd = (limboTime == 0)? (vel.Length() * GAME_SPEED): 0.0f;

	// the unit-handler updates *after* the threat-map does,
	// so the threat-values of enemy units are already known
	// and we can decrease them by those of our own units
	if (unitDef->isAttacker && unitDef->maxWeaponRange > 0.0f) {
		xaih->threatMap->AddThreatExt(pos, unitDef->maxWeaponRange * 1.25f, -unitDef->GetPower());
	}
}

void XAICUnit::UpdateCommand() {
	const CCommandQueue* cq = xaih->rcb->GetCurrentUnitCommands(id);
	const Command*       uc = (cq != NULL && !cq->empty())? &(cq->front()): NULL;

	currCmdID = (uc != NULL)? (uc->id): 0;
}

void XAICUnit::UpdateWait() {
	if (currCmdID == 0) {
		if (limboTime >= (GAME_SPEED * 5)) {
			// this unit has been doing nothing for at least 5
			// seconds, force it to be re-registered as idle by
			// by triggering the engine to fire a UnitIdle event
			// (which trickles down to this->group)
			Stop();
		}
	} else {
		if (group != 0 && group->GetTaskPtr() != 0) {
			if (!waiting) {
				if (group->GetTaskPtr()->IsWaiting() && (spd < (unitDef->GetMaxSpeed() * 0.75f)))
					Wait(true);
			} else {
				if (!group->GetTaskPtr()->IsWaiting())
					Wait(false);
			}
		}
	}
}



// check if this unit's CQ is non-empty
bool XAICUnit::HasCommand() const {
	const CCommandQueue* cq = xaih->rcb->GetCurrentUnitCommands(id);
	const Command*       uc = (cq != NULL && !cq->empty())? &(cq->front()): NULL;

	return (uc != NULL);
}

bool XAICUnit::CanGiveCommand(int cmdID) const {
	if (cmdID < 0) {
		// build order
		return (unitDef->HaveBuildOptionDefID(-cmdID));
	} else {
		switch (cmdID) {
			case CMD_MOVE:    { return unitDef->isMobile;             } break;
			case CMD_ATTACK:  { return unitDef->GetDef()->canAttack;  } break;
			case CMD_PATROL:  { return unitDef->GetDef()->canPatrol;  } break;
			case CMD_FIGHT:   { return unitDef->GetDef()->canFight;   } break; // and has weapons?
			case CMD_DGUN:    { return unitDef->GetDef()->canDGun;    } break; // and has dgun weapon?
			case CMD_GUARD:   { return unitDef->GetDef()->canGuard;   } break;
			case CMD_RECLAIM: { return unitDef->GetDef()->canReclaim; } break;
			case CMD_REPAIR:  { return unitDef->GetDef()->canRepair;  } break;
			case CMD_RESTORE: { return unitDef->GetDef()->canRestore; } break;
			case CMD_ONOFF:   { return unitDef->GetDef()->onoffable;  } break;
			case CMD_WAIT:    { return true;                          } break; // universal
			case CMD_STOP:    { return true;                          } break; // universal

			// this command does not exist, but canAssist still
			// needs to be checked for build-tasks and the like
			// (icw. CMD_REPAIR and / or CMD_GUARD)
			// case CMD_ASSIST:  { return unitDef->GetDef()->canAssist;  } break;

			default: {
				return false;
			} break;
		}
	}
}

int XAICUnit::GiveCommand(const Command& c) {
	const bool execute =
		(c.id != CMD_WAIT) ||
		(!unitDef->isMobile) ||
		(spd < (unitDef->GetMaxSpeed() * 0.75f));

	if (execute) {
		waiting = ((c.id == CMD_WAIT)? !waiting: false);

		XAICommand cc(id, true, c);
		return (cc.Send(xaih));
	}

	return 0;
}

int XAICUnit::TryGiveCommand(const Command& c) {
	if (CanGiveCommand(c.id)) {
		return (GiveCommand(c));
	}

	return -100;
}

void XAICUnit::Stop() { Command c; c.id = CMD_STOP; GiveCommand(c); limboTime = 0; }
void XAICUnit::Wait(bool w) { Command c; c.id = CMD_WAIT; GiveCommand(c); waiting = w; }
void XAICUnit::Move(const float3& goal) {
	if (pos != ZeroVector) {
		Command c;
			c.id = CMD_MOVE;
			c.params.push_back(goal.x);
			c.params.push_back(goal.y);
			c.params.push_back(goal.z);
		TryGiveCommand(c);
	}
}




float XAICUnit::GetPositionETA(const float3& p) {
	float length = 0.0f;
	float speed  = unitDef->GetMaxSpeed() / GAME_SPEED;

	if (speed <= 0.0f) {
		return 1e30f;
	}

	if (unitDef->GetDef()->movedata != 0) {
		length = xaih->rcb->GetPathLength(pos, p, unitDef->GetDef()->movedata->pathType);
	} else {
		length = (pos - p).Length();
	}

	if (length < 0.0f) {
		return 1e30f;
	}

	// rough ETA in frames: len in elmos, speed in elmos per frame
	return (length / speed);
}
