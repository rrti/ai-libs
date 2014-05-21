#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/IAICheats.h"

#include "./XAIITaskHandler.hpp"
#include "./XAIITask.hpp"
#include "./XAITaskListsParser.hpp"
#include "../events/XAIIEvent.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitHandler.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../units/XAIUnit.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../groups/XAIGroupHandler.hpp"
#include "../groups/XAIGroup.hpp"
#include "../utils/XAITimer.hpp"
#include "../utils/XAIRNG.hpp"
#include "../map/XAIThreatMap.hpp"

void XAICMilitaryTaskHandler::OnEvent(const XAIIEvent* e) {
	// make sure the base instance part
	// also gets to process this event
	this->XAIITaskHandler::OnEvent(e);

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
		} break;
		case XAI_EVENT_ENEMY_DAMAGED: {
		} break;

		case XAI_EVENT_UNIT_GIVEN: {
			UnitGiven(dynamic_cast<const XAIUnitGivenEvent*>(e));
		} break;
		case XAI_EVENT_UNIT_CAPTURED: {
			UnitCaptured(dynamic_cast<const XAIUnitCapturedEvent*>(e));
		} break;

		case XAI_EVENT_ENEMY_ENTER_LOS: {
			const XAIEnemyEnterLOSEvent* ee = dynamic_cast<const XAIEnemyEnterLOSEvent*>(e);
			const int enemyUnitID = ee->unitID;

			enemyUnitIDsInLOS.insert(enemyUnitID);
		} break;
		case XAI_EVENT_ENEMY_LEAVE_LOS: {
			const XAIEnemyLeaveLOSEvent* ee = dynamic_cast<const XAIEnemyLeaveLOSEvent*>(e);
			const int enemyUnitID = ee->unitID;

			enemyUnitIDsInLOS.erase(enemyUnitID);
		} break;
		case XAI_EVENT_ENEMY_ENTER_RADAR: {
			const XAIEnemyEnterRadarEvent* ee = dynamic_cast<const XAIEnemyEnterRadarEvent*>(e);
			const int enemyUnitID = ee->unitID;

			enemyUnitIDsInRDR.insert(enemyUnitID);
		} break;
		case XAI_EVENT_ENEMY_LEAVE_RADAR: {
			const XAIEnemyLeaveRadarEvent* ee = dynamic_cast<const XAIEnemyLeaveRadarEvent*>(e);
			const int enemyUnitID = ee->unitID;

			enemyUnitIDsInRDR.erase(enemyUnitID);
		} break;

		case XAI_EVENT_UPDATE: {
			// update the derived instance part
			this->XAICMilitaryTaskHandler::Update();
		} break;
		default: {
		} break;
	}
}

void XAICMilitaryTaskHandler::Update() {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::Update]", xaih->timer);

	UpdateTasks();

	std::list<XAIGroup*> idleGroups = xaih->groupHandler->GetNonTaskedGroups();

	AssignIdleUnitsToIdleGroups(&idleGroups);
	MergeIdleGroups(&idleGroups);
	AssignIdleGroupsToTasks(&idleGroups);
}

