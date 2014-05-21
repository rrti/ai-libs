#ifndef XAI_TASKLIST_HDR
#define XAI_TASKLIST_HDR

#include <string>
#include <list>
#include <map>
#include <set>

struct LuaParser;

struct XAIHelper;
struct XAIUnitDef;
struct XAIITaskListItem {
	friend struct XAITaskListsParser;

public:
	XAIITaskListItem(int itemIdx): idx(itemIdx) {
	}

	virtual unsigned int GetRepeatCount() const { return repeatCount; }
	virtual unsigned int GetIdx() const { return idx; }

	// todo: "build this if enemy has at least N units of type T" for builders
	// todo: "ignore enemies not in LOS | not in RDR" option for attackers
	unsigned int GetMinFrame() const { return minFrame; }
	unsigned int GetMaxFrame() const { return maxFrame; }
	unsigned int GetMinFrameSpacing() const { return minFrameSpacing; }

protected:
	unsigned int idx;

	unsigned int repeatCount;

	unsigned int minFrame;
	unsigned int maxFrame;
	unsigned int minFrameSpacing;
};


struct XAIAttackTaskListItem: public XAIITaskListItem {
friend struct XAITaskListsParser;
public:
	XAIAttackTaskListItem(unsigned int itemIdx): XAIITaskListItem(itemIdx) {
	}

	unsigned int GetAttackeeDefID() const { return attackeeDefID; }
	const char* GetAttackeeDefName() const { return (attackeeDefName.c_str()); }

	unsigned int GetGroupMinSize() const { return minGroupSize; }
	unsigned int GetGroupMaxSize() const { return maxGroupSize; }
	unsigned int GetGroupMinMtlCost() const { return minGroupMtlCost; }
	unsigned int GetGroupMaxMtlCost() const { return maxGroupMtlCost; }
	unsigned int GetGroupMinNrgCost() const { return minGroupNrgCost; }
	unsigned int GetGroupMaxNrgCost() const { return maxGroupNrgCost; }
	unsigned int GetAttackLimit() const { return attackLimit; }

private:
	// using unit categories might be more flexible?
	std::string attackeeDefName;
	unsigned int attackeeDefID;

	unsigned int minGroupSize;
	unsigned int maxGroupSize;
	unsigned int minGroupMtlCost;
	unsigned int maxGroupMtlCost;
	unsigned int minGroupNrgCost;
	unsigned int maxGroupNrgCost;

	unsigned int attackLimit;
};


struct XAIBuildTaskListItem: public XAIITaskListItem {
friend struct XAITaskListsParser;
public:
	XAIBuildTaskListItem(unsigned int itemIdx): XAIITaskListItem(itemIdx) {
	}

	unsigned int GetBuildeeDefID() const { return buildeeDefID; }
	const char* GetBuildeeDefName() const { return (buildeeDefName.c_str()); }

	unsigned int GetMinNrgIncome() const { return minNrgIncome; }
	unsigned int GetMaxNrgIncome() const { return maxNrgIncome; }
	unsigned int GetMinMtlIncome() const { return minMtlIncome; }
	unsigned int GetMaxMtlIncome() const { return maxMtlIncome; }
	unsigned int GetMinNrgUsage() const { return minNrgUsage; }
	unsigned int GetMaxNrgUsage() const { return maxNrgUsage; }
	unsigned int GetMinMtlUsage() const { return minMtlUsage; }
	unsigned int GetMaxMtlUsage() const { return maxMtlUsage; }
	unsigned int GetMinNrgLevel() const { return minNrgLevel; }
	unsigned int GetMaxNrgLevel() const { return maxNrgLevel; }
	unsigned int GetMinMtlLevel() const { return minMtlLevel; }
	unsigned int GetMaxMtlLevel() const { return maxMtlLevel; }

	unsigned int GetUnitLimit() const { return unitLimit; }
	unsigned int GetBuildLimit() const { return buildLimit; }

