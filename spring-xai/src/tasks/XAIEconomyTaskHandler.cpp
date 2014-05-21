#include <cassert>
#include <sstream>

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"
#include "LegacyCpp/UnitDef.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Features/FeatureDef.h"
#include "System/float3.h"

#include "./XAIITaskHandler.hpp"
#include "./XAIITask.hpp"
#include "./XAITaskListsParser.hpp"
#include "../events/XAIIEvent.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitDGunControllerHandler.hpp"
#include "../groups/XAIGroupHandler.hpp"
#include "../groups/XAIGroup.hpp"
#include "../resources/XAIIResourceFinder.hpp"
#include "../resources/XAIIResource.hpp"
#include "../trackers/XAIIStateTracker.hpp"
#include "../commands/XAICommand.hpp"
#include "../main/XAIHelper.hpp"
#include "../utils/XAILogger.hpp"
#include "../utils/XAITimer.hpp"
#include "../utils/XAIRNG.hpp"
#include "../utils/XAIUtil.hpp"
#include "../map/XAIThreatMap.hpp"

void XAICEconomyTaskHandler::OnEvent(const XAIIEvent* e) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::OnEvent]", xaih->timer);

	// make sure the base instance part
	// also gets to process this event
	this->XAIITaskHandler::OnEvent(e);

	switch (e->type) {
		case XAI_EVENT_INIT: {
			recResPositions = xaih->recResFinder->GetResources(true);
			extResPositions = xaih->extResFinder->GetResources(true);
		} break;

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

		case XAI_EVENT_UNIT_GIVEN: {
			UnitGiven(dynamic_cast<const XAIUnitGivenEvent*>(e));
		} break;
		case XAI_EVENT_UNIT_CAPTURED: {
			UnitCaptured(dynamic_cast<const XAIUnitCapturedEvent*>(e));
		} break;

		case XAI_EVENT_UPDATE: {
			// update the derived instance part
			this->XAICEconomyTaskHandler::Update();
		} break;

		default: {
		} break;
	}
}

void XAICEconomyTaskHandler::Update() {
	XAICScopedTimer t("[XAICEconomyTaskHandler::Update]", xaih->timer);

	if (xaih->GetCurrFrame() <= (TEAM_SU_INT_I << 1)) {
		return;
	}

	UpdateTasks();

	// NOTES:
	//    how to deal with (eg.) mobile builders that also have weapons?
	//    we don't want the handlers both vieing for control of the same unit
	//
	//    military units will never? be given economic tasks and vice versa,
	//    so there should not be any competition possible (if group->taskPtr->id
	//    is not a member of this->tasksByID then the other handler must have
	//    created it)
	//    if unit is both build- and attack-capable, the ETH will get first
	//    dibs and assign the unit to an economy-group (because it receives
	//    events first), so the MTH will not see it as non-grouped
	//
	//    HOWEVER, the call to GetNonTaskedGroups*( will also give us groups of
	//    non-economic units... would be better to create dedicated Economy and
	//    Military group types, so that the CanAddGroupMember checks would not
	//    have to be so elaborate (vice versa for MilitaryTaskHandler::Update)
	//
	//    for each non-idle group, re-assign it to different task as needed?
	//    (dealing with things like imminent stall is left to tasks themselves)
	std::list<XAIGroup*> idleGroups = xaih->groupHandler->GetNonTaskedGroups();

	AssignIdleUnitsToIdleGroups(&idleGroups);
	AssignIdleGroupsToTasks(&idleGroups);
}

void XAICEconomyTaskHandler::UpdateTasks() {
	XAICScopedTimer t("[XAICEconomyTaskHandler::UpdateTasks]", xaih->timer);

	std::map<int, XAIITask*>::iterator tasksIt;
	std::list<int> doneTasks;
	std::list<int>::iterator doneTasksIt;

	for (tasksIt = tasksByID.begin(); tasksIt != tasksByID.end(); tasksIt++) {
		XAIITask* task = tasksIt->second;

		if (task->XAIITask::Update() || task->Update()) {
			doneTasks.push_back(tasksIt->first);

			switch (task->type) {
				case XAI_TASK_BUILD_UNIT: {
					const Command& cmd = task->GetCommand();

					assert(buildTaskCountsForUnitDefID[-cmd.id] > 0);
					buildTaskCountsForUnitDefID[-cmd.id] -= 1;

					if (buildTaskCountsForUnitDefID[-cmd.id] == 0) {
						buildTaskCountsForUnitDefID.erase(-cmd.id);
					}
				} break;

				case XAI_TASK_ASSIST_UNIT: {
					const Command& taskCmd   = task->GetCommand();
					const int      taskObjID = int(taskCmd.params[0]); // GetObjectID() may already be -1

					assistTasksByStaticBuilderUnitID.erase(taskObjID);
					assistedStaticBuilderUnitIDs.erase(taskObjID);
				} break;

				default: {
				} break;
			}
		}
	}

	for (doneTasksIt = doneTasks.begin(); doneTasksIt != doneTasks.end(); doneTasksIt++) {
		delete tasksByID[*doneTasksIt]; tasksByID.erase(*doneTasksIt);
	}
}



int XAICEconomyTaskHandler::GetNonGroupedFinishedBuilders(
	std::list<XAICUnit*>* ngMobileBuilders,
	std::list<XAICUnit*>* ngStaticBuilders
) {
	int ngMobileBuilderCnt = 0;
	int ngStaticBuilderCnt = 0;

	// gather all non-grouped (finished!) mobile builders
	for (std::set<int>::iterator it = mobileBuilderUnitIDs.begin(); it != mobileBuilderUnitIDs.end(); it++) {
		XAICUnit* u  = xaih->unitHandler->GetUnitByID(*it);
		bool      uf = xaih->unitHandler->IsUnitFinished(u->GetID());

		if (uf && u->GetGroupPtr() == 0) {
			// no group always implies a unit is idle
			// (the converse is not true, since units
			// can become idle while part of a group)
			//
			// problem: internal orders make this fail
			// assert(xaih->unitHandler->IsUnitIdle(u->GetID()));
			if (xaih->unitHandler->IsUnitIdle(u->GetID())) {
				ngMobileBuilders->push_back(u);
				ngMobileBuilderCnt++;
			}
		}
	}
	// gather all non-grouped (finished!) static builders
	for (std::set<int>::iterator it = staticBuilderUnitIDs.begin(); it != staticBuilderUnitIDs.end(); it++) {
		XAICUnit* u  = xaih->unitHandler->GetUnitByID(*it);
		bool      uf = xaih->unitHandler->IsUnitFinished(u->GetID());

		if (uf && u->GetGroupPtr() == 0) {
			// assert(xaih->unitHandler->IsUnitIdle(u->GetID()));
			if (xaih->unitHandler->IsUnitIdle(u->GetID())) {
				ngStaticBuilders->push_back(u);
				ngStaticBuilderCnt++;
			}
		}
	}

	return (ngMobileBuilderCnt + ngStaticBuilderCnt);
}

