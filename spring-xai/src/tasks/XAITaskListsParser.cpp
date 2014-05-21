#include <climits>
#include <sstream>

#include "LegacyCpp/IAICallback.h"

#include "./XAITaskListsParser.hpp"
#include "../exports/XAIExports.hpp"
#include "../main/XAIHelper.hpp"
#include "../units/XAIUnitDef.hpp"
#include "../units/XAIUnitDefHandler.hpp"
#include "../utils/XAILogger.hpp"
#include "../utils/XAILuaParser.hpp"
#include "../utils/XAIRNG.hpp"

XAITaskListsParser::~XAITaskListsParser() {
	for (TaskListsMap::iterator mit = builderTaskListsMap.begin(); mit != builderTaskListsMap.end(); mit++) {
		std::list<XAITaskList*>& taskLists = mit->second;

		for (std::list<XAITaskList*>::iterator lit = taskLists.begin(); lit != taskLists.end(); lit++) {
			delete *lit;
		}
	}

	for (TaskListsMap::iterator mit = attackerTaskListsMap.begin(); mit != attackerTaskListsMap.end(); mit++) {
		std::list<XAITaskList*>& taskLists = mit->second;

		for (std::list<XAITaskList*>::iterator lit = taskLists.begin(); lit != taskLists.end(); lit++) {
			delete *lit;
		}
	}
}


bool XAITaskListsParser::HasBuildeeBuilderItem(int buildeeDefID, int builderDefID) const {
	const std::map<int, std::set<int> >::const_iterator it = buildeeBuilderItems.find(buildeeDefID);

	if (it != buildeeBuilderItems.end()) {
		return ((it->second).find(builderDefID) != (it->second).end());
	}

	return false;
}
bool XAITaskListsParser::HasAttackeeAttackerItem(int attackeeDefID, int attackerDefID) const {
	const std::map<int, std::set<int> >::const_iterator it = attackeeAttackerItems.find(attackeeDefID);

	if (it != attackeeAttackerItems.end()) {
		return ((it->second).find(attackerDefID) != (it->second).end());
	}

	return false;
}


const std::list<XAITaskList*>& XAITaskListsParser::GetTaskListsForBuilderDef(const XAIUnitDef* uDef) const {
	const static std::list<XAITaskList*> taskLists; // dummy
	const TaskListsMap::const_iterator it = builderTaskListsMap.find(uDef->GetID());

	if (it != builderTaskListsMap.end()) {
		return it->second;
	}

	return taskLists;
}

const std::list<XAITaskList*>& XAITaskListsParser::GetTaskListsForAttackerDef(const XAIUnitDef* uDef) const {
	const static std::list<XAITaskList*> taskLists; // dummy
	const TaskListsMap::const_iterator it = attackerTaskListsMap.find(uDef->GetID());

	if (it != attackerTaskListsMap.end()) {
		return it->second;
	}

	return taskLists;
}


const XAITaskList* XAITaskListsParser::GetTaskListForUnitDefRS(const XAIUnitDef* uDef, float r, bool builder) const {
	const static XAITaskList* taskList = NULL; // dummy
	const std::list<XAITaskList*>& lists = builder?
		GetTaskListsForBuilderDef(uDef):
		GetTaskListsForAttackerDef(uDef);

	if (lists.empty()) {
		return taskList;
	}

	float wSum = 0.0f;
	float pSum = 0.0f;

	for (std::list<XAITaskList*>::const_iterator it = lists.begin(); it != lists.end(); it++) {
		wSum += float((*it)->GetWeight());
	}

	// all zero-weight lists should have been
	// filtered out during the parsing phase
	assert(wSum > 0.0f);

	for (std::list<XAITaskList*>::const_iterator it = lists.begin(); it != lists.end(); it++) {
		pSum += (float((*it)->GetWeight()) / wSum);

		if (pSum >= r) {
			return (*it);
		}
	}

	return taskList;
}



