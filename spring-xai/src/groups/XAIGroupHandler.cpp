#include <cassert>
#include <list>

#include "LegacyCpp/IAICallback.h"

#include "./XAIGroupHandler.hpp"
#include "./XAIGroup.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../utils/XAITimer.hpp"

XAIGroup* XAICGroupHandler::NewGroup() {
	XAIGroup* g = new XAIGroup(numGroupIDs++, xaih);
	return g;
}
void XAICGroupHandler::AddGroup(XAIGroup* g) {
	if (!HasGroup(g)) {
		groupsByID[g->GetID()] = g;
	} else {
		assert(false);
	}
}
void XAICGroupHandler::DelGroup(XAIGroup* g) {
	// actual deletion should be done by caller
	if (HasGroup(g)) {
		groupsByID.erase(g->GetID());
	} else {
		assert(false);
	}
}
bool XAICGroupHandler::HasGroup(XAIGroup* g) const {
	return (groupsByID.find(g->GetID()) != groupsByID.end());
}



// get all groups that have an active task instance
std::list<XAIGroup*> XAICGroupHandler::GetTaskedGroups() const {
	std::list<XAIGroup*> groupLst;
	std::map<int, XAIGroup*>::const_iterator groupsIt;

	for (groupsIt = groupsByID.begin(); groupsIt != groupsByID.end(); groupsIt++) {
		assert(groupsIt->second != 0 && groupsIt->second->GetUnitCount() > 0);

		if (groupsIt->second->GetTaskPtr() != 0) {
			groupLst.push_back(groupsIt->second);
		}
	}

	return groupLst;
}

// get all groups that have no active task instance
std::list<XAIGroup*> XAICGroupHandler::GetNonTaskedGroups() const {
	std::list<XAIGroup*> groupLst;
	std::map<int, XAIGroup*>::const_iterator groupsIt;

	for (groupsIt = groupsByID.begin(); groupsIt != groupsByID.end(); groupsIt++) {
		assert(groupsIt->second != 0 && groupsIt->second->GetUnitCount() > 0);

		if (groupsIt->second->GetTaskPtr() == 0) {
			groupLst.push_back(groupsIt->second);
		}
	}

	return groupLst;
}

// get all groups that are currently executing a task of type <tt>
std::list<XAIGroup*> XAICGroupHandler::GetGroupsForTask(XAITaskType tt) const {
	std::list<XAIGroup*> groupLst;
	std::map<int, XAIGroup*>::const_iterator groupsIt;

	for (groupsIt = groupsByID.begin(); groupsIt != groupsByID.end(); groupsIt++) {
		const XAIITask* t = groupsIt->second->GetTaskPtr();

		if (t != 0 && t->type == tt) {
			groupLst.push_back(groupsIt->second);
		}
	}

	return groupLst;
}



void XAICGroupHandler::OnEvent(const XAIIEvent* e) {
	XAICScopedTimer t("[XAICGroupHandler::OnEvent]", xaih->timer);

	switch (e->type) {
		// units are not candidates for group assignment
		// until they are fully finished, so ignore the
		// UnitCreated event here
		case XAI_EVENT_UNIT_FINISHED: {
			const XAIUnitFinishedEvent* ee = dynamic_cast<const XAIUnitFinishedEvent*>(e);

			XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);
			XAIGroup* g = 0;

			// new units start out group-less
			u->SetGroupPtr(g);
		} break;
		case XAI_EVENT_UNIT_DESTROYED: {
			const XAIUnitDestroyedEvent* ee = dynamic_cast<const XAIUnitDestroyedEvent*>(e);

			XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);
			XAIGroup* g = u->GetGroupPtr();

			if (g != 0) {
				// updates u->group internally
				g->DelUnitMember(u);
			}
		} break;

		// same as UnitCreated
		case XAI_EVENT_UNIT_GIVEN: {
			const XAIUnitGivenEvent* ee = dynamic_cast<const XAIUnitGivenEvent*>(e);

			if (ee->newUnitTeam == xaih->rcb->GetMyTeam()) {
				XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);
				XAIGroup* g = 0;

				u->SetGroupPtr(g);
			}
		} break;
		// same as UnitDestroyed
		case XAI_EVENT_UNIT_CAPTURED: {
			const XAIUnitCapturedEvent* ee = dynamic_cast<const XAIUnitCapturedEvent*>(e);

			if (ee->oldUnitTeam == xaih->rcb->GetMyTeam()) {
				XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);
				XAIGroup* g = u->GetGroupPtr();

				if (g != 0) {
					g->DelUnitMember(u);
				}
			}
		} break;


		case XAI_EVENT_UNIT_IDLE: {
			const XAIUnitIdleEvent* ee = dynamic_cast<const XAIUnitIdleEvent*>(e);
			const XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);

			if (u->GetGroupPtr() != 0) {
				u->GetGroupPtr()->UnitIdle(ee->unitID);
			}
		} break;

		case XAI_EVENT_UPDATE: {
			this->Update();
		} break;

		default: {
		} break;
	}
}

void XAICGroupHandler::Update() {
	XAICScopedTimer t("[XAICGroupHandler::Update]", xaih->timer);

	std::map<int, XAIGroup*>::iterator groupsIt;
	std::list<int> deadGroups;
	std::list<int>::iterator deadGroupsIt;

	// update each group and bury the dead ones
	for (groupsIt = groupsByID.begin(); groupsIt != groupsByID.end(); groupsIt++) {
		if (groupsIt->second->Update()) {
			deadGroups.push_back(groupsIt->first);
		}
	}

	// remove groups that are now empty
	for (deadGroupsIt = deadGroups.begin(); deadGroupsIt != deadGroups.end(); deadGroupsIt++) {
		delete groupsByID[*deadGroupsIt]; groupsByID.erase(*deadGroupsIt);
	}
}
