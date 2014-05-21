#include <cassert>
#include <climits>
#include <list>

#include "LegacyCpp/IAICallback.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "System/float3.h"

#include "./XAIGroup.hpp"
#include "../tasks/XAIITask.hpp"
#include "../units/XAIUnit.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../main/XAIHelper.hpp"
#include "../utils/XAIUtil.hpp"

XAIGroup::~XAIGroup() {
	// when a group is destroyed, make the units
	// that were a member of it group-less again
	for (std::map<int, XAICUnit*>::const_iterator it = unitsByID.begin(); it != unitsByID.end(); it++) {
		it->second->SetGroupPtr(0);
	}
	if (task != 0) {
		task->DelGroupMember(this);
	}
}

bool XAIGroup::CanAddUnitMember(XAICUnit* u) {
	if (unitsByID.empty()) {
		return true;
	}

	const XAICUnit*   curUnit    = GetLeadUnitMember();
	const XAIUnitDef* curUnitDef = curUnit->GetUnitDefPtr();
	const XAIUnitDef* newUnitDef = u->GetUnitDefPtr();

	// only allow 100% homogeneous groups
	if (newUnitDef->GetID() != curUnitDef->GetID()) {
		return false;
	}

	const MoveData* newUnitMoveData = newUnitDef->GetMoveData();
	const MoveData* curUnitMoveData = curUnitDef->GetMoveData();

	const bool b0 = (curUnitMoveData == 0 && newUnitMoveData == 0); // both static or both aircraft
	const bool b1 = (curUnitMoveData != 0 && newUnitMoveData != 0); // both {land | water | hover}

	// ensure that all units in this group have
	// the same moveType (eg. only land units or
	// only water units or only hovercraft or...
	// aircraft); we want every group to always
	// be uniformly constructed for simplicity
	//
	// this implies that units ALL have non-NULL
	// MoveData instances or they all do not
	if ( !(b0 || (b1 && (curUnitMoveData->moveType == newUnitMoveData->moveType))) ) {
		return false;
	}

	// need some measure against dispersion
	if (u->GetPositionETA(centerPos) > 450.0f) {
		return false;
	}

	return true;
}

void XAIGroup::AddUnitMember(XAICUnit* u) {
	if (!HasUnitMember(u)) {
		unitsByID[u->GetID()] = u;
		u->SetGroupPtr(this);

		UnitIdle(u->GetID());
		Update(false);
	} else {
		assert(u->GetGroupPtr() == this);
	}
}

void XAIGroup::DelUnitMember(XAICUnit* u) {
	if (HasUnitMember(u)) {
		unitsByID.erase(u->GetID());
		u->SetGroupPtr(0);

		Update(false);
	} else {
		assert(u->GetGroupPtr() != this);
	}
}

bool XAIGroup::HasUnitMember(const XAICUnit* u) const {
	return (unitsByID.find(u->GetID()) != unitsByID.end());
}

void XAIGroup::UnitIdle(int unitID) const {
	std::map<int, XAICUnit*>::const_iterator uit = unitsByID.find(unitID);

	if (uit != unitsByID.end()) {
		if (task != 0) {
			GiveTaskCommand(uit->second);
		}
	} else {
		assert(false);
	}
}


bool XAIGroup::CanMergeIdleGroup(XAIGroup* that) {
	assert(this != that);

	const XAICUnit* thisLeadUnit = this->GetLeadUnitMember();
	const XAICUnit* thatLeadUnit = that->GetLeadUnitMember();

	const XAIUnitDef* thisLeadUnitDef = thisLeadUnit->GetUnitDefPtr();
	const XAIUnitDef* thatLeadUnitDef = thatLeadUnit->GetUnitDefPtr();
	const float3& thisPos = this->centerPos;
	const float3& thatPos = that->centerPos;

	if (thisLeadUnitDef->GetID() != thatLeadUnitDef->GetID()) {
		return false;
	}

	if (thisPos.distance(thatPos) > (this->maxWeaponRange + that->maxWeaponRange)) {
		return false;
	}

	return true;
}

void XAIGroup::MergeIdleGroup(XAIGroup* that) {
	for (std::map<int, XAICUnit*>::iterator it = that->unitsByID.begin(); it != that->unitsByID.end(); it++) {
		this->unitsByID[it->first] = it->second;
		(it->second)->SetGroupPtr(this);
	}

	that->unitsByID.clear();
	Update(false);
}


float XAIGroup::GetPositionETA(const float3& pos) const {
	const float groupSpeed = moveSpeed / GAME_SPEED;           // in elmos per frame
	const float groupDist =
		(pathType == -1)?
		centerPos.distance(pos):                               // straight-line distance
		xaih->rcb->GetPathLength(centerPos, pos, pathType);    // in elmos
	const float groupETA = groupDist / (groupSpeed + 0.01f);

	if (pathType == -1) {
		// we must be a group of aircraft
		// (rather than static buildings)
		assert(moveSpeed > 0.0f);
	}
	if (groupDist < 0.0f) {
		// path was invalid
		return -1.0f;
	}

	return groupETA;
}