	bool ForceBuild() const { return forceBuild; }

private:
	std::string buildeeDefName;
	unsigned int buildeeDefID;

	unsigned int minNrgIncome, maxNrgIncome;
	unsigned int minMtlIncome, maxMtlIncome;
	unsigned int minNrgUsage,  maxNrgUsage;
	unsigned int minMtlUsage,  maxMtlUsage;
	unsigned int minNrgLevel,  maxNrgLevel;
	unsigned int minMtlLevel,  maxMtlLevel;

	unsigned int unitLimit;
	unsigned int buildLimit;

	bool forceBuild;
};


struct XAITaskList {
public:
	XAITaskList(int listIdx, int listWeight): idx(listIdx), weight(listWeight) {
	}
	~XAITaskList() {
		for (std::map<int, const XAIITaskListItem*>::iterator it = items.begin(); it != items.end(); it++) {
			delete it->second;
		}
	}

	const std::map<int, const XAIITaskListItem*>& GetItems() const { return items; }
	void AddItem(const XAIITaskListItem* item) { items[item->GetIdx()] = item; }

	int GetIdx() const { return idx; }
	int GetWeight() const { return weight; }

private:
	int idx;
	int weight;

	std::map<int, const XAIITaskListItem*> items;
};



struct XAITaskListIterator {
public:
	XAITaskListIterator(): lst(NULL), numRepeats(0) {
	}

	bool IsValid() const {
		return (lstIt != (lst->GetItems()).end());
	}

	void SetList(const XAITaskList* _lst) {
		lst   = _lst;
		lstIt = (lst->GetItems()).begin();

		numRepeats = (lstIt->second)->GetRepeatCount() - 1;
	}

	const XAIITaskListItem* GetNextTaskItem(XAIHelper* h, bool ignoreRepeat) {
		const XAIITaskListItem* item = lstIt->second;

		if (numRepeats <= 0 || ignoreRepeat) {
			lstIt++;

			if (IsValid()) {
				numRepeats = lstIt->second->GetRepeatCount() - 1;
			}
		} else {
			numRepeats--;
		}

		return item;
	}

private:
	const XAITaskList* lst;
	std::map<int, const XAIITaskListItem*>::const_iterator lstIt;

	int numRepeats;
};



// holds all parsed task-lists for all builders and attackers
// in a single named configuration table (the mapping is from
// UnitDefID to task-lists)
struct LuaTable;
struct XAITaskListsParser {
public:
	typedef std::map<int, std::list<XAITaskList*> > TaskListsMap;

	~XAITaskListsParser();

	// these check if a given {build, attack}-eeID
	// is restrained by a given {build, attack}-erID
	bool HasBuildeeBuilderItem(int, int) const;
	bool HasAttackeeAttackerItem(int, int) const;

	// get all the task-lists for a given {builder, attacker}
	const std::list<XAITaskList*>& GetTaskListsForBuilderDef(const XAIUnitDef*) const;
	const std::list<XAITaskList*>& GetTaskListsForAttackerDef(const XAIUnitDef*) const;

	// get a single task-list for a given unitDef, using
	// roulette-selection over their weights (where r is
	// a random number in [0.0, 1.0])
	const XAITaskList* GetTaskListForUnitDefRS(const XAIUnitDef*, float, bool) const;

	void ParseTaskLists(XAIHelper*);
	void WriteDefaultTaskListsFile(XAIHelper*, const std::string&);

private:
	void ParseBuilderConfigSection(const LuaTable*, XAIHelper*);
	void ParseAttackerConfigSection(const LuaTable*, XAIHelper*);
	void ParseConfigSections(const LuaTable*, XAIHelper*);

	TaskListsMap builderTaskListsMap;
	TaskListsMap attackerTaskListsMap;

	std::map<int, std::set<int> > buildeeBuilderItems;
	std::map<int, std::set<int> > attackeeAttackerItems;
};

#endif