void XAICEconomyTaskHandler::AssignIdleUnitsToIdleGroups(std::list<XAIGroup*>* idleGroups) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::AssignIdleUnitsToIdleGroups]", xaih->timer);

	std::list<XAICUnit*> ngMobileBuilders;
	std::list<XAICUnit*> ngStaticBuilders;
	std::list<XAIGroup*>::iterator idleGroupsIt;

	if (GetNonGroupedFinishedBuilders(&ngMobileBuilders, &ngStaticBuilders) > 0) {
		// this is hard, because the optimal group assignment
		// also depends on the tasks we want the units to run
		//
		//   1. try to find idle groups we can add these non-grouped units to first
		//   2, try to create new idle groups for these non-grouped units _second_
		//
		// note that because groups are homogeneous, builders and attackers will
		// never be assigned to the same group within the current architecture
		// (however it is possible for a group to consist of a type of unit that
		// is both build- and attack-capable)
		for (std::list<XAICUnit*>::iterator it = ngMobileBuilders.begin(); it != ngMobileBuilders.end(); it++) {
			bool unitAddedToGroup = false;

			for (idleGroupsIt = idleGroups->begin(); idleGroupsIt != idleGroups->end(); idleGroupsIt++) {
				XAIGroup* idleGroup = *idleGroupsIt;

				if (idleGroup->IsMobile() && idleGroup->CanAddUnitMember(*it)) {
					idleGroup->AddUnitMember(*it);
					unitAddedToGroup = true; break;
				}
			}

			if (!unitAddedToGroup) {
				XAIGroup* g = xaih->groupHandler->NewGroup();

				assert(g->CanAddUnitMember(*it));
				g->AddUnitMember(*it);

				xaih->groupHandler->AddGroup(g);
			}
		}


		// NOTE: probably bad to stick all static builders in
		// same group, need some kind of distance constraints
		for (std::list<XAICUnit*>::iterator it = ngStaticBuilders.begin(); it != ngStaticBuilders.end(); it++) {
			bool unitAddedToGroup = false;

			for (idleGroupsIt = idleGroups->begin(); idleGroupsIt != idleGroups->end(); idleGroupsIt++) {
				XAIGroup* idleGroup = *idleGroupsIt;

				if (!idleGroup->IsMobile() && idleGroup->CanAddUnitMember(*it)) {
					idleGroup->AddUnitMember(*it);
					unitAddedToGroup = true; break;
				}
			}

			if (!unitAddedToGroup) {
				XAIGroup* g = xaih->groupHandler->NewGroup();

				assert(g->CanAddUnitMember(*it));
				g->AddUnitMember(*it);

				xaih->groupHandler->AddGroup(g);
			}
		}
	}
}



void XAICEconomyTaskHandler::AssignIdleGroupsToTasks(std::list<XAIGroup*>* idleGroups) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::AssignIdleGroupsToTasks]", xaih->timer);

	// on base frame <i> (in [0, 30>), assign groups <i * k, i * k + k>
	const int baseFrame = (xaih->GetCurrFrame() % GAME_SPEED);
	const int numGroups = (idleGroups->size() / GAME_SPEED) + 1;
	const int baseGroup = baseFrame * numGroups;
	const int lastGroup = baseGroup + numGroups;

	int groupCntr = 0;


	std::map<int, XAIITask*>::iterator tasksIt;
	std::list<XAIGroup*>::iterator idleGroupsIt;

	for (idleGroupsIt = idleGroups->begin(); idleGroupsIt != idleGroups->end(); idleGroupsIt++) {
		XAIGroup* group = *idleGroupsIt;

		// we cannot skip units that happen to have weapons here
		// (*A Commanders are build-capable, so they would always
		// be left without a task eg.)
		if (!group->IsBuilder()) { continue; }
		if (group->IsAttacker()) { /*continue;*/ }

		if (groupCntr < baseGroup) {
			groupCntr++;
			continue;
		}
		if (groupCntr < lastGroup) {
			groupCntr++;


			for (tasksIt = tasksByID.begin(); tasksIt != tasksByID.end(); tasksIt++) {
				XAIITask* task = tasksIt->second;

				if (task->CanAddGroupMember(group)) {
					task->AddGroupMember(group); break;
				}
			}

			// if this group was not added to any existing task
			// (eg. because there were no running tasks at all)
			if (group->GetTaskPtr() == 0) {
				// check if new task can be created for group
				// based on an element in the current wishlist
				//
				// note: include repair tasks here?
				if (!xaih->GetConfig()) {
					if (!TryAddReclaimTaskForGroup(group)) {
						if (!TryAddBuildAssistTaskForGroup(group)) {
							if (!TryAddBuildTaskForGroup(group, NULL)) {
							}
						}
					}
				} else {
					if (!TryAddReclaimTaskForGroup(group)) {
						if (!TryAddBuildAssistTaskForGroup(group)) {
							if (!TryAddLuaTaskForGroup(group)) {
							}
						}
					}
				}
			}
		}
	}
}

bool XAICEconomyTaskHandler::TryAddReclaimTaskForGroup(XAIGroup* group) {
	if (!group->CanGiveCommand(CMD_RECLAIM)) {
		return false;
	}

	/*
	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	const XAIResourceState& mState = ecoState->GetResourceState('M');
	const XAIResourceState& eState = ecoState->GetResourceState('E');

	if (mState.GetLevel() > mState.GetStorage() * 0.333f) { return false; }
	if (eState.GetLevel() > eState.GetStorage() * 0.333f) { return false; }
	*/

	const XAIReclaimableResource* bestRes = 0;
	float currResDistSq = 0.0f;
	float bestResDistSq = 1e30f;

	recResPositions = xaih->recResFinder->GetResources(true);

	for (ResLstIt it = recResPositions.begin(); it != recResPositions.end(); it++) {
		const XAIReclaimableResource* res = dynamic_cast<const XAIReclaimableResource*>(*it);

		if (res->fDef == 0 || res->fDef->geoThermal) {
			continue;
		}

		currResDistSq = (res->pos - group->GetPos()).SqLength();

		if (/*mLevelLow &&*/ (res->typeMask & XAI_RESOURCETYPE_METAL ) != 0) { currResDistSq /= (res->fDef->metal  + 1.0f); }
		if (/*eLevelLow &&*/ (res->typeMask & XAI_RESOURCETYPE_ENERGY) != 0) { currResDistSq /= (res->fDef->energy + 1.0f); }

		if (currResDistSq < bestResDistSq) {
			bestResDistSq = currResDistSq; bestRes = res;
		}
	}

	if (bestRes != 0) {
		Command c;
			c.id = CMD_RECLAIM;
			// c.params.push_back(bestRes->fDef->id + MAX_UNITS);
			c.params.push_back(bestRes->pos.x);
			c.params.push_back(bestRes->pos.y);
			c.params.push_back(bestRes->pos.z);
			c.params.push_back(bestRes->fDef->metal + bestRes->fDef->energy);
		Command cAux;
			cAux.id = CMD_STOP;

		XAIITask* newTask = new XAIReclaimTask(numTaskIDs++, xaih);
		newTask->SetCommand(c, cAux);

		// note: static builders should never be added to a
		// reclaim task, or any attached build-assist task
		// will fail to assert()
		if (newTask->CanAddGroupMember(group)) {
			AddTask(newTask);
			newTask->AddGroupMember(group);

			assert(group->GetTaskPtr() != 0);
			return true;
		}

		delete newTask;
	}

	return false;
}