void XAITaskListsParser::ParseBuilderConfigSection(const LuaTable* tbl, XAIHelper* h) {
	const LuaTable* builderTaskListsTbl = tbl->GetTblVal("builderlists");

	if (builderTaskListsTbl == NULL) {
		LOG_BASIC(h->logger, "[XAITaskListsParser::ParseBuilderConfigSection] missing \"BuilderLists\" table");
		return;
	}

	std::list<std::string> builders;
	std::list<std::string>::const_iterator lsit;

	builderTaskListsTbl->GetStrTblKeys(&builders);

	for (lsit = builders.begin(); lsit != builders.end(); lsit++) {
		// retrieve the item-lists for this builder
		const LuaTable* builderListsTbl = builderTaskListsTbl->GetTblVal(lsit->c_str());

		if (builderListsTbl == NULL)
			continue;

		// retrieve the index of each item-list
		std::list<int> listIndices;
		std::list<int>::const_iterator listIdxIt;
		builderListsTbl->GetIntTblKeys(&listIndices);

		const std::string builderDefName = *lsit;
		const UnitDef*    builderDef     = h->rcb->GetUnitDef(builderDefName.c_str());

		if (builderDef == NULL)
			continue;

		builderTaskListsMap[builderDef->id] = std::list<XAITaskList*>();

		for (listIdxIt = listIndices.begin(); listIdxIt != listIndices.end(); listIdxIt++) {
			const LuaTable* buildList = builderListsTbl->GetTblVal(*listIdxIt);

			if (buildList == NULL)
				continue;

			const LuaTable* listItems  = buildList->GetTblVal("items");
			const int       listWeight = buildList->GetIntVal("weight", 0);

			if (listItems == NULL)
				continue;
			if (listWeight <= 0)
				continue;

			std::list<int> itemIndices;
			listItems->GetIntTblKeys(&itemIndices);

			XAITaskList* taskList = new XAITaskList(*listIdxIt, listWeight);
			builderTaskListsMap[builderDef->id].push_back(taskList);

			for (std::list<int>::const_iterator itemIdxIt = itemIndices.begin(); itemIdxIt != itemIndices.end(); itemIdxIt++) {
				const LuaTable* item = listItems->GetTblVal(*itemIdxIt);

				if (item == NULL)
					continue;

				XAIBuildTaskListItem* taskListItem = new XAIBuildTaskListItem(*itemIdxIt);
					taskListItem->buildeeDefName = item->GetStrVal("buildeedef", "");

					// unsigned representation of -1 is ((1 << 32) - 1),
					// so such values act as positive infinity if given
					taskListItem->repeatCount     = std::max(item->GetIntVal("repeatcount",           1),       1);
					taskListItem->unitLimit       = std::min(item->GetIntVal("unitlimit",       INT_MAX), INT_MAX);
					taskListItem->buildLimit      = std::min(item->GetIntVal("buildlimit",      INT_MAX), INT_MAX);

					taskListItem->minNrgIncome    = std::max(item->GetIntVal("minenergyincome",       0),       0);
					taskListItem->maxNrgIncome    = std::min(item->GetIntVal("maxenergyincome", INT_MAX), INT_MAX);
					taskListItem->minMtlIncome    = std::max(item->GetIntVal("minmetalincome",        0),       0);
					taskListItem->maxMtlIncome    = std::min(item->GetIntVal("maxmetalincome",  INT_MAX), INT_MAX);

					taskListItem->minNrgUsage     = std::max(item->GetIntVal("minenergyusage",        0),       0);
					taskListItem->maxNrgUsage     = std::min(item->GetIntVal("maxenergyusage",  INT_MAX), INT_MAX);
					taskListItem->minMtlUsage     = std::max(item->GetIntVal("minmetalusage",         0),       0);
					taskListItem->maxMtlUsage     = std::min(item->GetIntVal("maxmetalusage",   INT_MAX), INT_MAX);

					taskListItem->minNrgLevel     = std::max(item->GetIntVal("minenergylevel",        0),       0);
					taskListItem->maxNrgLevel     = std::min(item->GetIntVal("maxenergylevel",  INT_MAX), INT_MAX);
					taskListItem->minMtlLevel     = std::max(item->GetIntVal("minmetallevel",         0),       0);
					taskListItem->maxMtlLevel     = std::min(item->GetIntVal("maxmetallevel",   INT_MAX), INT_MAX);

					taskListItem->minFrame        = std::max(item->GetIntVal("mingameframe",          0),       0);
					taskListItem->maxFrame        = std::min(item->GetIntVal("maxgameframe",    INT_MAX), INT_MAX);
					taskListItem->minFrameSpacing = std::max(item->GetIntVal("minframespacing",       0),       0);
					taskListItem->forceBuild      =          item->GetIntVal("forcestartbuild",       0);

				const UnitDef* buildeeDef   = h->rcb->GetUnitDef(taskListItem->buildeeDefName.c_str());
				const int      buildeeDefID = (buildeeDef != NULL)? buildeeDef->id: 0;

				taskListItem->buildeeDefID = buildeeDefID;
				taskList->AddItem(taskListItem);


				if (buildeeBuilderItems.find(buildeeDefID) == buildeeBuilderItems.end()) {
					buildeeBuilderItems[buildeeDefID] = std::set<int>();
				}

				buildeeBuilderItems[buildeeDefID].insert(builderDef->id);
			}
		}
	}
}

