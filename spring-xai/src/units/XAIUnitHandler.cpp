#include <list>
#include <cassert>

#include "LegacyCpp/IAICallback.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/CommandAI/CommandQueue.h"

#include "./XAIUnitHandler.hpp"
#include "./XAIUnit.hpp"
#include "./XAIUnitDef.hpp"
#include "./XAIUnitDefHandler.hpp"
#include "../main/XAIHelper.hpp"
#include "../events/XAIIEvent.hpp"
#include "../utils/XAITimer.hpp"

// return a list of (finished!) units that are assigned to some group
std::list<XAICUnit*> XAICUnitHandler::GetGroupedUnits() const {
	std::list<XAICUnit*> unitLst;

	for (std::set<int>::const_iterator it = finishedUnitsByID.begin(); it != finishedUnitsByID.end(); it++) {
		if ((unitsByID[*it]->GetGroupPtr()) != 0) {
			unitLst.push_back(unitsByID[*it]);
		}
	}

	return unitLst;
}

// return a list of (finished!) units that are not assigned to any group
std::list<XAICUnit*> XAICUnitHandler::GetNonGroupedUnits() const {
	std::list<XAICUnit*> unitLst;

	for (std::set<int>::const_iterator it = finishedUnitsByID.begin(); it != finishedUnitsByID.end(); it++) {
		if ((unitsByID[*it]->GetGroupPtr()) == 0) {
			unitLst.push_back(unitsByID[*it]);
		}
	}

	return unitLst;
}



unsigned int XAICUnitHandler::GetUnitIDsNearPosByTypeMask(const float3& p, float rSq, std::list<int>* unitIDs, unsigned int typeMask) const {
	unsigned int numUnits = 0;

	const std::set<int>& s = GetUnitIDsByMaskApprox(typeMask, 0, 0);

	for (std::set<int>::const_iterator sit = s.begin(); sit != s.end(); sit++) {
		if ((unitsByID[*sit]->GetPos() - p).SqLength() < rSq) {
			if (unitIDs != 0) {
				unitIDs->push_back(*sit);
			}

			numUnits += 1;
		}
	}

	/*
	// NOTE: far too expensive in this form (need spatial partitioning)
	for (std::set<int>::const_iterator it = createdUnitsByID.begin(); it != createdUnitsByID.end(); it++) {
		const XAICUnit*   u    = unitsByID[*it];
		const XAIUnitDef* uDef = u->GetUnitDefPtr();

		if ((uDef->typeMask & typeMask) != 0) {
			if ((unitsByID[*it]->GetPos() - p).SqLength() < rSq) {
				if (unitIDs != 0) {
					unitIDs->push_back(*it);
				}

				numUnits += 1;
			}
		}
	}

	for (std::set<int>::const_iterator it = finishedUnitsByID.begin(); it != finishedUnitsByID.end(); it++) {
		const XAICUnit*   u    = unitsByID[*it];
		const XAIUnitDef* uDef = u->GetUnitDefPtr();

		if ((uDef->typeMask & typeMask) != 0) {
			if ((unitsByID[*it]->GetPos() - p).SqLength() < rSq) {
				if (unitIDs != 0) {
					unitIDs->push_back(*it);
				}

				numUnits += 1;
			}
		}
	}
	*/

	return numUnits;
}