bool XAICEconomyTaskHandler::TryAddBuildTaskForGroup(XAIGroup* group, const XAIBuildTaskListItem* item) {
	const DefPosPair& defPosPair = GetBestUnitDefForGroup(group, item);

	const XAIUnitDef* buildeeDef = defPosPair.first;
	const float3&     buildeePos = defPosPair.second;

	if (buildeeDef == 0) {
		return false;
	}

	if (buildeePos.x <= -2.0f || buildeePos.x >= 0.0f) {
		XAIITask* newTask = new XAIBuildTask(numTaskIDs++, xaih);
		AddTask(newTask);

		// note: don't check CanAddGroupMember here, so that
		// groups of static units can start build-tasks too
		Command cAux;
		Command c;
			c.id = -buildeeDef->GetID();

			if (buildeePos.x >= 0.0f) {
				c.params.push_back(buildeePos.x);
				c.params.push_back(buildeePos.y);
				c.params.push_back(buildeePos.z);
				c.params.push_back(XAIUtil::GetBuildFacing(xaih->rcb, buildeePos)); // facing
				c.params.push_back(std::max(buildeeDef->GetDef()->xsize, buildeeDef->GetDef()->zsize)); // spacing
			}

		newTask->SetCommand(c, cAux);
		newTask->AddGroupMember(group);

		assert(group->CanGiveCommand(newTask->GetCommand().id));

		if (buildTaskCountsForUnitDefID.find(buildeeDef->GetID()) != buildTaskCountsForUnitDefID.end()) {
			buildTaskCountsForUnitDefID[buildeeDef->GetID()] += 1;
		} else {
			buildTaskCountsForUnitDefID[buildeeDef->GetID()] = 1;
		}

		assert(group->GetTaskPtr() != 0);
		return true;
	}

	return false;
}

bool XAICEconomyTaskHandler::TryAddBuildAssistTaskForGroup(XAIGroup* group) {
	const XAICUnit*   assisteeUnit    = 0;
	const XAIUnitDef* assisteeUnitDef = 0;

	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	const XAIResourceState& mState = ecoState->GetResourceState('M');
	const XAIResourceState& eState = ecoState->GetResourceState('E');


	const XAICUnit*   gUnit    = group->GetLeadUnitMember();
	const XAIUnitDef* gUnitDef = gUnit->GetUnitDefPtr();

	if (/* !(gUnitDef->GetDef())->isCommander */ true) {
		// disallow creation of dedicated assist-tasks in low-resource cases
		//
		// problem with making an exception for the Commander in *A mods to
		// always assist the first factory: AI economy is no longer demand-
		// driven because units follow task-lists, so when resources run out
		// due to assisting they are not pulled away from their assist-tasks
		// until maxAge is exceeded and just keep waiting
		//
		// assist-tasks should abort when they become inaffordable?
		// task-list should be scanned for economy-items when stalling?
		if (mState.GetLevel() < mState.GetStorage() * 0.333f) { return false; }
		if (eState.GetLevel() < eState.GetStorage() * 0.333f) { return false; }

		if (mobileBuilderUnitIDs.size() > (staticMProducerUnitIDs.size() >> 1)) {
			return false;
		}
	}


	// notes:
	//   only static units are assisted via BuildAssistTasks,
	//   mobile builders get assistance via their BuildTask
	//
	//   should use only every third unit (eg.) for assisting,
	//   so that expansion isn't hindered by mobiles sticking
	//   to factories too much
	//
	//   should incorporate nanotower groups into this as well
	for (std::set<int>::const_iterator it = staticBuilderUnitIDs.begin(); it != staticBuilderUnitIDs.end(); it++) {
		const XAICUnit*   u    = xaih->unitHandler->GetUnitByID(*it);
		const XAIUnitDef* uDef = u->GetUnitDefPtr();

		if (!uDef->GetDef()->canBeAssisted)
			continue;
		if (xaih->unitHandler->IsUnitIdle(u->GetID()))
			continue;
		if (group->HasUnitMember(u))
			continue;
		if (assistedStaticBuilderUnitIDs.find(u->GetID()) != assistedStaticBuilderUnitIDs.end())
			continue;

		assisteeUnit    = u;
		assisteeUnitDef = u->GetUnitDefPtr();
		break;
	}

	if (assisteeUnit != 0) {
		Command c;
			c.id = CMD_GUARD;
			c.params.push_back(assisteeUnit->GetID());
		Command cAux;
			cAux.id = CMD_STOP;

		XAIITask* newTask = new XAIBuildAssistTask(numTaskIDs++, xaih);
		newTask->SetCommand(c, cAux);
		newTask->SetUnitLimit(5);
		newTask->SetAgeLimit(30 * 60 * 15);

		if (newTask->CanAddGroupMember(group)) {
			newTask->AddGroupMember(group);
			AddTask(newTask);

			assistedStaticBuilderUnitIDs.insert(assisteeUnit->GetID());
			assert(assistTasksByStaticBuilderUnitID.find(assisteeUnit->GetID()) == assistTasksByStaticBuilderUnitID.end());
			assistTasksByStaticBuilderUnitID[assisteeUnit->GetID()] = newTask;

			assert(group->GetTaskPtr() != 0);
			return true;
		} else {
			delete newTask;
			return false;
		}
	}

	return false;
}

