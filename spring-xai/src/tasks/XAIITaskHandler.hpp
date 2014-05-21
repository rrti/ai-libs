#ifndef XAI_ITASKHANDLER_HDR
#define XAI_ITASKHANDLER_HDR

#include <list>
#include <map>
#include <set>
#include <vector>

#include "System/float3.h"
#include "Sim/Misc/GlobalConstants.h"
#include "../events/XAIIEventReceiver.hpp"

struct XAIITask;
struct XAIHelper;
class XAIITaskHandler: public XAIIEventReceiver {
public:
	XAIITaskHandler(XAIHelper* h): numTaskIDs(0), xaih(h) {}
	virtual ~XAIITaskHandler() {}

	virtual void AddTask(XAIITask*);
	virtual void DelTask(XAIITask*);
	virtual bool HasTask(XAIITask*) const;

	virtual void OnEvent(const XAIIEvent*);

protected:
	virtual void Update();

	int numTaskIDs;

	std::map<int, XAIITask*> tasksByID;
	std::map<int, std::list< std::set<int>*> > unitSetListsByID;

	XAIHelper* xaih;
};




class XAICUnit;
struct XAIUnitDef;
struct XAIGroup;
class XAICEconomyStateTracker;
struct XAITaskRequestQueue;
struct XAITaskRequest;
struct XAIIResource;
struct XAIUnitCreatedEvent;
struct XAIUnitFinishedEvent;
struct XAIUnitDestroyedEvent;
struct XAIUnitDamagedEvent;
struct XAIUnitGivenEvent;
struct XAIUnitCapturedEvent;
struct XAITaskListIterator;
struct XAIBuildTaskListItem;
struct XAIAttackTaskListItem;

class XAICEconomyTaskHandler: public XAIITaskHandler {
public:
	XAICEconomyTaskHandler(XAIHelper* h): XAIITaskHandler(h) {
	}

	void OnEvent(const XAIIEvent*);

	const std::set<int>& GetMobileBuilderUnitIDs() const { return mobileBuilderUnitIDs; }
	const std::set<int>& GetStaticBuilderUnitIDs() const { return staticBuilderUnitIDs; }

	size_t GetMobileProducerUnitIDs(std::set<int>* s) const {
		std::set<int>::const_iterator sit;

		// merge the mobile M- and E-producers
		for (sit = mobileEProducerUnitIDs.begin(); sit != mobileEProducerUnitIDs.end(); sit++) { s->insert(*sit); }
		for (sit = mobileMProducerUnitIDs.begin(); sit != mobileMProducerUnitIDs.end(); sit++) { s->insert(*sit); }

		return (s->size());
	}
	size_t GetStaticProducerUnitIDs(std::set<int>* s) const {
		std::set<int>::const_iterator sit;

		// merge the static M- and E-producers
		for (sit = staticEProducerUnitIDs.begin(); sit != staticEProducerUnitIDs.end(); sit++) { s->insert(*sit); }
		for (sit = staticMProducerUnitIDs.begin(); sit != staticMProducerUnitIDs.end(); sit++) { s->insert(*sit); }

		return (s->size());
	}

	bool IsStaticBuilderAssisted(int unitID) const {
		return (assistedStaticBuilderUnitIDs.find(unitID) != assistedStaticBuilderUnitIDs.end());
	}
	const XAIITask* GetStaticBuilderAssistTask(int unitID) const {
		std::map<int, XAIITask*>::const_iterator it = assistTasksByStaticBuilderUnitID.find(unitID);

		if (it != assistTasksByStaticBuilderUnitID.end()) {
			assert(IsStaticBuilderAssisted(unitID));
			return it->second;
		}

		return 0;
	}

protected:
	void Update();

private:
	typedef std::list<XAIIResource*>                 ResLst;
	typedef std::list<XAIIResource*>::const_iterator ResLstIt;

	typedef std::pair<const XAIUnitDef*, float3> DefPosPair;
	typedef std::pair<XAIIResource*, float> ResDstPair;

	typedef std::list<const XAIUnitDef*>                 UnitDefLst;
	typedef std::list<const XAIUnitDef*>::const_iterator UnitDefLstIt;
	typedef std::list<ResDstPair>                 ResDstPairLst;
	typedef std::list<ResDstPair>::const_iterator ResDstPairIt;
	typedef std::pair<const XAIIResource*, const XAIUnitDef*> ResDefPair;

	void UpdateTasks();
	void UnitCreated(const XAIUnitCreatedEvent*);
	void UnitFinished(const XAIUnitFinishedEvent*);
	void UnitDestroyed(const XAIUnitDestroyedEvent*);
	void UnitGiven(const XAIUnitGivenEvent*);
	void UnitCaptured(const XAIUnitCapturedEvent*);
	void UnitDamaged(const XAIUnitDamagedEvent*);