void XAITaskListsParser::ParseAttackerConfigSection(const LuaTable* tbl, XAIHelper* h) {
	const LuaTable* attackerTaskListsTbl = tbl->GetTblVal("attackerlists");

	if (attackerTaskListsTbl == NULL) {
		LOG_BASIC(h->logger, "[XAITaskListsParser::ParseAttackerConfigSection] missing \"AttackerLists\" table");
		return;
	}

	std::list<std::string> attackers;
	std::list<std::string>::const_iterator lsit;

	attackerTaskListsTbl->GetStrTblKeys(&attackers);

	for (lsit = attackers.begin(); lsit != attackers.end(); lsit++) {
		// retrieve the item-lists for this attacker
		const LuaTable* attackerListsTbl = attackerTaskListsTbl->GetTblVal(lsit->c_str());

		if (attackerListsTbl == NULL)
			continue;

		// retrieve the index of each item-list
		std::list<int> listIndices;
		std::list<int>::const_iterator listIdxIt;
		attackerListsTbl->GetIntTblKeys(&listIndices);

		const std::string attackerDefName = *lsit;
		const UnitDef*    attackerDef     = h->rcb->GetUnitDef(attackerDefName.c_str());

		if (attackerDef == NULL)
			continue;

		attackerTaskListsMap[attackerDef->id] = std::list<XAITaskList*>();

		for (listIdxIt = listIndices.begin(); listIdxIt != listIndices.end(); listIdxIt++) {
			const LuaTable* attackList = attackerListsTbl->GetTblVal(*listIdxIt);

			if (attackList == NULL)
				continue;

			const LuaTable* listItems  = attackList->GetTblVal("items");
			const int       listWeight = attackList->GetIntVal("weight", 0);

			if (listItems == NULL)
				continue;
			if (listWeight <= 0)
				continue;

			std::list<int> itemIndices;
			listItems->GetIntTblKeys(&itemIndices);

			XAITaskList* taskList = new XAITaskList(*listIdxIt, listWeight);
			attackerTaskListsMap[attackerDef->id].push_back(taskList);

			for (std::list<int>::const_iterator itemIdxIt = itemIndices.begin(); itemIdxIt != itemIndices.end(); itemIdxIt++) {
				const LuaTable* item = listItems->GetTblVal(*itemIdxIt);

				if (item == NULL)
					continue;

				XAIAttackTaskListItem* taskListItem = new XAIAttackTaskListItem(*itemIdxIt);
					taskListItem->attackeeDefName = item->GetStrVal("attackeedef", "");

					taskListItem->repeatCount     = std::max(item->GetIntVal("repeatcount",        INT_MAX), INT_MAX);
					taskListItem->attackLimit     = std::min(item->GetIntVal("attacklimit",        INT_MAX), INT_MAX);

					taskListItem->minGroupSize    = std::max(item->GetIntVal("mingroupsize",             1),       1);
					taskListItem->maxGroupSize    = std::min(item->GetIntVal("maxgroupsize",       INT_MAX), INT_MAX);
					taskListItem->minGroupMtlCost = std::max(item->GetIntVal("mingroupmetalcost",        0),       0);
					taskListItem->maxGroupMtlCost = std::min(item->GetIntVal("maxgroupmetalcost",  INT_MAX), INT_MAX);
					taskListItem->minGroupNrgCost = std::max(item->GetIntVal("mingroupenergycost",       0),       0);
					taskListItem->maxGroupNrgCost = std::min(item->GetIntVal("maxgroupenergycost", INT_MAX), INT_MAX);

					taskListItem->minFrame        = std::max(item->GetIntVal("mingameframe",             0),       0);
					taskListItem->maxFrame        = std::min(item->GetIntVal("maxgameframe",       INT_MAX), INT_MAX);
					taskListItem->minFrameSpacing = std::max(item->GetIntVal("minframespacing",          0),       0);

				const UnitDef* attackeeDef   = h->rcb->GetUnitDef(taskListItem->attackeeDefName.c_str());
				const int      attackeeDefID = (attackeeDef != NULL)? attackeeDef->id: 0;

				taskListItem->attackeeDefID = attackeeDefID;
				taskList->AddItem(taskListItem);


				if (attackeeAttackerItems.find(attackeeDefID) == attackeeAttackerItems.end()) {
					attackeeAttackerItems[attackeeDefID] = std::set<int>();
				}

				attackeeAttackerItems[attackeeDefID].insert(attackerDef->id);
			}
		}
	}
}