bool XAICEconomyTaskHandler::TryAddLuaTaskForGroup(XAIGroup* group) {
	assert(group->GetUnitCount() > 0);

	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	const XAIResourceState& mState = ecoState->GetResourceState('M');
	const XAIResourceState& eState = ecoState->GetResourceState('E');

	const XAICUnit*   unit     = group->GetLeadUnitMember();
	const XAIUnitDef* unitDef  = unit->GetUnitDefPtr();

	bool ret = false;

	if (taskListIterators.find(unit->GetID()) == taskListIterators.end()) {
		const float rndListProb = (*(xaih->frng))();
		const XAITaskList* taskList = xaih->taskListsParser->GetTaskListForUnitDefRS(unitDef, rndListProb, true);

		// if the config does not define any task-lists for this
		// type of unit, the unit (group) will never be given a
		// build-task but it can still be used for assisting
		if (taskList != NULL) {
			taskListIterators[unit->GetID()] = new XAITaskListIterator();
			taskListIterators[unit->GetID()]->SetList(taskList);
		}
	}

	if (taskListIterators.find(unit->GetID()) != taskListIterators.end()) {
		XAITaskListIterator* taskListIter = taskListIterators[unit->GetID()];

		while (taskListIter->IsValid()) {
			XAIITaskListItem* taskItem = const_cast<XAIITaskListItem*>(taskListIter->GetNextTaskItem(xaih, false));
			bool taskItemOK = true;

			if (taskItem != NULL) {
				XAIBuildTaskListItem* buildTaskItem = dynamic_cast<XAIBuildTaskListItem*>(taskItem);

				if (eState.GetIncome() < float(buildTaskItem->GetMinNrgIncome())) { taskItemOK = false; }
				if (eState.GetIncome() > float(buildTaskItem->GetMaxNrgIncome())) { taskItemOK = false; }
				if (mState.GetIncome() < float(buildTaskItem->GetMinMtlIncome())) { taskItemOK = false; }
				if (mState.GetIncome() > float(buildTaskItem->GetMaxMtlIncome())) { taskItemOK = false; }
				if (eState.GetUsage() < float(buildTaskItem->GetMinNrgUsage())) { taskItemOK = false; }
				if (eState.GetUsage() > float(buildTaskItem->GetMaxNrgUsage())) { taskItemOK = false; }
				if (mState.GetUsage() < float(buildTaskItem->GetMinMtlUsage())) { taskItemOK = false; }
				if (mState.GetUsage() > float(buildTaskItem->GetMaxMtlUsage())) { taskItemOK = false; }
				if (eState.GetLevel() < float(buildTaskItem->GetMinNrgLevel())) { taskItemOK = false; }
				if (eState.GetLevel() > float(buildTaskItem->GetMaxNrgLevel())) { taskItemOK = false; }
				if (mState.GetLevel() < float(buildTaskItem->GetMinMtlLevel())) { taskItemOK = false; }
				if (mState.GetLevel() > float(buildTaskItem->GetMaxMtlLevel())) { taskItemOK = false; }
				if (xaih->GetCurrFrame() < taskItem->GetMinFrame()) { taskItemOK = false; }
				if (xaih->GetCurrFrame() > taskItem->GetMaxFrame()) { taskItemOK = false; }

				if (taskItemOK) {
					const int         defID = buildTaskItem->GetBuildeeDefID();
					const XAIUnitDef* def   = xaih->unitDefHandler->GetUnitDefByID(defID);

					const std::set<XAICUnit*>& units = xaih->unitHandler->GetUnitsByUnitDefID(defID);
					const std::map<int, int>& tasks = buildTaskCountsForUnitDefID;
					const std::map<int, int>::const_iterator btIt = tasks.find(defID);

					// engine handles the other per-type limit
					const unsigned int numUnits = units.size();
					const unsigned int numTasks = (btIt != tasks.end())? btIt->second: 0;

					if (numUnits >= buildTaskItem->GetUnitLimit()) { taskItemOK = false; }
					if (numTasks >= buildTaskItem->GetBuildLimit()) { taskItemOK = false; }

					if (taskItemOK) {
						if (buildTaskItem->GetMinFrameSpacing() != 0) {
							for (std::set<XAICUnit*>::const_iterator it = units.begin(); it != units.end(); it++) {
								if ((*it)->GetAge() < buildTaskItem->GetMinFrameSpacing()) {
									taskItemOK = false; break;
								}
							}
						}
					}

					if (taskItemOK) {
						if ((def->typeMask & MASK_BUILDER_STATIC) != 0) {
							// if the buildee is a factory, check if we have any existing
							// factories of the same type that are not at their assist-limit
							for (std::set<int>::const_iterator sit = staticBuilderUnitIDs.begin(); sit != staticBuilderUnitIDs.end(); sit++) {
								const XAICUnit*   unit    = xaih->unitHandler->GetUnitByID(*sit);
								const XAIUnitDef* unitDef = unit->GetUnitDefPtr();

								// factory, but not of the same type
								if (unitDef->GetID() != defID)
									continue;

								const std::map<int, XAIITask*>::const_iterator mit = assistTasksByStaticBuilderUnitID.find(*sit);

								if (mit == assistTasksByStaticBuilderUnitID.end()) {
									// factory, but without dedicated assisters
									taskItemOK = false; break;
								} else {
									XAIBuildAssistTask* tsk = dynamic_cast<XAIBuildAssistTask*>(mit->second);

									if (!tsk->AssistLimitReached(unit, NULL, NULL)) {
										// factory, but not enough dedicated assisters
										taskItemOK = false; break;
									}
								}
							}
						}
					}
				}

				if (taskItemOK && (ret = TryAddBuildTaskForGroup(group, buildTaskItem))) {
					break;
				} else {
					// ignore the remaining repeat-count
					if (taskListIter->IsValid())
						taskListIter->GetNextTaskItem(xaih, true);
				}
			}
		}

		if (!taskListIter->IsValid()) {
			delete taskListIterators[unit->GetID()];
			taskListIterators.erase(unit->GetID());
		}
	}

	return ret;
}



XAICEconomyTaskHandler::UnitDefLst
XAICEconomyTaskHandler::GetFeasibleUnitDefsForGroup(
	XAITaskRequestQueue* taskReqsQue,
	XAITaskRequest* taskReq,
	const XAIGroup* group,
	const XAICEconomyStateTracker* ecoState
) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::GetFeasibleUnitDefsForGroup]", xaih->timer);

	std::list<XAITaskRequest> tReqsLst;
	std::list<const XAIUnitDef*> uDefs;

	bool haveDefs = false;

	while (!haveDefs) {
		const XAITaskRequest& tReq = taskReqsQue->Top();
		const std::set<int>& uDefIDs = xaih->unitDefHandler->GetUnitDefIDsForMask(tReq.typeMask, tReq.terrMask, tReq.weapMask, false);

		for (std::set<int>::iterator uDefIDsIt = uDefIDs.begin(); uDefIDsIt != uDefIDs.end(); uDefIDsIt++) {
			const XAIUnitDef* xDef = xaih->unitDefHandler->GetUnitDefByID(*uDefIDsIt);

			if (ecoState->CanAffordNow(xDef, 0.0f, group->GetBuildSpeed(), false, false)) {
				if (group->CanGiveCommand(-xDef->GetID())) {
					uDefs.push_back(xDef);
				}
			}
		}

		if (uDefs.empty()) {
			tReqsLst.push_back(tReq);
		} else {
			haveDefs = true;
			*taskReq = tReq;
		}

		taskReqsQue->Pop();

		if (taskReqsQue->Empty()) {
			break;
		}
	}

	// restore the temporarily popped items (except the matching one)
	for (std::list<XAITaskRequest>::const_iterator it = tReqsLst.begin(); it != tReqsLst.end(); it++) {
		taskReqsQue->Push(*it);
	}

	return uDefs;
}