void XAICMilitaryTaskHandler::UpdateTasks() {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::UpdateTasks]", xaih->timer);

	std::map<int, XAIITask*>::iterator tasksIt;
	std::list<int> doneTasks;
	std::list<int>::iterator doneTasksIt;

	for (tasksIt = tasksByID.begin(); tasksIt != tasksByID.end(); tasksIt++) {
		XAIITask* task = tasksIt->second;

		if (task->XAIITask::Update() || task->Update()) {
			doneTasks.push_back(tasksIt->first);

			switch (task->type) {
				case XAI_TASK_ATTACK_UNIT: {
					const Command& taskCmd   = task->GetCommand();
					const int      taskObjID = int(taskCmd.params[0]); // GetObjectID() may already be -1

					assert(taskObjID >= 0);

					enemyUnitIDsInLOS.erase(taskObjID);
					enemyUnitIDsInRDR.erase(taskObjID);

					if (attackTaskCountsForUnitID[taskObjID] > 0) {
						attackTaskCountsForUnitID[taskObjID] -= 1;
					} else {
						attackTaskCountsForUnitID.erase(taskObjID);
					}
				} break;

				case XAI_TASK_DEFEND_UNIT: {
					const Command& taskCmd   = task->GetCommand();
					const int      taskObjID = int(taskCmd.params[0]); // GetObjectID() may already be -1

					assert(taskObjID >= 0);

					if (defendTaskCountsForUnitID[taskObjID] > 0) {
						defendTaskCountsForUnitID[taskObjID] -= 1;
					} else {
						defendTaskCountsForUnitID.erase(taskObjID);
					}
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



int XAICMilitaryTaskHandler::GetNonGroupedFinishedOffenders(
	std::list<XAICUnit*>* ngMobileOffenders,
	std::list<XAICUnit*>* ngStaticOffenders
) {
	int ngMobileOffenderCnt = 0;
	int ngStaticOffenderCnt = 0;

	for (std::set<int>::iterator it = mobileOffenseUnitIDs.begin(); it != mobileOffenseUnitIDs.end(); it++) {
		XAICUnit* u  = xaih->unitHandler->GetUnitByID(*it);
		bool      uf = xaih->unitHandler->IsUnitFinished(u->GetID());

		// if a unit is a builder, then the ETH will have
		// already grouped it (regardless of whether that
		// unit is also an attacker), so *A Commanders are
		// never included here
		if (uf && u->GetGroupPtr() == 0) {
			if (xaih->unitHandler->IsUnitIdle(u->GetID())) {
				ngMobileOffenders->push_back(u);
				ngMobileOffenderCnt++;
			}
		}
	}

	for (std::set<int>::iterator it = staticOffenseUnitIDs.begin(); it != staticOffenseUnitIDs.end(); it++) {
		XAICUnit* u  = xaih->unitHandler->GetUnitByID(*it);
		bool      uf = xaih->unitHandler->IsUnitFinished(u->GetID());

		if (uf && u->GetGroupPtr() == 0) {
			if (xaih->unitHandler->IsUnitIdle(u->GetID())) {
				ngStaticOffenders->push_back(u);
				ngStaticOffenderCnt++;
			}
		}
	}

	return (ngMobileOffenderCnt + ngStaticOffenderCnt);
}

void XAICMilitaryTaskHandler::AssignIdleUnitsToIdleGroups(std::list<XAIGroup*>* idleGroups) {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::AssignIdleUnitsToIdleGroups]", xaih->timer);

	std::list<XAICUnit*> ngMobileOffenders;
	std::list<XAICUnit*> ngStaticOffenders;
	std::list<XAIGroup*>::iterator idleGroupsIt;

	if (GetNonGroupedFinishedOffenders(&ngMobileOffenders, &ngStaticOffenders) > 0) {
		for (std::list<XAICUnit*>::iterator it = ngMobileOffenders.begin(); it != ngMobileOffenders.end(); it++) {
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

		for (std::list<XAICUnit*>::iterator it = ngStaticOffenders.begin(); it != ngStaticOffenders.end(); it++) {
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

void XAICMilitaryTaskHandler::MergeIdleGroups(std::list<XAIGroup*>* idleGroups) {
	std::list<XAIGroup*>::iterator idleGroupsIt1;
	std::list<XAIGroup*>::iterator idleGroupsIt2;

	if (idleGroups->size() <= 1) {
		return;
	}

	for (idleGroupsIt1 = idleGroups->begin(); idleGroupsIt1 != idleGroups->end(); idleGroupsIt1++) {
		if ((*idleGroupsIt1)->GetUnitCount() == 0)
			continue;

		for (idleGroupsIt2 = idleGroupsIt1, idleGroupsIt2++; idleGroupsIt2 != idleGroups->end(); idleGroupsIt2++) {
			if ((*idleGroupsIt2)->GetUnitCount() == 0)
				continue;

			if ((*idleGroupsIt1)->CanMergeIdleGroup(*idleGroupsIt2)) {
				(*idleGroupsIt1)->MergeIdleGroup(*idleGroupsIt2);
			}

		}
	}

	// idleGroups can now contain empty groups, so remove them
	idleGroupsIt1 = idleGroups->begin();

	while (idleGroupsIt1 != idleGroups->end()) {
		if ((*idleGroupsIt1)->GetUnitCount() == 0) {
			idleGroupsIt1 = idleGroups->erase(idleGroupsIt1);
		} else {
			idleGroupsIt1++;
		}
	}
}

void XAICMilitaryTaskHandler::AssignIdleGroupsToTasks(std::list<XAIGroup*>* idleGroups) {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::AssignIdleGroupsToTasks]", xaih->timer);

	const int baseFrame = (xaih->GetCurrFrame() % GAME_SPEED);
	const int numGroups = (idleGroups->size() / GAME_SPEED) + 1;
	const int baseGroup = baseFrame * numGroups;
	const int lastGroup = baseGroup + numGroups;

	int groupCntr = 0;


	std::map<int, XAIITask*>::iterator tasksIt;
	std::list<XAIGroup*>::iterator idleGroupsIt;

	for (idleGroupsIt = idleGroups->begin(); idleGroupsIt != idleGroups->end(); idleGroupsIt++) {
		XAIGroup* group = *idleGroupsIt;

		// groups that can build should be handled by the ETH
		// groups that cannot attack should not be in this list
		if (group->IsBuilder()) { continue; }
		if (!group->IsAttacker()) { continue; }

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

			if (group->GetTaskPtr() == 0) {
				if (!xaih->GetConfig()) {
					if (!TryAddAttackTaskForGroup(group, NULL)) {
						if (!TryAddDefendTaskForGroup(group)) {
						}
					}
				} else {
					if (!TryAddLuaTaskForGroup(group)) {
						if (!TryAddAttackTaskForGroup(group, NULL)) {
							if (!TryAddDefendTaskForGroup(group)) {
							}
						}
					}
				}
			}
		}
	}
}



bool XAICMilitaryTaskHandler::TryAddAttackTaskForGroup(XAIGroup* group, const XAIAttackTaskListItem* item) {
	const int attackeeUnitID = GetBestAttackeeIDForGroup(group, item);

	if (attackeeUnitID == -1 || xaih->ccb->GetUnitDef(attackeeUnitID) == NULL) {
		return false;
	}

	const float3& attackeeUnitPos = xaih->ccb->GetUnitPos(attackeeUnitID);
	const std::map<int, int>::iterator it = attackTaskCountsForUnitID.find(attackeeUnitID);
	const int attackTaskCount = (it != attackTaskCountsForUnitID.end())? it->second: 0;

	if (item != NULL && attackTaskCount > item->GetAttackLimit()) {
		return false;
	}

	Command cAux;
		cAux.id = CMD_MOVE;
		cAux.params.push_back(attackeeUnitPos.x);
		cAux.params.push_back(attackeeUnitPos.y);
		cAux.params.push_back(attackeeUnitPos.z);
	Command c;
		c.id = CMD_ATTACK;
		c.params.push_back(attackeeUnitID);

	XAIITask* newTask = new XAIAttackTask(numTaskIDs++, xaih);
	newTask->SetCommand(c, cAux);

	// make sure we don't add a group that is unarmed
	// (this is needed because <group> can consist of
	// builders too in the current architecture)
	if (newTask->CanAddGroupMember(group)) {
		AddTask(newTask);
		newTask->AddGroupMember(group);
		newTask->SetUnitLimit(item? item->GetGroupMaxSize(): (1U << 31U));

		assert(group->GetTaskPtr() != 0);

		attackTaskCountsForUnitID[attackeeUnitID] = attackTaskCount + 1;
		return true;
	}

	delete newTask;
	return false;
}

bool XAICMilitaryTaskHandler::TryAddDefendTaskForGroup(XAIGroup* group) {
	// disabled for now
	return false;

	const int defendeeUnitID = GetBestDefendeeIDForGroup(group, NULL);

	if (defendeeUnitID == -1 || xaih->rcb->GetUnitDef(defendeeUnitID) == NULL) {
		return false;
	}

	const float3& defendeeUnitPos = xaih->ccb->GetUnitPos(defendeeUnitID);
	const std::map<int, int>::iterator it = defendTaskCountsForUnitID.find(defendeeUnitID);
	const int defendTaskCount = (it != defendTaskCountsForUnitID.end())? it->second: 0;

	Command c;
		c.id = CMD_GUARD;
		c.params.push_back(defendeeUnitID);
	Command cAux;
		cAux.id = CMD_MOVE;
		cAux.params.push_back(defendeeUnitPos.x);
		cAux.params.push_back(defendeeUnitPos.y);
		cAux.params.push_back(defendeeUnitPos.z);

	XAIITask* newTask = new XAIDefendTask(numTaskIDs++, xaih);
	newTask->SetCommand(c, cAux);
	newTask->SetAgeLimit(30 * 60 * 5);

	if (newTask->CanAddGroupMember(group)) {
		AddTask(newTask);
		newTask->AddGroupMember(group);

		assert(group->GetTaskPtr() != 0);

		defendTaskCountsForUnitID[defendeeUnitID] = defendTaskCount + 1;
		return true;
	}

	delete newTask;
	return false;
}

bool XAICMilitaryTaskHandler::TryAddLuaTaskForGroup(XAIGroup* group) {
	assert(group->GetUnitCount() > 0);

	const XAICUnit*   unit     = group->GetLeadUnitMember();
	const XAIUnitDef* unitDef  = unit->GetUnitDefPtr();

	bool ret = false;

	if (taskListIterators.find(unit->GetID()) == taskListIterators.end()) {
		const float rndListProb = (*(xaih->frng))();
		const XAITaskList* taskList = xaih->taskListsParser->GetTaskListForUnitDefRS(unitDef, rndListProb, false);

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
				XAIAttackTaskListItem* attackTaskItem = dynamic_cast<XAIAttackTaskListItem*>(taskItem);

				if (group->GetUnitCount() < attackTaskItem->GetGroupMinSize()) { taskItemOK = false; }
				if (group->GetUnitCount() > attackTaskItem->GetGroupMaxSize()) { taskItemOK = false; }
				if (group->GetMtlCost() < attackTaskItem->GetGroupMinMtlCost()) { taskItemOK = false; }
				if (group->GetMtlCost() > attackTaskItem->GetGroupMaxMtlCost()) { taskItemOK = false; }
				if (group->GetNrgCost() < attackTaskItem->GetGroupMinNrgCost()) { taskItemOK = false; }
				if (group->GetNrgCost() > attackTaskItem->GetGroupMaxNrgCost()) { taskItemOK = false; }
				if (xaih->GetCurrFrame() < taskItem->GetMinFrame()) { taskItemOK = false; }
				if (xaih->GetCurrFrame() > taskItem->GetMaxFrame()) { taskItemOK = false; }

				if (taskItemOK && (ret = TryAddAttackTaskForGroup(group, attackTaskItem))) {
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






#define SEARCH_FOR_KNOWN_ENEMIES_BY_DEF(unitIDs)                                                  \
	sqDistMin = 1e30f;                                                                            \
                                                                                                  \
	for (std::set<int>::const_iterator it = unitIDs.begin(); it != unitIDs.end(); it++) {         \
		const    UnitDef* sDef = xaih->ccb->GetUnitDef(*it);                                      \
		const XAIUnitDef* xDef = NULL;                                                            \
                                                                                                  \
		if (sDef == NULL)                                                                         \
			continue;                                                                             \
                                                                                                  \
		xDef = xaih->unitDefHandler->GetUnitDefByID(sDef->id);                                    \
		numEnemyMobileBuilders += int((xDef->typeMask & MASK_BUILDER_MOBILE) > 0);                \
		numEnemyStaticBuilders += int((xDef->typeMask & MASK_BUILDER_STATIC) > 0);                \
                                                                                                  \
		if (sDef->id == item->GetAttackeeDefID()) {                                               \
			const std::map<int, int>::iterator mit = attackTaskCountsForUnitID.find(*it);         \
			const int attackTaskCount = (mit != attackTaskCountsForUnitID.end())? mit->second: 1; \
                                                                                                  \
			sqDistCur = (xaih->ccb->GetUnitPos(*it) - group->GetPos()).SqLength();                \
			sqDistCur *= attackTaskCount;                                                         \
                                                                                                  \
			if (sqDistCur < sqDistMin) {                                                          \
				sqDistMin = sqDistCur; attackeeID = *it;                                          \
			}                                                                                     \
		}                                                                                         \
	}

#define SEARCH_FOR_KNOWN_ENEMIES_BY_MASK(unitIDs)                                                              \
	sqDistMin = 1e30f;                                                                                         \
                                                                                                               \
	for (std::set<int>::const_iterator it = enemyUnitIDsInLOS.begin(); it != enemyUnitIDsInLOS.end(); it++) {  \
		const UnitDef* uDef = xaih->ccb->GetUnitDef(*it);                                                      \
		const XAIUnitDef* xuDef = NULL;                                                                        \
                                                                                                               \
		if (uDef == NULL)                                                                                      \
			continue;                                                                                          \
                                                                                                               \
		const std::map<int, int>::iterator mit = attackTaskCountsForUnitID.find(*it);                          \
		const int attackTaskCount = (mit != attackTaskCountsForUnitID.end())? mit->second: 1;                  \
                                                                                                               \
		xuDef = xaih->unitDefHandler->GetUnitDefByID(uDef->id);                                                \
		sqDistCur = (xaih->ccb->GetUnitPos(*it) - group->GetPos()).SqLength();                                 \
		sqDistCur *= attackTaskCount;                                                                          \
                                                                                                               \
		if (xuDef->typeMask & MASK_OFFENSE_MOBILE   ) { sqDistCur /= 256.0f; }                                 \
		if (xuDef->typeMask & MASK_M_PRODUCER_STATIC) { sqDistCur /= 128.0f; }                                 \
		if (xuDef->typeMask & MASK_E_PRODUCER_STATIC) { sqDistCur /= 128.0f; }                                 \
		if (xuDef->typeMask & MASK_M_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }                                 \
		if (xuDef->typeMask & MASK_E_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }                                 \
		if (xuDef->typeMask & MASK_BUILDER_MOBILE   ) { sqDistCur /=  32.0f; }                                 \
		if (xuDef->typeMask & MASK_BUILDER_STATIC   ) { sqDistCur /=  32.0f; }                                 \
		if (xuDef->typeMask & MASK_M_STORAGE_STATIC ) { sqDistCur /=  16.0f; }                                 \
		if (xuDef->typeMask & MASK_E_STORAGE_STATIC ) { sqDistCur /=  16.0f; }                                 \
		if (xuDef->typeMask & MASK_M_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }                                 \
		if (xuDef->typeMask & MASK_E_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }                                 \
		if (xuDef->typeMask & MASK_INTEL_MOBILE     ) { sqDistCur /=   4.0f; }                                 \
		if (xuDef->typeMask & MASK_INTEL_STATIC     ) { sqDistCur /=   4.0f; }                                 \
		if (xuDef->typeMask & MASK_ASSISTER_MOBILE  ) { sqDistCur /=   2.0f; }                                 \
		if (xuDef->typeMask & MASK_ASSISTER_STATIC  ) { sqDistCur /=   2.0f; }                                 \
                                                                                                               \
		if (sqDistCur < sqDistMin) {                                                                           \
			sqDistMin = sqDistCur; attackeeID = *it;                                                           \
		}                                                                                                      \
	}

#define SEARCH_FOR_UNKNOWN_ENEMIES_BY_DEF()                                                       \
	sqDistMin = 1e30f;                                                                            \
                                                                                                  \
	for (int i = xaih->threatMap->GetNumEnemies(); i >= 0; i--) {                                 \
		const int    enemyID  = xaih->threatMap->GetEnemyID(i);                                   \
		const float3 enemyPos = xaih->ccb->GetUnitPos(enemyID);                                   \
                                                                                                  \
		const    UnitDef* sDef = xaih->ccb->GetUnitDef(enemyID);                                  \
		const XAIUnitDef* xDef = NULL;                                                            \
                                                                                                  \
		if (sDef == NULL)                                                                         \
			continue;                                                                             \
                                                                                                  \
		xDef = xaih->unitDefHandler->GetUnitDefByID(sDef->id);                                    \
		numEnemyMobileBuilders += int((xDef->typeMask & MASK_BUILDER_MOBILE) > 0);                \
		numEnemyStaticBuilders += int((xDef->typeMask & MASK_BUILDER_STATIC) > 0);                \
                                                                                                  \
		if (group->GetPower() < xaih->threatMap->GetThreat(enemyPos)) {                           \
			continue;                                                                             \
		}                                                                                         \
                                                                                                  \
		if (sDef->id == item->GetAttackeeDefID()) {                                               \
			const std::map<int, int>::iterator mit = attackTaskCountsForUnitID.find(enemyID);     \
			const int attackTaskCount = (mit != attackTaskCountsForUnitID.end())? mit->second: 1; \
                                                                                                  \
			sqDistCur = (xaih->ccb->GetUnitPos(enemyID) - group->GetPos()).SqLength();            \
			sqDistCur *= attackTaskCount;                                                         \
                                                                                                  \
			if (sqDistCur < sqDistMin) {                                                          \
				sqDistMin = sqDistCur; attackeeID = enemyID;                                      \
			}                                                                                     \
		}                                                                                         \
	}

// NOTE: this skips over eg. armmex for corgator because the
// latter has an AttackListItem defined for the former which
// says minGroupSize=8 ... instead/also use {min, max}Threat?
#define SEARCH_FOR_UNKNOWN_ENEMIES_BY_MASK()                                                  \
	sqDistMin = 1e30f;                                                                        \
                                                                                              \
	for (int i = xaih->threatMap->GetNumEnemies(); i >= 0; i--) {                             \
		const int    enemyID  = xaih->threatMap->GetEnemyID(i);                               \
		const float3 enemyPos = xaih->ccb->GetUnitPos(enemyID);                               \
                                                                                              \
		const    UnitDef* suDef = xaih->ccb->GetUnitDef(enemyID);                             \
		const XAIUnitDef* xuDef = NULL;                                                       \
                                                                                              \
		if (suDef == NULL)                                                                    \
			continue;                                                                         \
                                                                                              \
		/* if we have a task-item restraint on this attackee, skip it */                      \
		if (xaih->taskListsParser->HasAttackeeAttackerItem(suDef->id, gUnitDef->GetID()))     \
			continue;                                                                         \
                                                                                              \
		const std::map<int, int>::iterator mit = attackTaskCountsForUnitID.find(enemyID);     \
		const int attackTaskCount = (mit != attackTaskCountsForUnitID.end())? mit->second: 1; \
                                                                                              \
		xuDef = xaih->unitDefHandler->GetUnitDefByID(suDef->id);                              \
		sqDistCur = (xaih->ccb->GetUnitPos(enemyID) - group->GetPos()).SqLength();            \
		sqDistCur *= attackTaskCount;                                                         \
                                                                                              \
		if (xuDef->typeMask & MASK_OFFENSE_MOBILE   ) { sqDistCur /= 256.0f; }                \
		if (xuDef->typeMask & MASK_M_PRODUCER_STATIC) { sqDistCur /= 128.0f; }                \
		if (xuDef->typeMask & MASK_E_PRODUCER_STATIC) { sqDistCur /= 128.0f; }                \
		if (xuDef->typeMask & MASK_M_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }                \
		if (xuDef->typeMask & MASK_E_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }                \
		if (xuDef->typeMask & MASK_BUILDER_MOBILE   ) { sqDistCur /=  32.0f; }                \
		if (xuDef->typeMask & MASK_BUILDER_STATIC   ) { sqDistCur /=  32.0f; }                \
		if (xuDef->typeMask & MASK_M_STORAGE_STATIC ) { sqDistCur /=  16.0f; }                \
		if (xuDef->typeMask & MASK_E_STORAGE_STATIC ) { sqDistCur /=  16.0f; }                \
		if (xuDef->typeMask & MASK_M_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }                \
		if (xuDef->typeMask & MASK_E_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }                \
		if (xuDef->typeMask & MASK_INTEL_MOBILE     ) { sqDistCur /=   4.0f; }                \
		if (xuDef->typeMask & MASK_INTEL_STATIC     ) { sqDistCur /=   4.0f; }                \
		if (xuDef->typeMask & MASK_ASSISTER_MOBILE  ) { sqDistCur /=   2.0f; }                \
		if (xuDef->typeMask & MASK_ASSISTER_STATIC  ) { sqDistCur /=   2.0f; }                \
                                                                                              \
		if (group->GetPower() < xaih->threatMap->GetThreat(enemyPos)) {                       \
			continue;                                                                         \
		}                                                                                     \
                                                                                              \
		if (sqDistCur < sqDistMin) {                                                          \
			sqDistMin = sqDistCur; attackeeID = enemyID;                                      \
		}                                                                                     \
	}



int XAICMilitaryTaskHandler::GetBestAttackeeIDForGroup(XAIGroup* group, const XAIAttackTaskListItem* item) {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::GetBestAttackeeIDForGroup]", xaih->timer);

	const XAICUnit*   gUnit    = group->GetLeadUnitMember();
	const XAIUnitDef* gUnitDef = gUnit->GetUnitDefPtr();

	int attackeeID = -1;

	int numEnemyMobileBuilders = 0;
	int numEnemyStaticBuilders = 0;

	float sqDistCur =  0.0f;
	float sqDistMin = 1e30f;

	if (item != NULL) {
		// let the list-item define what we are looking for
		SEARCH_FOR_KNOWN_ENEMIES_BY_DEF(enemyUnitIDsInLOS);

		if (attackeeID == -1) {
			SEARCH_FOR_KNOWN_ENEMIES_BY_DEF(enemyUnitIDsInRDR);
		}

		if (attackeeID == -1) {
			// nothing in LOS or radar, get out the ASE
			SEARCH_FOR_UNKNOWN_ENEMIES_BY_DEF();
		}

		if (attackeeID == -1) {
			// at this point, attackers would have to remain idle
			// for consistency with the economy task-handler, but
			// that would put a large onus on the config-writer
			//
			// solved another way: if an idle group cannot satisfy
			// *any* task-list item, it runs this search as backup
			// via TryAddAttackTaskForGroup(group, NULL)
			//
			// SEARCH_FOR_UNKNOWN_ENEMIES_BY_MASK();
		}

	} else {
		// /*
		// hack-sih: a group that cannot satisfy any task-item needs
		// to be of a certain size before it is allowed to attack at
		// will (to prevent the trickle-effect)
		// FIXME: if disabled, units attack places of too high threat
		if (group->GetUnitCount() < 4) {
			// return -1;
		}

		SEARCH_FOR_KNOWN_ENEMIES_BY_MASK(enemyUnitIDsInLOS);

		if (attackeeID == -1) {
			SEARCH_FOR_KNOWN_ENEMIES_BY_MASK(enemyUnitIDsInRDR);
		}

		if (attackeeID == -1) {
			SEARCH_FOR_UNKNOWN_ENEMIES_BY_MASK();
		}
	}

	return attackeeID;
}

int XAICMilitaryTaskHandler::GetBestDefendeeIDForGroup(XAIGroup* group, const XAIAttackTaskListItem* item) {
	XAICScopedTimer t("[XAICMilitaryTaskHandler::GetBestDefendeeIDForGroup]", xaih->timer);

	int defendeeID = -1;

	const unsigned int mask =
		MASK_M_PRODUCER_STATIC | MASK_M_PRODUCER_MOBILE |
		MASK_E_PRODUCER_STATIC | MASK_E_PRODUCER_MOBILE |
		MASK_BUILDER_MOBILE;

	const std::set<int>& unitIDs = xaih->unitHandler->GetUnitIDsByMaskApprox(mask, 0, 0);
	const XAICUnit*   unit    = NULL;
	const XAIUnitDef* unitDef = NULL;

	float sqDistCur =  0.0f;
	float sqDistMin = 1e30f;

	for (std::set<int>::const_iterator sit = unitIDs.begin(); sit != unitIDs.end(); sit++) {
		unit    = xaih->unitHandler->GetUnitByID(*sit);
		unitDef = unit->GetUnitDefPtr();

		if (unitDef->isAttacker)
			continue;

		const std::map<int, int>::iterator it = defendTaskCountsForUnitID.find(*sit);
		const int defendTaskCount = (it != defendTaskCountsForUnitID.end())? it->second: 1;

		sqDistCur = (unit->GetPos() - group->GetPos()).SqLength();
		sqDistCur *= defendTaskCount;

		if (unitDef->typeMask & MASK_M_PRODUCER_STATIC) { sqDistCur /= 128.0f; }
		if (unitDef->typeMask & MASK_E_PRODUCER_STATIC) { sqDistCur /= 128.0f; }
		if (unitDef->typeMask & MASK_M_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }
		if (unitDef->typeMask & MASK_E_PRODUCER_MOBILE) { sqDistCur /=  64.0f; }
		if (unitDef->typeMask & MASK_BUILDER_MOBILE   ) { sqDistCur /=  32.0f; }
		if (unitDef->typeMask & MASK_BUILDER_STATIC   ) { sqDistCur /=  32.0f; }
		if (unitDef->typeMask & MASK_M_STORAGE_STATIC ) { sqDistCur /=  16.0f; }
		if (unitDef->typeMask & MASK_E_STORAGE_STATIC ) { sqDistCur /=  16.0f; }
		if (unitDef->typeMask & MASK_M_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }
		if (unitDef->typeMask & MASK_E_STORAGE_MOBILE ) { sqDistCur /=   8.0f; }
		if (unitDef->typeMask & MASK_INTEL_MOBILE     ) { sqDistCur /=   4.0f; }
		if (unitDef->typeMask & MASK_INTEL_STATIC     ) { sqDistCur /=   4.0f; }
		if (unitDef->typeMask & MASK_ASSISTER_MOBILE  ) { sqDistCur /=   2.0f; }
		if (unitDef->typeMask & MASK_ASSISTER_STATIC  ) { sqDistCur /=   2.0f; }

		if (sqDistCur < sqDistMin) {
			sqDistMin = sqDistCur; defendeeID = *sit;
		}
	}

	return defendeeID;
}

void XAICMilitaryTaskHandler::UnitCreated(const XAIUnitCreatedEvent* ee) {
	assert(ee != NULL);

	const XAICUnit*   unit    = xaih->unitHandler->GetUnitByID(ee->unitID);
	const XAIUnitDef* unitDef = unit->GetUnitDefPtr();

	unitSetListsByID[ee->unitID] = std::list< std::set<int>* >();

	if (unitDef->typeMask & MASK_DEFENSE_MOBILE) { mobileDefenseUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&mobileDefenseUnitIDs); }
	if (unitDef->typeMask & MASK_DEFENSE_STATIC) { staticDefenseUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&staticDefenseUnitIDs); }
	if (unitDef->typeMask & MASK_OFFENSE_MOBILE) { mobileOffenseUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&mobileOffenseUnitIDs); }
	if (unitDef->typeMask & MASK_OFFENSE_STATIC) { staticOffenseUnitIDs.insert(ee->unitID); unitSetListsByID[ee->unitID].push_back(&staticOffenseUnitIDs); }
	if (unitDef->typeMask & MASK_INTEL_MOBILE  ) { mobileIntelUnitIDs.insert(ee->unitID);   unitSetListsByID[ee->unitID].push_back(&mobileIntelUnitIDs); }
	if (unitDef->typeMask & MASK_INTEL_STATIC  ) { staticIntelUnitIDs.insert(ee->unitID);   unitSetListsByID[ee->unitID].push_back(&staticIntelUnitIDs); }
}

void XAICMilitaryTaskHandler::UnitFinished(const XAIUnitFinishedEvent* ee) {
	if (unitSetListsByID.find(ee->unitID) == unitSetListsByID.end()) {
		// UnitFinished not preceded by UnitCreated
		XAIUnitCreatedEvent e;
			e.type      = XAI_EVENT_UNIT_CREATED;
			e.unitID    = ee->unitID;
			e.builderID = -1;
			e.frame     = ee->frame;
		UnitCreated(&e);
	}
}

void XAICMilitaryTaskHandler::UnitGiven(const XAIUnitGivenEvent* ee) {
	if (ee->newUnitTeam != xaih->rcb->GetMyTeam()) {
		return;
	}

	// convert the event
	XAIUnitCreatedEvent e;
		e.unitID    = ee->unitID;
		e.builderID = -1;
		e.frame     = ee->frame;
	UnitCreated(&e);
}


void XAICMilitaryTaskHandler::UnitDestroyed(const XAIUnitDestroyedEvent* ee) {
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

	if (taskListIterators.find(ee->unitID) != taskListIterators.end()) {
		delete taskListIterators[ee->unitID];
		taskListIterators.erase(ee->unitID);
	}
}

void XAICMilitaryTaskHandler::UnitCaptured(const XAIUnitCapturedEvent* ee) {
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