void XAITaskListsParser::ParseConfigSections(const LuaTable* tbl, XAIHelper* h) {
	ParseBuilderConfigSection(tbl, h);
	ParseAttackerConfigSection(tbl, h);
}



void XAITaskListsParser::ParseTaskLists(XAIHelper* h) {
	const LuaTable* root = h->luaParser->GetRootTbl();

	std::stringstream xaiVersionStr(aiexport_getVersion());
	std::stringstream cfgVersionStr(root->GetStrVal("version", xaiVersionStr.str()));

	float xaiVersion = 0.0f; xaiVersionStr >> xaiVersion;
	float cfgVersion = 0.0f; cfgVersionStr >> cfgVersion;

	if (xaiVersion < cfgVersion) {
		LOG_BASIC(h->logger, "[XAITaskListsParser::ParseTaskLists] unknown config-file version (" << cfgVersionStr.str() << ")");
		return;
	}


	std::list<std::string> configNameKeys;
	root->GetStrTblKeys(&configNameKeys);

	// there must be at least one named configuration
	if (configNameKeys.empty()) {
		LOG_BASIC(h->logger, "[XAITaskListsParser::ParseTaskLists] no named configurations defined");
		h->SetConfig(false);
		return;
	}


	// we select the config randomly up-front from
	// the given (named) configurations, so that we
	// only have to read a single subtable
	float configWgtSum = 0.0f;
	float configPrbSum = 0.0f;

	int             rndConfigWgt = 0;
	const float     rndConfigPrb = (*h->frng)();
	const LuaTable* rndConfigTbl = 0;

	for (std::list<std::string>::const_iterator it = configNameKeys.begin(); it != configNameKeys.end(); it++) {
		rndConfigTbl = root->GetTblVal(*it);
		rndConfigWgt = rndConfigTbl->GetIntVal("weight", 0);

		if (rndConfigWgt > 0) {
			configWgtSum += rndConfigWgt;
		}
	}

	// there must be at least one named
	// configuration with non-zero weight
	if (configWgtSum <= 0) {
		LOG_BASIC(h->logger, "[XAITaskListsParser::ParseTaskLists] no configurations with positive weight defined");
		h->SetConfig(false);
		return;
	}

	for (std::list<std::string>::const_iterator it = configNameKeys.begin(); it != configNameKeys.end(); it++) {
		rndConfigTbl = root->GetTblVal(*it);
		configPrbSum += (rndConfigTbl->GetIntVal("weight", 0) / configWgtSum);

		if (configPrbSum >= rndConfigPrb) {
			LOG_BASIC(h->logger, "[XAITaskListsParser::ParseTaskLists] selected configuration \"" << (*it) << "\"");
			break;
		}
	}

	ParseConfigSections(rndConfigTbl, h);
}