XAICEconomyTaskHandler::DefPosPair
XAICEconomyTaskHandler::GetBestUnitDefForGroup(
	const XAIGroup* group,
	const XAIBuildTaskListItem* item
) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::GetBestUnitDefForGroup]", xaih->timer);

	DefPosPair ret(NULL, ZeroVector);

	const XAICEconomyStateTracker* ecoState = dynamic_cast<const XAICEconomyStateTracker*>(xaih->stateTracker->GetEcoState());
	assert(ecoState != 0);



	if (item != 0) {
		const XAIUnitDef* itemBuildeeDef = xaih->unitDefHandler->GetUnitDefByID(item->GetBuildeeDefID());

		if (!item->ForceBuild()) {
			if (!ecoState->CanAffordNow(itemBuildeeDef, 0.0f, group->GetBuildSpeed(), false, false))
				return ret;
		}

		if (!group->CanGiveCommand(-itemBuildeeDef->GetID()))
			return ret;

		if (itemBuildeeDef->GetDef()->extractsMetal > 0.0f) {
			UnitDefLst defs; defs.push_back(itemBuildeeDef);

			const ResDefPair& resDefPair = GetBestMetalResource(group, defs);

			if (resDefPair.first != 0) {
				ret.first  = (resDefPair.second);
				ret.second = (resDefPair.first)->pos;
			}
		} else {
			ret.first  = itemBuildeeDef;
			ret.second = GetTaskPositionFor(itemBuildeeDef, group);
		}

		return ret;
	}



	XAITaskRequestQueue& tReqs = xaih->stateTracker->GetTaskRequests();
	XAITaskRequest tReq;

	if (tReqs.Empty())
		return ret;

	// modifies the request-queue
	const UnitDefLst& feasibleDefs = GetFeasibleUnitDefsForGroup(&tReqs, &tReq, group, ecoState);

	const XAIUnitDef* bestDef = 0;
	float bestDefScore = 0.0f;

	if (feasibleDefs.empty())
		return ret;

	if ((tReq.typeMask & MASK_M_PRODUCER_STATIC) != 0) {
		// if the resource is non-NULL we want an extractor, otherwise a maker
		const ResDefPair& resDefPair = GetBestMetalResource(group, feasibleDefs);

		if (resDefPair.first != 0) {
			ret.first  = (resDefPair.second);
			ret.second = (resDefPair.first)->pos;
			return ret;
		}
	}


	const bool
		isReqMProducer = ((tReq.typeMask & MASK_M_PRODUCER_MOBILE) || (tReq.typeMask & MASK_M_PRODUCER_STATIC)),
		isReqEProducer = ((tReq.typeMask & MASK_E_PRODUCER_MOBILE) || (tReq.typeMask & MASK_E_PRODUCER_STATIC)),
		isReqMStorage  = ((tReq.typeMask & MASK_M_STORAGE_MOBILE ) || (tReq.typeMask & MASK_M_STORAGE_STATIC )),
		isReqEStorage  = ((tReq.typeMask & MASK_E_STORAGE_MOBILE ) || (tReq.typeMask & MASK_E_STORAGE_STATIC )),
		isReqSBuilder  =  (tReq.typeMask & MASK_BUILDER_STATIC   ),
		isReqMBuilder  =  (tReq.typeMask & MASK_BUILDER_MOBILE   );

	const XAIResourceState& mState = ecoState->GetResourceState('M');
	const XAIResourceState& eState = ecoState->GetResourceState('E');
	const float avgWndStren = ecoState->GetAvgWindStrength();
	const float avgTdlStren = ecoState->GetAvgTidalStrength();

	if (isReqMStorage && !mState.WantStorage())          { return ret; }
	if (isReqEStorage && !eState.WantStorage())          { return ret; }
	if (isReqMBuilder && !ecoState->WantMobileBuilder()) { return ret; }
	if (isReqSBuilder && !ecoState->WantStaticBuilder()) { return ret; }

	// find the UnitDef that is economically feasible
	//    must disregard eg. floating metal makers on land maps
	//    during E-stalls, pick generator that makes most E at lowest E-cost
	//    during M-stalls, pick generator that makes most M at lowest M-cost
	//    etc.
	// note: maybe add a 2D matrix M[A][B] of "how many
	// counts of A can we afford for the cost of one B"?
	// note: do roulette selection over all feasible defs?
	for (UnitDefLstIt it = feasibleDefs.begin(); it != feasibleDefs.end(); it++) {
		const XAIUnitDef* xDef = (*it);
		const UnitDef*    sDef = xDef->GetDef();

		const bool
			isDefMProducer =
				(xDef->typeMask & MASK_M_PRODUCER_MOBILE) ||
				(xDef->typeMask & MASK_M_PRODUCER_STATIC),
			isDefEProducer =
				(xDef->typeMask & MASK_E_PRODUCER_MOBILE) ||
				(xDef->typeMask & MASK_E_PRODUCER_STATIC),
			isDefMStorage =
				(xDef->typeMask & MASK_M_STORAGE_MOBILE) ||
				(xDef->typeMask & MASK_M_STORAGE_STATIC),
			isDefEStorage =
				(xDef->typeMask & MASK_E_STORAGE_MOBILE) ||
				(xDef->typeMask & MASK_E_STORAGE_STATIC),
			isDefMBuilder = (xDef->typeMask & MASK_BUILDER_MOBILE),
			isDefSBuilder = (xDef->typeMask & MASK_BUILDER_STATIC),
			isDefBuilder  = (isDefMBuilder || isDefSBuilder);

		const std::map<int, int>::const_iterator defTaskCountIt = buildTaskCountsForUnitDefID.find(xDef->GetID());
		const int defTaskCount = (defTaskCountIt != buildTaskCountsForUnitDefID.end())? defTaskCountIt->second: 0;

		float defScore = 0.0f;

		// note 1: for now, don't honor water requests (at all)
		// note 2: for now, only allow one s-builder per type to exist
		//
		// note: might still want to build a shipyard in a river, etc.
		// if (xaih->mapAnalyzer->terrainMask != xDef->terrainMask) { continue; }
		//
		if ((xDef->terrainMask & MASK_LAND) == 0 && (xDef->terrainMask & MASK_AIR) == 0) {
			continue;
		}
		if (!xaih->unitHandler->GetUnitsByUnitDefID(xDef->GetID()).empty() && sDef->canBeAssisted) {
			if ((xDef->typeMask & MASK_BUILDER_STATIC) != 0) { continue; }
		}
		if (((sDef->metalCost > mState.GetLevel() * 0.5f) || (sDef->energyCost > eState.GetLevel() * 0.5f)) && (defTaskCount >= 1)) {
			continue;
		}

		// problem: when requesting eg. MASK_M_PRODUCER_*, this can
		// include factories etc. which typically have storage and
		// so drive the economic score up artificially (because the
		// storage properties compensate for the factory's own low
		// resource production)
		// therefore, make sure the request and def "match"
		if ((isReqMProducer || isReqEProducer) && (isDefBuilder)) { continue; }
		if ((isReqMStorage  || isReqEStorage ) && (isDefBuilder)) { continue; }

		if ((isReqMStorage || isReqEStorage) && (isDefMStorage || isDefEStorage) && (defTaskCount >= 1)) { continue; }
		if ((isReqSBuilder                 ) && (isDefSBuilder                 ) && (defTaskCount >= 1)) { continue; }


		if (isDefMProducer && isReqMProducer) {
			// note: engine UnitDefHandler should ensure no zero-costs,
			//       but AI interface lets some of them through anyway?
			const float defMtlIncCostRatio = xDef->GetResourceCostRatio('M', 0.0f, 0.0f);
			const float defMtlIncUseRatio = xDef->GetIncomeUsageRatio('M', mState.GetGain(), mState.GetAvgUsage(), 0.0f, 0.0f);

			if (defMtlIncCostRatio < 0.05f) {
				continue;
			}

			if (defMtlIncCostRatio > 1.0f) {
				defScore += (1.0f / (xDef->GetReturnInvestmentTimeFrames(mState.GetIncome(), eState.GetIncome()) + 1.0f));
			} else {
				defScore += defMtlIncCostRatio;
			}

			defScore *= defMtlIncUseRatio;
		}

		if (isDefEProducer && isReqEProducer) {
			const float defNrgIncCostRatio = xDef->GetResourceCostRatio('E', avgWndStren, avgTdlStren);
			const float defNrgIncUseRatio = xDef->GetIncomeUsageRatio('E', eState.GetGain(), eState.GetAvgUsage(), avgWndStren, avgTdlStren);

			if (defNrgIncCostRatio < 0.01f) {
				continue;
			}

			if (defNrgIncCostRatio > 1.0f) {
				// defScore += (1.0f / (xDef->GetReturnInvestmentTimeFrames(mState.GetGain(), eState.GetGain()) + 1.0f));
				defScore += (1.0f / (xDef->GetReturnInvestmentTimeFrames(mState.GetIncome(), eState.GetIncome()) + 1.0f));
			} else {
				defScore += defNrgIncCostRatio;
			}

			// need some way to gradually decrease rating of
			// low-income structures over time as AI's income
			// grows?
			// if ((eState.GetAvgUsage() > 0.0f) && (defENetInc / eState.GetAvgUsage()) < 0.01f) { continue; }

			defScore *= defNrgIncUseRatio;
		}


		if (isDefMStorage && isReqMStorage) {
			// need to look at our absolute storage
			// requirements too (see the code above)
			const float defMStoEff = xDef->GetStorageCostRatio('M');

			if (defMStoEff > 1.0f) {
				defScore += defMStoEff;
			} else {
				continue;
			}
		}
		if (isDefEStorage && isReqEStorage) {
			const float defEStoEff = xDef->GetStorageCostRatio('E');

			if (defEStoEff > 1.0f) {
				defScore += defEStoEff;
			} else {
				continue;
			}
		}


		if (isDefBuilder && (isReqMBuilder || isReqSBuilder)) {
			if (xDef->isSpecialBuilder) {
				continue;
			}

			// we want the builder with the most "new" build options
			//   defScore *= xaih->mapAnalyzer->TerrainFeasibilityRating(xDef);
			//   defScore *= xDef->buildOptionUDIDs.size();
			defScore += (sDef->buildSpeed / sDef->buildTime);
		}


		// these bias too much in favor of cheap structures
		//   defScore *= (1.0f / (sDef->metalCost + 1.0f));
		//   defScore *= (1.0f / (sDef->energyCost + 1.0f));
		//   defScore *= (1.0f / xDef->nBuildTime);

		if (defScore > bestDefScore) {
			bestDefScore = defScore;
			bestDef      = xDef;
		}
	}

	if (bestDef != 0) {
		ret.first  = bestDef;
		ret.second = GetTaskPositionFor(bestDef, group);
	} else {
		/*
		// re-insert with lower priority; if priority reaches 0 discard request
		// (disabled because GetFeasibleUnitDefsForGroup modifies the queue too)
		if (tReq.priority > 0) {
			XAITaskRequest tRReq(tReq);
				tRReq.priority >>= 1;
			tReqs->Push(tRReq);
		}
		*/
	}

	return ret;
}