void XAICUnitHandler::OnEvent(const XAIIEvent* e) {
	switch (e->type) {
		case XAI_EVENT_UNIT_CREATED: {
			UnitCreated(dynamic_cast<const XAIUnitCreatedEvent*>(e));
		} break;

		case XAI_EVENT_UNIT_FINISHED: {
			UnitFinished(dynamic_cast<const XAIUnitFinishedEvent*>(e));
		} break;

		case XAI_EVENT_UNIT_DESTROYED: {
			UnitDestroyed(dynamic_cast<const XAIUnitDestroyedEvent*>(e));
		} break;

		case XAI_EVENT_UNIT_DAMAGED: {
			UnitDamaged(dynamic_cast<const XAIUnitDamagedEvent*>(e));
		} break;

		case XAI_EVENT_UNIT_IDLE: {
			UnitIdle(dynamic_cast<const XAIUnitIdleEvent*>(e));
		} break;


		case XAI_EVENT_UNIT_GIVEN: {
			UnitGiven(dynamic_cast<const XAIUnitGivenEvent*>(e));
		} break;

		case XAI_EVENT_UNIT_CAPTURED: {
			UnitCaptured(dynamic_cast<const XAIUnitCapturedEvent*>(e));
		} break;


		case XAI_EVENT_INIT: {
			unitsByID.resize(MAX_UNITS, NULL);

			for (int i = 0; i < MAX_UNITS; i++) {
				unitsByID[i] = new XAICUnit(i, xaih);
			}
		} break;

		case XAI_EVENT_UPDATE: {
			Update();
		} break;

		case XAI_EVENT_RELEASE: {
			for (int i = 0; i < MAX_UNITS; i++) {
				delete unitsByID[i]; unitsByID[i] = NULL;
			}

			unitsByID.clear();
		} break;

		default: {
		} break;
	}
}

void XAICUnitHandler::Update() {
	XAICScopedTimer t("[XAICUnitHandler::Update]", xaih->timer);

	std::list<int> undamagedUnitsByID;
	std::list<int> activeUnitsByID;
	std::list<int>::const_iterator it;

	if (!damagedUnitsByID.empty()) {
		for (std::set<int>::const_iterator sit = damagedUnitsByID.begin(); sit != damagedUnitsByID.end(); sit++) {
			const float h = xaih->rcb->GetUnitHealth(*sit);

			if (h >= xaih->rcb->GetUnitMaxHealth(*sit)) {
				undamagedUnitsByID.push_back(*sit);
			}
		}

		for (it = undamagedUnitsByID.begin(); it != undamagedUnitsByID.end(); it++) {
			damagedUnitsByID.erase(*it);
		}
	}

	if (!idleUnitsByID.empty()) {
		// units with a non-empty command
		// queue are by definition not idle
		for (std::set<int>::const_iterator sit = idleUnitsByID.begin(); sit != idleUnitsByID.end(); sit++) {
			const CCommandQueue* q = xaih->rcb->GetCurrentUnitCommands(*sit);

			if (q != NULL && !q->empty()) {
				activeUnitsByID.push_back(*sit);
			}
		}

		for (it = activeUnitsByID.begin(); it != activeUnitsByID.end(); it++) {
			idleUnitsByID.erase(*it);
		}
	}

	// created and finished units together cover all
	// other sets, so we only need to update these
	for (std::set<int>::iterator it = createdUnitsByID.begin(); it != createdUnitsByID.end(); it++) {
		assert(unitsByID[*it]->GetUnitDefPtr() != 0); unitsByID[*it]->Update();
	}
	for (std::set<int>::iterator it = finishedUnitsByID.begin(); it != finishedUnitsByID.end(); it++) {
		assert(unitsByID[*it]->GetUnitDefPtr() != 0); unitsByID[*it]->Update();
	}
}



void XAICUnitHandler::UnitCreated(const XAIUnitCreatedEvent* ee) {
	// this should _never_ fail
	assert(createdUnitsByID.find(ee->unitID) == createdUnitsByID.end());

	// new units start life being idle
	createdUnitsByID.insert(ee->unitID);
	idleUnitsByID.insert(ee->unitID);

	const UnitDef*    sprUnitDef = xaih->rcb->GetUnitDef(ee->unitID);
	const XAIUnitDef* xaiUnitDef = xaih->unitDefHandler->GetUnitDefByID(sprUnitDef->id);

	const unsigned int typeM = xaiUnitDef->typeMask;
	const unsigned int terrM = xaiUnitDef->terrainMask;
	const unsigned int weapM = xaiUnitDef->weaponMask;

	unitsByID[ee->unitID]->Init();
	unitsByID[ee->unitID]->SetUnitDefPtr(xaiUnitDef);


	if (unitsByUnitDefID.find(xaiUnitDef->GetID()) == unitsByUnitDefID.end()) {
		unitsByUnitDefID[xaiUnitDef->GetID()] = std::set<XAICUnit*>();
	}

	unitsByUnitDefID[xaiUnitDef->GetID()].insert(unitsByID[ee->unitID]);


	if (unitsByTypeMask.find(typeM) == unitsByTypeMask.end()) { unitsByTypeMask[typeM] = std::set<int>(); }
	if (unitsByTerrMask.find(terrM) == unitsByTerrMask.end()) { unitsByTerrMask[terrM] = std::set<int>(); }
	if (unitsByWeapMask.find(weapM) == unitsByWeapMask.end()) { unitsByWeapMask[terrM] = std::set<int>(); }

	unitsByTypeMask[typeM].insert(ee->unitID);
	unitsByTerrMask[terrM].insert(ee->unitID);
	unitsByWeapMask[weapM].insert(ee->unitID);
}

