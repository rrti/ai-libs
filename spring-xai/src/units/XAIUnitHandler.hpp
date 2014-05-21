#ifndef XAI_UNITHANDLER_HDR
#define XAI_UNITHANDLER_HDR

#include <list>
#include <map>
#include <set>
#include <vector>

#include "System/float3.h"
#include "../events/XAIIEventReceiver.hpp"

class XAICUnit;
struct XAIHelper;
struct XAIUnitCreatedEvent;
struct XAIUnitFinishedEvent;
struct XAIUnitDestroyedEvent;
struct XAIUnitGivenEvent;
struct XAIUnitCapturedEvent;
struct XAIUnitDamagedEvent;
struct XAIUnitIdleEvent;

class XAICUnitHandler: public XAIIEventReceiver {
public:
	XAICUnitHandler(XAIHelper* h): xaih(h) {}
	void OnEvent(const XAIIEvent*);

	const std::set<int>& GetCreatedUnits() const { return createdUnitsByID; }
	const std::set<int>& GetFinishedUnits() const { return finishedUnitsByID; }
	const std::set<int>& GetDamagedUnits() const { return damagedUnitsByID; }
	const std::set<int>& GetIdleUnits() const { return idleUnitsByID; }

	bool IsUnitIdle(int unitID) const {
		return (idleUnitsByID.find(unitID) != idleUnitsByID.end());
	}
	bool IsUnitCreatedOrFinished(int unitID) const {
		return
			(createdUnitsByID.find(unitID) != createdUnitsByID.end() ||
			finishedUnitsByID.find(unitID) != finishedUnitsByID.end());
	}
	bool IsUnitFinished(int unitID) const {
		return (finishedUnitsByID.find(unitID) != finishedUnitsByID.end());
	}


	// assumes caller does the bounds-check
	XAICUnit* GetUnitByID(int unitID) const {
		return (unitsByID[unitID]);
	}
	const std::set<XAICUnit*>& GetUnitsByUnitDefID(int uDefID) const {
		static const std::set<XAICUnit*> s; // dummy
		const std::map<int, std::set<XAICUnit*> >::const_iterator it = unitsByUnitDefID.find(uDefID);

		if (it == unitsByUnitDefID.end()) {
			return s;
		} else {
			return (it->second);
		}
	}


	// unimplemented
	// void GetUnitIDsByTypeMaskApprox(unsigned int, std::list<int>*) const {}
	// only returns units whose type-mask exactly matches <m>
	const std::set<int>& GetUnitIDsByTypeMaskExact(unsigned int m) const {
		static const std::set<int> s; // dummy
		const std::map<unsigned int, std::set<int> >::const_iterator it = unitsByTypeMask.find(m);

		if (it == unitsByTypeMask.end()) {
			return s;
		} else {
			return (it->second);
		}
	}

	// unimplemented
	// void GetUnitIDsByTerrMaskApprox(unsigned int, std::list<int>*) const {}
	// only returns units whose terrain-mask exactly matches <m>
	const std::set<int>& GetUnitIDsByTerrMaskExact(unsigned int m) const {
		static const std::set<int> s; // dummy
		const std::map<unsigned int, std::set<int> >::const_iterator it = unitsByTerrMask.find(m);

		if (it == unitsByTerrMask.end()) {
			return s;
		} else {
			return (it->second);
		}
	}

	// unimplemented
	// void GetUnitIDsByWeapMaskApprox(unsigned int, std::list<int>*) const {}
	// only returns units whose weapon-mask exactly matches <m>
	const std::set<int>& GetUnitIDsByWeapMaskExact(unsigned int m) const {
		static const std::set<int> s; // dummy
		const std::map<unsigned int, std::set<int> >::const_iterator it = unitsByWeapMask.find(m);

		if (it == unitsByWeapMask.end()) {
			return s;
		} else {
			return (it->second);
		}
	}

	unsigned int GetUnitIDsNearPosByTypeMask(const float3&, float r, std::list<int>*, unsigned int) const;


	// analogous to GetUnitDefIDsForMask(); returns a set
	// of all unitIDs whose {type, terrain, weapon}-masks
	// match the given values
	std::set<int> GetUnitIDsByMaskApprox(unsigned int typeM, unsigned int terrM, unsigned int weapM) const {
		std::set<int> unitIDs;

		std::map<unsigned int, std::set<int> >::const_iterator it;
		std::set<int>::const_iterator sit;

		if (typeM != 0) {
			for (it = unitsByTypeMask.begin(); it != unitsByTypeMask.end(); it++) {
				if ((it->first & typeM) != 0) {
					for (sit = (it->second).begin(); sit != (it->second).end(); sit++) {
						unitIDs.insert(*sit);
					}
				}
			}
		}

		if (terrM != 0) {
			for (it = unitsByTerrMask.begin(); it != unitsByTerrMask.end(); it++) {
				if ((it->first & terrM) != 0) {
					for (sit = (it->second).begin(); sit != (it->second).end(); sit++) {
						unitIDs.insert(*sit);
					}
				}
			}
		}

		if (weapM != 0) {
			for (it = unitsByWeapMask.begin(); it != unitsByWeapMask.end(); it++) {
				if ((it->first & weapM) != 0) {
					for (sit = (it->second).begin(); sit != (it->second).end(); sit++) {
						unitIDs.insert(*sit);
					}
				}
			}
		}

		/*
		for (std::vector<XAICUnit*>::iterator it = unitsByID.begin(); it != unitsByID.end(); it++) {
			const XAICUnit* u = *it;

			if (IsUnitCreatedOrFinished(u->GetID())) {
				const XAIUnitDef* def = u->GetUnitDefPtr();

				const bool b0 = (typeM != 0 && ((def->typeMask    & typeM) != 0));
				const bool b1 = (terrM != 0 && ((def->terrainMask & terrM) != 0));
				const bool b2 = (weapM != 0 && ((def->weaponMask  & weapM) != 0));

				if (b0 || b1 || b2) {
					unitIDs.insert(u->GetID());
				}
			}
		}
		*/

		return unitIDs;
	}

	std::list<XAICUnit*> GetGroupedUnits() const;
	std::list<XAICUnit*> GetNonGroupedUnits() const;

private:
	void Update();
	void UnitCreated(const XAIUnitCreatedEvent*);
	void UnitFinished(const XAIUnitFinishedEvent*);
	void UnitDestroyed(const XAIUnitDestroyedEvent*);
	void UnitGiven(const XAIUnitGivenEvent*);
	void UnitCaptured(const XAIUnitCapturedEvent*);
	void UnitDamaged(const XAIUnitDamagedEvent*);
	void UnitIdle(const XAIUnitIdleEvent*);

	std::vector<XAICUnit*> unitsByID;
	std::map<int, std::set<XAICUnit*> > unitsByUnitDefID;

	std::set<int> createdUnitsByID;
	std::set<int> finishedUnitsByID;
	std::set<int> damagedUnitsByID;
	std::set<int> idleUnitsByID;

	// unit {type, terrain, weapon} bitmask ==> unitID set
	std::map<unsigned int, std::set<int> > unitsByTypeMask;
	std::map<unsigned int, std::set<int> > unitsByTerrMask;
	std::map<unsigned int, std::set<int> > unitsByWeapMask;

	XAIHelper* xaih;
};

#endif