float3 XAICEconomyTaskHandler::GetTaskPositionFor(const XAIUnitDef* def, const XAIGroup* g) {
	XAICScopedTimer t("[XAICEconomyTaskHandler::GetTaskPositionFor]", xaih->timer);

	float3 ret = float3(-1.0f, 0.0f, 0.0f);

	if (!g->IsMobile() && def->isMobile) {
		// statics building mobile ==> position parameter is ignored
		// statics building static ==> position parameter is used
		// mobiles building mobile ==> position parameter is used
		// mobiles building static ==> position parameter is used
		ret = float3(-2.0f, 0.0f, 0.0f);
	} else {


		const UnitDef* buildDef = def->GetDef();
		const int buildSize = std::max(buildDef->xsize, buildDef->zsize);

		if (buildDef->needGeo) {
			// todo: if water structure, ignore spots with y > 0.0f
			const float radSq =
				float(buildDef->xsize * buildDef->xsize) +
				float(buildDef->zsize * buildDef->zsize);

			int friendlyUnitIDs[32] = {0};
			float curResDstSq = 0.0f;
			float bstResDstSq = 1e30f;

			XAIIResource* bstResObj = 0;

			for (ResLstIt it = recResPositions.begin(); it != recResPositions.end(); it++) {
				XAIReclaimableResource* res = dynamic_cast<XAIReclaimableResource*>(*it);

				if (res->fDef == 0 || !res->fDef->geoThermal) {
					continue;
				}

				if (!xaih->rcb->CanBuildAt(buildDef, res->pos, 0)) {
					continue;
				}
				if (xaih->threatMap->GetThreat(res->pos) > 0.0f) {
					continue;
				}


				if (g->GetPathType() == -1) {
					curResDstSq = (res->pos - g->GetPos()).SqLength();
				} else {
					curResDstSq = xaih->rcb->GetPathLength(g->GetPos(), res->pos, g->GetPathType());
					curResDstSq *= curResDstSq;
				}


				const int numFriendlyUnits = xaih->rcb->GetFriendlyUnits(friendlyUnitIDs, res->pos, radSq, 32);

				for (int n = numFriendlyUnits - 1; n >= 0; n--) {
					const bool b0 = (xaih->rcb->GetUnitTeam(friendlyUnitIDs[n]) != xaih->rcb->GetMyTeam());
					const bool b1 = (xaih->rcb->GetUnitDef(friendlyUnitIDs[n])->needGeo);

					if (b0 && b1) {
						// presence of allied geothermals
						// pushes this spot further away
						curResDstSq = 1e30f; break;
					}
				}


				if (curResDstSq < bstResDstSq) {
					bstResDstSq = curResDstSq;
					bstResObj   = res;
				}
			}

			if (bstResObj != 0) {
				// returns float3(-1.0f, 0.0f, 0.0f) on any error
				ret = xaih->rcb->ClosestBuildSite(
					buildDef,
					bstResObj->pos,
					g->GetBuildDist() * g->GetBuildDist(),
					buildSize,
					XAIUtil::GetBuildFacing(xaih->rcb, bstResObj->pos)
				);
			}
		} else {
			float3 buildPos   = g->GetPos();
			bool   buildPosOK = staticBuilderUnitIDs.empty();

			while (!buildPosOK) {
				// try to place the building such that factory exits are kept free
				// TODO: invoke proper build-placement algorithm using blocking-map
				//
				for (std::set<int>::iterator it = staticBuilderUnitIDs.begin(); it != staticBuilderUnitIDs.end(); it++) {
					if (buildPosOK)
						break;

					const XAICUnit*   u       = xaih->unitHandler->GetUnitByID(*it);
					const XAIUnitDef* uDef    = u->GetUnitDefPtr();
					const float3&     uPos    = u->GetPos();
					const int         uFacing = xaih->rcb->GetBuildingFacing(*it);

					int x0 = 0, z0 = 0;
					int x1 = 0, z1 = 0;

					switch (uFacing) {
						case FACING_DOWN: {
							x0 = uPos.x - (uDef->GetDef()->xsize * SQUARE_SIZE); z0 = uPos.z;
							x1 = uPos.x + (uDef->GetDef()->xsize * SQUARE_SIZE); z1 = uPos.z + 200.0f;

							// space south of factory must remain clear
							buildPosOK =
								!XAIUtil::PosInRectangle(buildPos, x0, z0, x1, z1) &&
								xaih->rcb->CanBuildAt(buildDef, buildPos, FACING_DOWN);

							if (!buildPosOK)
								buildPos += float3((-uDef->GetDef()->xsize * SQUARE_SIZE), 0.0f, 0.0f);
						} break;

						case FACING_UP: {
							x0 = uPos.x - (uDef->GetDef()->xsize * SQUARE_SIZE); z0 = uPos.z - 200.0f;
							x1 = uPos.x + (uDef->GetDef()->xsize * SQUARE_SIZE); z1 = uPos.z;

							// space north of factory must remain clear
							buildPosOK =
								!XAIUtil::PosInRectangle(buildPos, x0, z0, x1, z1) &&
								xaih->rcb->CanBuildAt(buildDef, buildPos, FACING_DOWN);

							if (!buildPosOK)
								buildPos += float3((uDef->GetDef()->xsize * SQUARE_SIZE), 0.0f, 0.0f);
						} break;

						case FACING_RIGHT: {
							x0 = uPos.x;          z0 = uPos.z - (uDef->GetDef()->zsize * SQUARE_SIZE);
							x1 = uPos.x + 200.0f; z1 = uPos.z + (uDef->GetDef()->zsize * SQUARE_SIZE);

							// space right of factory must remain clear
							buildPosOK =
								!XAIUtil::PosInRectangle(buildPos, x0, z0, x1, z1) &&
								xaih->rcb->CanBuildAt(buildDef, buildPos, FACING_DOWN);

							if (!buildPosOK)
								buildPos += float3(0.0f, 0.0f, (-uDef->GetDef()->zsize * SQUARE_SIZE));
						} break;

						case FACING_LEFT: {
							x0 = uPos.x - 200.0f; z0 = uPos.z - (uDef->GetDef()->zsize * SQUARE_SIZE);
							x1 = uPos.x;          z1 = uPos.z + (uDef->GetDef()->zsize * SQUARE_SIZE);

							// space left of factory must remain clear
							buildPosOK =
								!XAIUtil::PosInRectangle(buildPos, x0, z0, x1, z1) &&
								xaih->rcb->CanBuildAt(buildDef, buildPos, FACING_DOWN);

							if (!buildPosOK)
								buildPos += float3(0.0f, 0.0f, (uDef->GetDef()->zsize * SQUARE_SIZE));
						} break;
						default: {
						} break;
					}
				}
			}

			ret = xaih->rcb->ClosestBuildSite(
				buildDef,
				buildPos,
				g->GetBuildDist() * g->GetBuildDist(),
				buildSize,
				XAIUtil::GetBuildFacing(xaih->rcb, buildPos)
			);
		}
	}

	return ret;
}