void XAICUnitHandler::UnitFinished(const XAIUnitFinishedEvent* ee) {
	// UnitCreated is _always_ triggered
	// before a possible UnitFinished is
	// nevertheless, play it safe
	//
	// assert(createdUnitsByID.find(ee->unitID) != createdUnitsByID.end());
	//
	if (createdUnitsByID.find(ee->unitID) == createdUnitsByID.end()) {
		XAIUnitCreatedEvent e;
			e.type      = XAI_EVENT_UNIT_CREATED;
			e.unitID    = ee->unitID;
			e.builderID = -1;
			e.frame     = ee->frame;
		UnitCreated(&e);
	}

	createdUnitsByID.erase(ee->unitID);
	finishedUnitsByID.insert(ee->unitID);
}


void XAICUnitHandler::UnitDestroyed(const XAIUnitDestroyedEvent* ee) {
	const bool b0 = (createdUnitsByID.find(ee->unitID) != createdUnitsByID.end());
	const bool b1 = (finishedUnitsByID.find(ee->unitID) != finishedUnitsByID.end());
	const bool b2 = (damagedUnitsByID.find(ee->unitID) != damagedUnitsByID.end());
	const bool b3 = (idleUnitsByID.find(ee->unitID) != idleUnitsByID.end());

	assert(b0 || b1);

	if (b0) { createdUnitsByID.erase(ee->unitID); }
	if (b1) { finishedUnitsByID.erase(ee->unitID); }
	if (b2) { damagedUnitsByID.erase(ee->unitID); }
	if (b3) { idleUnitsByID.erase(ee->unitID); }

	const XAIUnitDef* xaiUnitDef = unitsByID[ee->unitID]->GetUnitDefPtr();

	// don't bother erasing the set itself from the map, just leave it
	// empty (no way to tell the difference via GetUnitsByUnitDefID())
	unitsByUnitDefID[xaiUnitDef->GetID()].erase(unitsByID[ee->unitID]);

	// update the mask sets
	unitsByTypeMask[xaiUnitDef->typeMask].erase(ee->unitID);
	unitsByTerrMask[xaiUnitDef->terrainMask].erase(ee->unitID);
	unitsByWeapMask[xaiUnitDef->weaponMask].erase(ee->unitID);

	unitsByID[ee->unitID]->Init();
	unitsByID[ee->unitID]->SetUnitDefPtr(0);
}