bool XAIGroup::Update(bool incAge) {
	// note: uniform group, so all these are
	// equal to the values for the lead unit
	centerPos      =  ZeroVector;
	buildDist      =  1e9f;
	losDist        =  0.0f;
	minWeaponRange =  1e9f;
	maxWeaponRange =  0.0f;
	moveSpeed      =  1e9f;
	buildSpeed     =  0.0f;
	reclaimSpeed   =  0.0f;
	repairSpeed    =  0.0f;
	mtlCost        =  0.0f;
	nrgCost        =  0.0f;
	pathType       = -1;
	power          =  0.0f;

	isMobile   = true;
	isAttacker = false;
	isBuilder  = false;

	const MoveData* groupMoveData = 0;

	// note: groups are uniform, so we don't need to iterate
	// over all units to calculate the total buildSpeed etc.
	for (std::map<int, XAICUnit*>::iterator it = unitsByID.begin(); it != unitsByID.end(); it++) {
		const XAICUnit*   unit    = it->second;
		const XAIUnitDef* unitDef = unit->GetUnitDefPtr();

		assert(unit->GetUnitDefPtr() != 0);
		assert(unit->GetGroupPtr() == this);

		centerPos     += (unit->GetPos());
		buildSpeed    += unitDef->GetDef()->buildSpeed;
		reclaimSpeed  += unitDef->GetDef()->reclaimSpeed;
		repairSpeed   += unitDef->GetDef()->repairSpeed;
		buildDist      = std::min(unit->GetUnitDefPtr()->GetBuildDist(), buildDist);
		losDist        = std::max(unit->GetUnitDefPtr()->GetDef()->losRadius, losDist);
		minWeaponRange = std::min(unit->GetUnitDefPtr()->minWeaponRange, minWeaponRange);
		maxWeaponRange = std::max(unit->GetUnitDefPtr()->maxWeaponRange, maxWeaponRange);
		moveSpeed      = std::min(unit->GetUnitDefPtr()->GetMaxSpeed(), moveSpeed);
		mtlCost       += unitDef->GetDef()->metalCost;
		nrgCost       += unitDef->GetDef()->energyCost;
		power         += unitDef->GetPower();

		groupMoveData = unitDef->GetMoveData();

		isMobile   = (isMobile   && unitDef->isMobile  );
		isAttacker = (isAttacker || unitDef->isAttacker);
		isBuilder  = (isBuilder  || unitDef->isBuilder );
	}

	if (isMobile) {
		assert(moveSpeed > 0.0f);
	}
	if (groupMoveData != 0) {
		pathType = groupMoveData->pathType;
	}

	if (centerPos != ZeroVector) {
		centerPos /= GetUnitCount();
	}

	if (incAge) {
		age++;
	}

	// if true, group will be deleted by handler
	return (GetUnitCount() == 0);
}



// true iif at least one unit within this group
// is capable of executing the command <cmdID>
bool XAIGroup::CanGiveCommand(int cmdID) const {
	if (unitsByID.empty())
		return false;

	const std::map<int, XAICUnit*>::const_iterator it = unitsByID.begin();
	const XAICUnit* u = it->second;

	return (u->CanGiveCommand(cmdID));
}

// only called from UnitIdle()
int XAIGroup::GiveTaskCommand(XAICUnit* u) const {
	int r = 0;

	if (task->GetProgress() > 0.0f) {
		r = u->TryGiveCommand(task->GetCommandAux());
	} else {
		r = u->TryGiveCommand(task->GetCommand());
	}

	return r;
}


// tell ALL units in this group to execute <c>
// (units that cannot comply get a guard order)
int XAIGroup::GiveCommand(const Command& c) const {
	int r = 0;

	if (CanGiveCommand(c.id)) {
		// either all units in this group can execute
		// <c>, or none can... but in the latter case,
		// ::CanGiveCommand() should always have been
		// called before GiveCommand(c) to ensure this
		// does not happen (there should always be >=
		// 1 unit capable of executing the command per
		// group)
		for (std::map<int, XAICUnit*>::const_iterator uit = unitsByID.begin(); uit != unitsByID.end(); uit++) {
			r += (uit->second)->GiveCommand(c);
		}
	} else {
		if (task != 0) {
			// we can end up here when eg. a group of
			// arm_construction_vehicle is added to a
			// a BuildTask for a core_vehicle_plant
			const std::set<XAIGroup*>& groups = task->GetGroupMembers();

			for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
				if ((*git) != this) {
					if ((*git)->CanGiveCommand(c.id)) {
						// (*git) should already be executing <c>, guard it
						GiveCommand(CMD_GUARD, *git);
						break;
					}
				}
			}
		}
	}

	return r;
}


// tell ALL the units in this group to guard
// (or repair, etc.) a non-idle unit in <g> 
int XAIGroup::GiveCommand(int cmdID, XAIGroup* g) const {
	int r = -1;

	const int gCmdID = g->GetCommandID();
	const XAICUnit* gUnit = 0;

	// find a member in the other group that we
	// should assist; idle units are skipped
	for (std::map<int, XAICUnit*>::iterator uit = g->unitsByID.begin(); uit != g->unitsByID.end(); uit++) {
		if (gCmdID != 0 && uit->second->GetCurrCmdID() == gCmdID) {
			gUnit = uit->second; break;
		}
	}

	if (gUnit == 0) {
		// other group is completely idle
		return r;
	}

	// for each of our units, guard or repair one of <g>'s members
	for (std::map<int, XAICUnit*>::const_iterator uit = unitsByID.begin(); uit != unitsByID.end(); uit++) {
		XAICUnit* u = uit->second;
		Command gc;
			gc.id = cmdID;
			gc.params.push_back(gUnit->GetID());
		r += (u->TryGiveCommand(gc));
	}

	return r;
}



int XAIGroup::GetCommandID() const {
	if (task != 0) {
		return (task->GetCommand().id);
	}

	return 0;
}
int XAIGroup::GetCommandAuxID() const {
	if (task != 0) {
		return (task->GetCommandAux().id);
	}

	return 0;
}
