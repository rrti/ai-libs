#include <cassert>
#include <list>

#include "LegacyCpp/IAICallback.h"

#include "./XAIITaskHandler.hpp"
#include "./XAIITask.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"
#include "../utils/XAITimer.hpp"

void XAIITaskHandler::AddTask(XAIITask* t) {
	if (!HasTask(t)) {
		tasksByID[t->GetID()] = t;
	} else {
		assert(false);
	}
}
void XAIITaskHandler::DelTask(XAIITask* t) {
	// actual deletion should be done by caller
	if (HasTask(t)) {
		tasksByID.erase(t->GetID());
	} else {
		assert(false);
	}
}
bool XAIITaskHandler::HasTask(XAIITask* t) const {
	return (tasksByID.find(t->GetID()) != tasksByID.end());
}



void XAIITaskHandler::OnEvent(const XAIIEvent* e) {
	XAICScopedTimer t("[XAIITaskHandler::OnEvent]", xaih->timer);

	// note: the base task handler does not act on
	// to Unit{Created, Destroyed, etc} events, as
	// symmetry with GroupHandler::OnEvent() would
	// require a Task{Created, Destroyed, etc} type
	// of custom event
	switch (e->type) {
		case XAI_EVENT_UPDATE: {
			// update the base instance part
			this->XAIITaskHandler::Update();
		} break;

		default: {
		} break;
	}
}

void XAIITaskHandler::Update() {
	/*
	XAICScopedTimer t("[XAIITaskHandler::Update]", xaih->timer);

	std::map<int, XAIITask*>::iterator currTasksIt;
	std::list<int> doneTasks;
	std::list<int>::iterator doneTasksIt;

	// let each task evaluate its state this frame
	// note: only the base instance part is updated
	for (currTasksIt = tasksByID.begin(); currTasksIt != tasksByID.end(); currTasksIt++) {
		XAIITask* task = currTasksIt->second;

		// note: should there be some kind of
		// special response to aborted tasks?
		if (task->XAIITask::Update()) {
			doneTasks.push_back(currTasksIt->first);
		}
	}

	// remove tasks that are now finished
	for (doneTasksIt = doneTasks.begin(); doneTasksIt != doneTasks.end(); doneTasksIt++) {
		delete tasksByID[*doneTasksIt]; tasksByID.erase(*doneTasksIt);
	}
	*/
}