	int GetNonGroupedFinishedBuilders(std::list<XAICUnit*>*, std::list<XAICUnit*>*);
	void AssignIdleUnitsToIdleGroups(std::list<XAIGroup*>*);
	void MergeIdleGroups(std::list<XAIGroup*>*);
	void AssignIdleGroupsToTasks(std::list<XAIGroup*>*);
	bool TryAddReclaimTaskForGroup(XAIGroup*);
	bool TryAddBuildTaskForGroup(XAIGroup*, const XAIBuildTaskListItem*);
	bool TryAddBuildAssistTaskForGroup(XAIGroup*);
	bool TryAddLuaTaskForGroup(XAIGroup*);

	std::pair<const XAIUnitDef*, float3> GetBestUnitDefForGroup(const XAIGroup*, const XAIBuildTaskListItem*);
	std::list<const XAIUnitDef*> GetFeasibleUnitDefsForGroup(
		XAITaskRequestQueue*,
		XAITaskRequest*,
		const XAIGroup*,
		const XAICEconomyStateTracker*
	);

	float3 GetTaskPositionFor(const XAIUnitDef*, const XAIGroup*);
	std::pair<const XAIIResource*, const XAIUnitDef*>
		GetBestMetalResource(const XAIGroup*, const std::list<const XAIUnitDef*>&);
	void GetReachableResources(const XAIGroup*, float, std::list<ResDstPair>*);


	std::set<int> mobileBuilderUnitIDs;
	std::set<int> staticBuilderUnitIDs;
	std::set<int> mobileAssisterUnitIDs;
	std::set<int> staticAssisterUnitIDs;
	std::set<int> mobileEProducerUnitIDs, staticEProducerUnitIDs; // GetCombinedEProductionIncome()?
	std::set<int> mobileMProducerUnitIDs, staticMProducerUnitIDs; // GetCombinedMProductionIncome()?
	std::set<int> mobileEStorageUnitIDs,  staticEStorageUnitIDs;  // GetCombinedEStorageCapacity()?
	std::set<int> mobileMStorageUnitIDs,  staticMStorageUnitIDs;  // GetCombinedMStorageCapacity()?

	// all static builders that have some other group(s)
	// of units assisting them via a BuildAssistTask (a
	// static builder can have one such task associated
	// with it at once)
	std::set<int> assistedStaticBuilderUnitIDs;
	// maps each static builder to an assist-task created for it
	std::map<int, XAIITask*> assistTasksByStaticBuilderUnitID;
	std::map<int, int> buildTaskCountsForUnitDefID;

	std::map<int, XAITaskListIterator*> taskListIterators;

	std::list<XAIIResource*> recResPositions;
	std::list<XAIIResource*> extResPositions;
};


class XAICMilitaryTaskHandler: public XAIITaskHandler {
public:
	XAICMilitaryTaskHandler(XAIHelper* h): XAIITaskHandler(h) {
		enemyUnitIDs.resize(MAX_UNITS, 0);
	}

	void OnEvent(const XAIIEvent*);

protected:
	void Update();

private:
	void UpdateTasks();

	void UnitCreated(const XAIUnitCreatedEvent*);
	void UnitFinished(const XAIUnitFinishedEvent*);
	void UnitDestroyed(const XAIUnitDestroyedEvent*);
	void UnitGiven(const XAIUnitGivenEvent*);
	void UnitCaptured(const XAIUnitCapturedEvent*);

	int GetNonGroupedFinishedOffenders(std::list<XAICUnit*>*, std::list<XAICUnit*>*);
	void AssignIdleUnitsToIdleGroups(std::list<XAIGroup*>*);
	void MergeIdleGroups(std::list<XAIGroup*>*);
	void AssignIdleGroupsToTasks(std::list<XAIGroup*>*);
	bool TryAddAttackTaskForGroup(XAIGroup*, const XAIAttackTaskListItem*);
	bool TryAddDefendTaskForGroup(XAIGroup*);
	bool TryAddLuaTaskForGroup(XAIGroup*);

	int GetBestAttackeeIDForGroup(XAIGroup*, const XAIAttackTaskListItem*);
	int GetBestDefendeeIDForGroup(XAIGroup*, const XAIAttackTaskListItem*);

	std::set<int> enemyUnitIDsInLOS;
	std::set<int> enemyUnitIDsInRDR;

	std::set<int> mobileDefenseUnitIDs;
	std::set<int> staticDefenseUnitIDs;
	std::set<int> mobileOffenseUnitIDs;
	std::set<int> staticOffenseUnitIDs;
	std::set<int> mobileIntelUnitIDs;
	std::set<int> staticIntelUnitIDs;

	std::vector<int> enemyUnitIDs;

	// maps each enemy unitID to the number of attack tasks
	// assigned to destroy the unit corresponding to the ID
	std::map<int, int> attackTaskCountsForUnitID;
	std::map<int, int> defendTaskCountsForUnitID;

	std::map<int, XAITaskListIterator*> taskListIterators;
};

#endif
