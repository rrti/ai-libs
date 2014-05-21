#include <cassert>
#include <list>

#include "./XAIITask.hpp"
#include "../groups/XAIGroup.hpp"

XAIITask::~XAIITask() {
	Wait(false);

	// when a task is destroyed, make the groups
	// that were a member of it task-less again
	Command c; c.id = CMD_STOP;

	for (std::set<XAIGroup*>::const_iterator it = groups.begin(); it != groups.end(); it++) {
		(*it)->SetTaskPtr(0);
		(*it)->GiveCommand(c);
	}
}

void XAIITask::AddGroupMember(XAIGroup* g) {
	groups.insert(g);
	g->SetTaskPtr(this);
}
void XAIITask::DelGroupMember(XAIGroup* g) {
	groups.erase(g);
	g->SetTaskPtr(0);
}
bool XAIITask::HasGroupMember(XAIGroup* g) const {
	return (groups.find(g) != groups.end());
}

bool XAIITask::Update() {
	numUnits = 0;

	std::list<XAIGroup*> emptyGroups;
	std::list<XAIGroup*>::iterator emptyGroupsIt;

	// gather all the groups that are empty
	// there should never be any though, as
	// ~XAIGroup calls DelGroupMember first
	// (groupHandler updates before we do)
	for (std::set<XAIGroup*>::const_iterator it = groups.begin(); it != groups.end(); it++) {
		if ((*it)->GetUnitCount() == 0) {
			assert(false);
			emptyGroups.push_back(*it);
		} else {
			assert((*it)->GetTaskPtr() == this);
			numUnits += ((*it)->GetUnitCount());
		}
	}

	for (emptyGroupsIt = emptyGroups.begin(); emptyGroupsIt != emptyGroups.end(); emptyGroupsIt++) {
		groups.erase(*emptyGroupsIt);
	}

	age++;

	if (waiting) {
		waitTime++;
	} else {
		waitTime = 0;
	}

	// if true, task will be deleted by handler
	return (groups.empty() || (age > maxAge));
}

void XAIITask::Wait(bool wantWait) {
	Command c; c.id = CMD_WAIT;

	if (wantWait != waiting) {
		waiting = !waiting;

		for (std::set<XAIGroup*>::iterator git = groups.begin(); git != groups.end(); git++) {
			(*git)->GiveCommand(c);
		}
	}
}