XAICEconomyTaskHandler::ResDefPair
XAICEconomyTaskHandler::GetBestMetalResource(
	const XAIGroup* g,
	const UnitDefLst& extDefs
) {
	int friendlyUnitIDs[32] = {0};
	float bstResDstSq = 1e30f;
	float bstResVal   = 0.0f;

	const XAIIResource* bstRes = 0;
	const XAIUnitDef* bstDef = 0;

	ResDefPair ret(bstRes, bstDef);
	UnitDefLstIt extResProDefIt;

	ResDstPairLst reachableResources;
	ResDstPairIt extResPosIt;

	GetReachableResources(g, 450.0f * 10.0f, &reachableResources);


	for (extResProDefIt = extDefs.begin(); extResProDefIt != extDefs.end(); extResProDefIt++) {
		const XAIUnitDef* def = *extResProDefIt;

		if (def->GetDef()->extractsMetal <= 0.0f) {
			continue;
		}

		// the extractor radius is often tiny, so not
		// a good way of detecting nearby enemies
		const float rad = sqrt(std::max(
			float(def->GetDef()->xsize * def->GetDef()->zsize),
			float(def->GetDef()->extractRange * def->GetDef()->extractRange)
		));

		for (extResPosIt = reachableResources.begin(); extResPosIt != reachableResources.end(); extResPosIt++) {
			const XAIExtractableResource* res = dynamic_cast<const XAIExtractableResource*>((*extResPosIt).first);

			if (!xaih->rcb->CanBuildAt(def->GetDef(), res->pos, 0)) {
				continue;
			}
			if (xaih->threatMap->GetThreat(res->pos) > 0.0f) {
				continue;
			}

			float curResDist   = (*extResPosIt).second;
			float curResDstSq  = curResDist * curResDist;
			float curResExtVal = def->ResMakeOn('M', res->rExtractionValue);


			const int numFriendlyUnits = xaih->rcb->GetFriendlyUnits(friendlyUnitIDs, res->pos, rad, 32);

			for (int n = 0; n < numFriendlyUnits; n++) {
				const bool b0 = (xaih->rcb->GetUnitTeam(friendlyUnitIDs[n]) != xaih->rcb->GetMyTeam());
				const bool b1 = (xaih->rcb->GetUnitDef(friendlyUnitIDs[n])->extractsMetal > 0.0f);

				if (b0 && b1) {
					curResDstSq = 1e30f; break;
				}
			}


			// push higher-value spots inward
			assert(curResExtVal > 0.0f);
			curResDstSq /= curResExtVal;

			if (curResDstSq < bstResDstSq) {
				bstResDstSq = curResDstSq;
				bstResVal   = curResExtVal;
				bstRes      = res;
				bstDef      = def;
			}
		}
	}

	ret.first  = bstRes;
	ret.second = bstDef;
	return ret;
}

void XAICEconomyTaskHandler::GetReachableResources(
	const XAIGroup* g,
	float maxETA,
	ResDstPairLst* reachableResources
) {
	// gather all resources reachable by group <g> in at most <maxETA> frames
	// better: get <N> closest positions, or all positions within distance <R>
	for (ResLstIt extResPosIt = extResPositions.begin(); extResPosIt != extResPositions.end(); extResPosIt++) {
		const XAIIResource* res = *extResPosIt;
		const float resDst = (res->pos - g->GetPos()).Length();

		float resPathLen  = -1.0f;
		float resGroupETA =  0.0f;

		if (g->IsMobile()) {
			if (g->GetPathType() == -1) {
				resGroupETA = resDst / g->GetMaxMoveSpeed();

				if (resGroupETA <= (maxETA / GAME_SPEED)) {
					reachableResources->push_back(ResDstPair(*extResPosIt, resDst));
				}
			} else {
				resPathLen = xaih->rcb->GetPathLength(g->GetPos(), res->pos, g->GetPathType());
				resGroupETA = resPathLen / g->GetMaxMoveSpeed(); 

				if (resPathLen >= 0.0f && resGroupETA <= (maxETA / GAME_SPEED)) {
					reachableResources->push_back(ResDstPair(*extResPosIt, resPathLen));
				}
			}
		} else {
			if (resDst < g->GetBuildDist()) {
				reachableResources->push_back(ResDstPair(*extResPosIt, resDst));
			}
		}
	}
}