void XAICUnitHandler::UnitGiven(const XAIUnitGivenEvent* ee) {
	if (ee->newUnitTeam == xaih->rcb->GetMyTeam()) {
		// units can not be given prior to being finished,
		// so only make sure we don't already have a unit
		// with this ID in the finished set
		// note: command queues are never transferred, so
		// register the new unit as idle immediately
		assert(finishedUnitsByID.find(ee->unitID) == finishedUnitsByID.end());
		finishedUnitsByID.insert(ee->unitID);
		idleUnitsByID.insert(ee->unitID);

		const UnitDef*    sprUnitDef = xaih->rcb->GetUnitDef(ee->unitID);
		const XAIUnitDef* xaiUnitDef = xaih->unitDefHandler->GetUnitDefByID(sprUnitDef->id);

		unitsByID[ee->unitID]->Init();
		unitsByID[ee->unitID]->SetUnitDefPtr(xaiUnitDef);
		unitsByID[ee->unitID]->Stop();

		if (unitsByUnitDefID.find(xaiUnitDef->GetID()) == unitsByUnitDefID.end()) {
			unitsByUnitDefID[xaiUnitDef->GetID()] = std::set<XAICUnit*>();
		}

		unitsByUnitDefID[xaiUnitDef->GetID()].insert(unitsByID[ee->unitID]);


		const unsigned int typeM = xaiUnitDef->typeMask;
		const unsigned int terrM = xaiUnitDef->terrainMask;
		const unsigned int weapM = xaiUnitDef->weaponMask;

		if (unitsByTypeMask.find(typeM) == unitsByTypeMask.end()) { unitsByTypeMask[typeM] = std::set<int>(); }
		if (unitsByTerrMask.find(terrM) == unitsByTerrMask.end()) { unitsByTerrMask[terrM] = std::set<int>(); }
		if (unitsByWeapMask.find(weapM) == unitsByWeapMask.end()) { unitsByWeapMask[terrM] = std::set<int>(); }

		unitsByTypeMask[typeM].insert(ee->unitID);
		unitsByTerrMask[terrM].insert(ee->unitID);
		unitsByWeapMask[weapM].insert(ee->unitID);
	}
}

void XAICUnitHandler::UnitCaptured(const XAIUnitCapturedEvent* ee) {
	if (ee->oldUnitTeam == xaih->rcb->GetMyTeam()) {
		// if the unit was ours, we must have known about it
		const bool b0 = (createdUnitsByID.find(ee->unitID) != createdUnitsByID.end());
		const bool b1 = (finishedUnitsByID.find(ee->unitID) != finishedUnitsByID.end());
		const bool b2 = (damagedUnitsByID.find(ee->unitID) != damagedUnitsByID.end());
		const bool b3 = (idleUnitsByID.find(ee->unitID) != idleUnitsByID.end());

		// under-construction units can be captured too
		assert(b0 || b1);

		if (b0) { createdUnitsByID.erase(ee->unitID); }
		if (b1) { finishedUnitsByID.erase(ee->unitID); }
		if (b2) { damagedUnitsByID.erase(ee->unitID); }
		if (b3) { idleUnitsByID.erase(ee->unitID); }

		const XAIUnitDef* xaiUnitDef = unitsByID[ee->unitID]->GetUnitDefPtr();

		unitsByUnitDefID[xaiUnitDef->GetID()].erase(unitsByID[ee->unitID]);

		// update the mask sets
		unitsByTypeMask[xaiUnitDef->typeMask].erase(ee->unitID);
		unitsByTerrMask[xaiUnitDef->terrainMask].erase(ee->unitID);
		unitsByWeapMask[xaiUnitDef->weaponMask].erase(ee->unitID);

		unitsByID[ee->unitID]->Init();
		unitsByID[ee->unitID]->SetUnitDefPtr(0);
	}
}


void XAICUnitHandler::UnitDamaged(const XAIUnitDamagedEvent* ee) {
	const bool b0 = (createdUnitsByID.find(ee->unitID) != createdUnitsByID.end());
	const bool b1 = (finishedUnitsByID.find(ee->unitID) != finishedUnitsByID.end());

	assert(b0 || b1);

	damagedUnitsByID.insert(ee->unitID);
}

void XAICUnitHandler::UnitIdle(const XAIUnitIdleEvent* ee) {
	const CCommandQueue* q = xaih->rcb->GetCurrentUnitCommands(ee->unitID);

	const bool b0 = (createdUnitsByID.find(ee->unitID) != createdUnitsByID.end());
	const bool b1 = (finishedUnitsByID.find(ee->unitID) != finishedUnitsByID.end());
	const bool b2 = (q == NULL || q->empty());

	assert((b0 || b1) && b2);

	idleUnitsByID.insert(ee->unitID);
}