void XAITaskListsParser::WriteDefaultTaskListsFile(XAIHelper* h, const std::string& cfgFileName) {
	std::ofstream fs(cfgFileName.c_str(), std::ios::out);

	std::stringstream vStr(aiexport_getVersion());

	fs << "cfg = {" << std::endl;
	fs << "\t[\"version\"] = \"" << (aiexport_getVersion()) << "\"," << std::endl;
	fs << "\t[\"UnnamedConfig\"] = {" << std::endl;
	fs << "\t\t[\"Weight\"] = 100," << std::endl;
	fs << "\t\t[\"BuilderLists\"] = {" << std::endl;

	const unsigned int builderMask = MASK_BUILDER_MOBILE | MASK_BUILDER_STATIC;
	const unsigned int attackerMask = MASK_OFFENSE_MOBILE | MASK_OFFENSE_STATIC;
	const std::list<std::set<int>* >& builderDefIDSets = h->unitDefHandler->GetUnitDefIDSetsForMask(builderMask, 0, 0);
	const std::list<std::set<int>* >& attackerDefIDSets = h->unitDefHandler->GetUnitDefIDSetsForMask(attackerMask, 0, 0);
	const XAIUnitDef* builderDef  = 0;
	const XAIUnitDef* buildeeDef  = 0;
	const XAIUnitDef* attackerDef = 0;
	const XAIUnitDef* attackeeDef = 0;

	for (std::list<std::set<int>* >::const_iterator lit = builderDefIDSets.begin(); lit != builderDefIDSets.end(); lit++) {
		for (std::set<int>::const_iterator sit = (*lit)->begin(); sit != (*lit)->end(); sit++) {
			builderDef = h->unitDefHandler->GetUnitDefByID(*sit);
			buildeeDef = h->unitDefHandler->GetUnitDefByID(*(builderDef->buildOptionUDIDs.begin()));

			fs << "\t\t\t[\"" << std::string(builderDef->GetName()) << "\"] = {" << std::endl;
			fs << "\t\t\t\t[1] = {" << std::endl;
			fs << "\t\t\t\t\t[\"Weight\"] = 100," << std::endl;
			fs << "\t\t\t\t\t[\"Items\"] = {" << std::endl;
			fs << "\t\t\t\t\t\t[1] = {" << std::endl;
			fs << "\t\t\t\t\t\t\tbuildeeDef = \"" << std::string(buildeeDef->GetName()) << "\"," << std::endl;
			fs << "\t\t\t\t\t\t\tunitCount  = 1," << std::endl;
			fs << "\t\t\t\t\t\t}," << std::endl;
			fs << "\t\t\t\t\t}," << std::endl;
			fs << "\t\t\t\t}," << std::endl;
			fs << "\t\t\t}," << std::endl;
		}
	}

	fs << "\t\t}," << std::endl;
	fs << std::endl;
	fs << "\t\t[\"AttackerLists\"] = {" << std::endl;

	for (std::list<std::set<int>* >::const_iterator lit = attackerDefIDSets.begin(); lit != attackerDefIDSets.end(); lit++) {
		for (std::set<int>::const_iterator sit = (*lit)->begin(); sit != (*lit)->end(); sit++) {
			attackerDef = h->unitDefHandler->GetUnitDefByID(*sit);
			attackeeDef = h->unitDefHandler->GetUnitDefByID(1);

			fs << "\t\t\t[\"" << std::string(attackerDef->GetName()) << "\"] = {" << std::endl;
			fs << "\t\t\t\t[1] = {" << std::endl;
			fs << "\t\t\t\t\t[\"Weight\"] = 100," << std::endl;
			fs << "\t\t\t\t\t[\"Items\"] = {" << std::endl;
			fs << "\t\t\t\t\t\t[1] = {" << std::endl;
			fs << "\t\t\t\t\t\t\tattackeeDef = \"" << std::string(attackeeDef->GetName()) << "\"," << std::endl;
			fs << "\t\t\t\t\t\t}," << std::endl;
			fs << "\t\t\t\t\t}," << std::endl;
			fs << "\t\t\t\t}," << std::endl;
			fs << "\t\t\t}," << std::endl;
		}
	}

	fs << "\t\t}," << std::endl;
	fs << "\t}," << std::endl;
	fs << "}" << std::endl;

	fs.close();
}