void XAICEconomyTaskHandler::UnitCreated(const XAIUnitCreatedEvent* ee) {
	assert(ee != NULL);

	const XAICUnit*   unit    = xaih->unitHandler->GetUnitByID(ee->unitID);
	const XAIUnitDef* unitDef = unit->GetUnitDefPtr();

	unitSetListsByID[ee->unitID] = std::list< std::set<int>* >();

	if (unitDef->typeMask & MASK_BUILDER_MOBILE   ) { mobileBuilderUnitIDs.insert(ee->unitID);   unitSetListsByID[ee->unitID].push_back(&mobileBuilderUnitIDs); }
	if (unitDef->typeMask & MASK_BUILDER_STATIC   ) { staticBuilderUnitIDs.insert(ee->unitID);   unitSetListsByID[ee->unitID].push_back(&staticBuilderUnitIDs); }
	if (unitDef->typeMask & MASK_ASSISTER_MOBILE  ) { mobileAssisterUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&mobileAssisterUnitIDs); }
	if (unitDef->typeMask & MASK_ASSISTER_STATIC  ) { staticAssisterUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&staticAssisterUnitIDs); }
	if (unitDef->typeMask & MASK_E_PRODUCER_MOBILE) { mobileEProducerUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&mobileEProducerUnitIDs); }
	if (unitDef->typeMask & MASK_E_PRODUCER_STATIC) { staticEProducerUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&staticEProducerUnitIDs); }
	if (unitDef->typeMask & MASK_M_PRODUCER_MOBILE) { mobileMProducerUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&mobileMProducerUnitIDs); }
	if (unitDef->typeMask & MASK_M_PRODUCER_STATIC) { staticMProducerUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&staticMProducerUnitIDs); }
	if (unitDef->typeMask & MASK_E_STORAGE_MOBILE ) { mobileEStorageUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&mobileEStorageUnitIDs); }
	if (unitDef->typeMask & MASK_E_STORAGE_STATIC ) { staticEStorageUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&staticEStorageUnitIDs); }
	if (unitDef->typeMask & MASK_M_STORAGE_MOBILE ) { mobileMStorageUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&mobileMStorageUnitIDs); }
	if (unitDef->typeMask & MASK_M_STORAGE_STATIC ) { staticMStorageUnitIDs.insert(ee->unitID);  unitSetListsByID[ee->unitID].push_back(&staticMStorageUnitIDs); }

	if (ee->builderID > 0) {
		const XAICUnit* buildeeUnit = xaih->unitHandler->GetUnitByID(ee->unitID);
		const XAICUnit* builderUnit = xaih->unitHandler->GetUnitByID(ee->builderID);
		const XAIGroup* builderGroup = builderUnit->GetGroupPtr();
		const XAIUnitDef* builderDef = builderUnit->GetUnitDefPtr();
		const XAIUnitDef* buildeeDef = buildeeUnit->GetUnitDefPtr();

		assert(buildeeDef != 0);
		assert(builderDef != 0);
		assert(builderUnit->GetGroupPtr() != 0);
		// task may already have been cleaned up?
		// assert(builderGroup->GetTaskPtr() != 0);

		// note: why not just notify all tasks?
		if (builderGroup->GetTaskPtr() != 0) {
			(builderGroup->GetTaskPtr())->UnitCreated(ee->unitID, buildeeDef->GetID(), ee->builderID);
		}
	}
}

void XAICEconomyTaskHandler::UnitFinished(const XAIUnitFinishedEvent* ee) {
	if (unitSetListsByID.find(ee->unitID) == unitSetListsByID.end()) {
		// UnitFinished not preceded by UnitCreated
		XAIUnitCreatedEvent e;
			e.type      = XAI_EVENT_UNIT_CREATED;
			e.unitID    = ee->unitID;
			e.builderID = -1;
			e.frame     = ee->frame;
		UnitCreated(&e);
	} else {
		// give freshly-built units a move order
		// so they don't perma-block factory exits
		// f = xaih->rcb->GetBuildingFacing(ee->builderID);
		XAICUnit* unit = xaih->unitHandler->GetUnitByID(ee->unitID);
		unit->Move(unit->GetPos() + float3(0.0f, 0.0f, 150.0f));
	}
}

void XAICEconomyTaskHandler::UnitGiven(const XAIUnitGivenEvent* ee) {
	if (ee->newUnitTeam != xaih->rcb->GetMyTeam()) {
		return;
	}

	// convert the event
	XAIUnitCreatedEvent e;
		e.type      = XAI_EVENT_UNIT_CREATED;
		e.unitID    = ee->unitID;
		e.builderID = -1;
		e.frame     = ee->frame;
	UnitCreated(&e);
}


void XAICEconomyTaskHandler::UnitDestroyed(const XAIUnitDestroyedEvent* ee) {
	assert(ee != NULL);

	// list of all sets that this unit was part of
	std::list< std::set<int>* > unitSetList = unitSetListsByID[ee->unitID];
	std::list< std::set<int>* >::iterator lsit;

	for (lsit = unitSetList.begin(); lsit != unitSetList.end(); lsit++) {
		std::set<int>* unitSet = *lsit; unitSet->erase(ee->unitID);
	}

	unitSetListsByID.erase(ee->unitID);

	for (std::map<int, XAIITask*>::iterator tasksIt = tasksByID.begin(); tasksIt != tasksByID.end(); tasksIt++) {
		tasksIt->second->UnitDestroyed(ee->unitID);
	}

	// note: this way, when the "leader" (first added unit)
	// of a group is destroyed, the rest of the group will
	// be given a new task-list (after completing whatever
	// its current task is) rather than continuing with the
	// old one, but we need to since we do not get informed
	// of group destruction events
	if (taskListIterators.find(ee->unitID) != taskListIterators.end()) {
		delete taskListIterators[ee->unitID];
		taskListIterators.erase(ee->unitID);
	}
}

void XAICEconomyTaskHandler::UnitCaptured(const XAIUnitCapturedEvent* ee) {
	if (ee->oldUnitTeam != xaih->rcb->GetMyTeam()) {
		return;
	}

	// convert the event
	// (no way to access attackerID)
	XAIUnitDestroyedEvent e;
		e.type       = XAI_EVENT_UNIT_DESTROYED;
		e.unitID     = ee->unitID;
		e.attackerID = -1;
		e.frame      = ee->frame;
	UnitDestroyed(&e);
}

void XAICEconomyTaskHandler::UnitDamaged(const XAIUnitDamagedEvent* ee) {
	if (ee->attackerID == -1)
		return;

	const XAICUnit* u = xaih->unitHandler->GetUnitByID(ee->unitID);
	const XAIGroup* g = u->GetGroupPtr();

	if (xaih->dgunConHandler->GetController(ee->unitID) != 0)
		return;
	if (g == 0)
		return;

	if (g->GetTaskPtr() != 0) {
		(g->GetTaskPtr())->UnitDamaged(ee->unitID, ee->attackerID);
	}
}
